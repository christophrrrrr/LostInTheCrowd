#include "NPCCharacter.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/Skeleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Misc/CommandLine.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NPCAIController.h"

ANPCCharacter::ANPCCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(30.f, 90.f);
	// Make sure the click trace (visibility channel) always hits the capsule.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// The capsule is the click target; the mesh is display only.
	GetMesh()->SetCollisionProfileName(TEXT("NoCollision"));
	GetMesh()->SetGenerateOverlapEvents(false);
	GetMesh()->SetCanEverAffectNavigation(false);

	// Walk in the direction of movement instead of following controller yaw.
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 360.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 160.f;

	AIControllerClass = ANPCAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

namespace
{
	// Find an animation in the character's own folder by name suffix, so the
	// import can name assets however it likes (e.g. FarmerCharacterArmature_Walk).
	UAnimSequence* FindAnimInFolder(const FString& Folder, const FString& Suffix)
	{
		const FAssetRegistryModule& Registry =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		// The startup registry scan is async; force this folder to be known
		// now, or lookups at BeginPlay come back empty.
		Registry.Get().ScanPathsSynchronous({ Folder }, false);
		TArray<FAssetData> Assets;
		Registry.Get().GetAssetsByPath(FName(*Folder), Assets, true);
		for (const FAssetData& Asset : Assets)
		{
			if (Asset.AssetClassPath.GetAssetName() == TEXT("AnimSequence") &&
				Asset.AssetName.ToString().EndsWith(Suffix))
			{
				return Cast<UAnimSequence>(Asset.GetAsset());
			}
		}
		return nullptr;
	}
}

void ANPCCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Per-NPC amble speed so the crowd doesn't move in lockstep.
	GetCharacterMovement()->MaxWalkSpeed = FMath::FRandRange(110.f, 210.f);

	ApplyMesh();
}

void ANPCCharacter::SetNPCType(ENPCType InType)
{
	NPCType = InType;
	if (HasActorBegunPlay())
	{
		ApplyMesh();
	}
}

void ANPCCharacter::ApplyMesh()
{
	const FNPCTypeStyle& Style = NPCTypeStyles::Get(NPCType);

	USkeletalMesh* SkeletalMesh = LoadObject<USkeletalMesh>(nullptr, Style.MeshPath);
	if (!SkeletalMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("LITS: failed to load mesh %s"), Style.MeshPath);
		return;
	}

	GetMesh()->SetSkeletalMesh(SkeletalMesh);
	// The auto-generated physics assets inherit the broken import scale and
	// corrupt the component bounds we calibrate against. No ragdolls needed.
	GetMesh()->SetPhysicsAsset(nullptr, true);

#if WITH_EDITOR
	// Pin every bone INCLUDING the root to the skeleton's own proportions:
	// the pack's animation tracks carry root orientation/translation that
	// tips characters over and stretches limbs. Animations then drive
	// rotation only, which is all this low-poly pack needs.
	if (USkeleton* Skeleton = SkeletalMesh->GetSkeleton())
	{
		const int32 NumBones = Skeleton->GetReferenceSkeleton().GetNum();
		for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
		{
			Skeleton->SetBoneTranslationRetargetingMode(BoneIndex, EBoneTranslationRetargetingMode::Skeleton);
		}
	}
#endif

	// Stand the glTF character upright (roll 90 lifts the Y-up rig to Z-up)
	// and yaw so it faces the actor's forward. -LITSMeshRot=P,Y,R overrides
	// for quick iteration without recompiling.
	FRotator MeshRotation(0.f, -90.f, 0.f);
	FString RotationOverride;
	// bShouldStopOnSeparator=false: the value contains commas, which FParse
	// otherwise treats as terminators (it silently truncates to "0").
	if (FParse::Value(FCommandLine::Get(), TEXT("LITSMeshRot="), RotationOverride, false))
	{
		TArray<FString> Parts;
		RotationOverride.ParseIntoArray(Parts, TEXT(","));
		if (Parts.Num() == 3)
		{
			MeshRotation = FRotator(FCString::Atof(*Parts[0]), FCString::Atof(*Parts[1]), FCString::Atof(*Parts[2]));
		}
	}
	GetMesh()->SetRelativeRotation(MeshRotation);

	// FBX asset bounds are trustworthy (unlike the old GLBs), so size the
	// character once from the reference pose: TargetHeight tall, feet on the
	// capsule bottom.
	const FBoxSphereBounds RefBounds = SkeletalMesh->GetBounds();
	const float RefHeight = RefBounds.BoxExtent.Z * 2.f;
	if (RefHeight > KINDA_SMALL_NUMBER)
	{
		const float Scale = TargetHeight / RefHeight;
		GetMesh()->SetRelativeScale3D(FVector(Scale));
		const float FeetZ = (RefBounds.Origin.Z - RefBounds.BoxExtent.Z) * Scale;
		GetMesh()->SetRelativeLocation(FVector(0.f, 0.f,
			-GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - FeetZ));
	}

	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	CurrentLoop = nullptr;

	// Clips live in the character's own folder; meshes that imported without
	// animations share the Adventurer's skeleton and borrow its clips.
	const FString Folder = FPackageName::GetLongPackagePath(Style.MeshPath);
	const FString DonorFolder = TEXT("/Game/LostInTheSauce/Characters/Adventurer");
	auto FindAnim = [&Folder, &DonorFolder](const TCHAR* Suffix) -> UAnimSequence*
	{
		if (UAnimSequence* Anim = FindAnimInFolder(Folder, Suffix))
		{
			return Anim;
		}
		UAnimSequence* Donor = FindAnimInFolder(DonorFolder, Suffix);
		if (!Donor)
		{
			UE_LOG(LogTemp, Warning, TEXT("LITS: no anim ending in %s under %s or donor folder"), Suffix, *Folder);
		}
		return Donor;
	};
	WalkAnim = FindAnim(TEXT("_Walk"));
	IdleAnim = FindAnim(TEXT("_Idle_Neutral"));
	WaveAnim = FindAnim(TEXT("_Wave"));
	HitAnim = FindAnim(TEXT("_HitRecieve"));
}

void ANPCCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bCelebrating)
	{
		return; // waving at the player until the round restarts
	}
	if (GetWorld()->GetTimeSeconds() < OneShotEndTime)
	{
		return; // let a one-shot reaction finish
	}

	const float Speed = GetVelocity().Size2D();
	if (Speed > 20.f)
	{
		PlayLooping(WalkAnim);
		// Match stride to actual speed so feet don't skate.
		if (UAnimSingleNodeInstance* Node = GetMesh()->GetSingleNodeInstance())
		{
			Node->SetPlayRate(FMath::Clamp(Speed / 170.f, 0.7f, 1.4f));
		}
	}
	else
	{
		PlayLooping(IdleAnim);
	}
}

void ANPCCharacter::PlayLooping(UAnimSequence* Anim)
{
	if (!Anim || CurrentLoop == Anim)
	{
		return;
	}
	CurrentLoop = Anim;
	GetMesh()->PlayAnimation(Anim, true);
	// Random start offset so 50 NPCs don't animate in perfect sync.
	if (UAnimSingleNodeInstance* Node = GetMesh()->GetSingleNodeInstance())
	{
		Node->SetPosition(FMath::FRand() * Anim->GetPlayLength(), false);
	}
}

void ANPCCharacter::ReactToWrongClick()
{
	LaunchCharacter(FVector(0.f, 0.f, 420.f), false, true);

	if (HitAnim)
	{
		GetMesh()->PlayAnimation(HitAnim, false);
		CurrentLoop = nullptr;
		OneShotEndTime = GetWorld()->GetTimeSeconds() + HitAnim->GetPlayLength();
	}
}

void ANPCCharacter::CelebrateFound()
{
	if (ANPCAIController* AI = Cast<ANPCAIController>(GetController()))
	{
		AI->SetWanderEnabled(false);
	}

	bCelebrating = true;
	if (WaveAnim)
	{
		GetMesh()->PlayAnimation(WaveAnim, true);
		CurrentLoop = WaveAnim;
	}
}
