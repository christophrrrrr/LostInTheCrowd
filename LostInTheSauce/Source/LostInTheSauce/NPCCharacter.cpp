#include "NPCCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NPCAIController.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Board-game-piece proportions: cylinder body, sphere head, hat on top.
	constexpr float BodyHeight = 140.f;
	constexpr float BodyWidth = 54.f;
	constexpr float BodyCenterZ = -20.f;  // body spans -90..+50 relative to capsule center
	constexpr float HeadSize = 40.f;
	constexpr float HeadCenterZ = 68.f;   // head spans +48..+88
	constexpr float HatBaseZ = 86.f;      // hats sit just on top of the head

	// Every NPC gets the same head color so the only tells are outfit and hat.
	const FLinearColor SkinTone(0.55f, 0.42f, 0.32f);
}

ANPCCharacter::ANPCCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(30.f, 90.f);
	// Make sure the click trace (visibility channel) always hits the capsule.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	CubeMesh = CubeFinder.Object;
	SphereMesh = SphereFinder.Object;
	CylinderMesh = CylinderFinder.Object;
	ConeMesh = ConeFinder.Object;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetStaticMesh(CylinderFinder.Object);
	BodyMesh->SetMaterial(0, MaterialFinder.Object);
	BodyMesh->SetCollisionProfileName(TEXT("NoCollision"));
	BodyMesh->SetGenerateOverlapEvents(false);
	BodyMesh->SetCanEverAffectNavigation(false);

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(GetCapsuleComponent());
	HeadMesh->SetStaticMesh(SphereFinder.Object);
	HeadMesh->SetMaterial(0, MaterialFinder.Object);
	HeadMesh->SetCollisionProfileName(TEXT("NoCollision"));
	HeadMesh->SetGenerateOverlapEvents(false);
	HeadMesh->SetCanEverAffectNavigation(false);

	HatMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HatMesh"));
	HatMesh->SetupAttachment(GetCapsuleComponent());
	HatMesh->SetMaterial(0, MaterialFinder.Object);
	HatMesh->SetCollisionProfileName(TEXT("NoCollision"));
	HatMesh->SetGenerateOverlapEvents(false);
	HatMesh->SetCanEverAffectNavigation(false);

	// Walk in the direction of movement instead of following controller yaw.
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 360.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 160.f;

	AIControllerClass = ANPCAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ANPCCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Per-NPC amble speed so the crowd doesn't move in lockstep.
	GetCharacterMovement()->MaxWalkSpeed = FMath::FRandRange(110.f, 210.f);

	ApplyStyle();
}

void ANPCCharacter::SetNPCType(ENPCType InType)
{
	NPCType = InType;
	if (HasActorBegunPlay())
	{
		ApplyStyle();
	}
}

void ANPCCharacter::ApplyStyle()
{
	const FNPCTypeStyle& Style = NPCTypeStyles::Get(NPCType);

	ScaleMeshTo(BodyMesh, FVector(BodyWidth, BodyWidth, BodyHeight), BodyCenterZ);
	ScaleMeshTo(HeadMesh, FVector(HeadSize, HeadSize, HeadSize), HeadCenterZ);

	if (!BodyMID)
	{
		BodyMID = BodyMesh->CreateAndSetMaterialInstanceDynamic(0);
	}
	BodyMID->SetVectorParameterValue(TEXT("Color"), Style.BodyColor);

	if (!HeadMID)
	{
		HeadMID = HeadMesh->CreateAndSetMaterialInstanceDynamic(0);
		HeadMID->SetVectorParameterValue(TEXT("Color"), SkinTone);
	}

	UStaticMesh* HatShapeMesh = nullptr;
	switch (Style.HatShape)
	{
	case EHatShape::Cube:     HatShapeMesh = CubeMesh; break;
	case EHatShape::Sphere:   HatShapeMesh = SphereMesh; break;
	case EHatShape::Cylinder: HatShapeMesh = CylinderMesh; break;
	case EHatShape::Cone:     HatShapeMesh = ConeMesh; break;
	default: break;
	}

	if (HatShapeMesh)
	{
		HatMesh->SetStaticMesh(HatShapeMesh);
		HatMesh->SetVisibility(true);
		ScaleMeshTo(HatMesh, Style.HatSize, HatBaseZ + Style.HatSize.Z * 0.5f);

		if (!HatMID)
		{
			HatMID = HatMesh->CreateAndSetMaterialInstanceDynamic(0);
		}
		// Slightly darker than the body so hats read as gear, not neon markers.
		HatMID->SetVectorParameterValue(TEXT("Color"), Style.BodyColor * 0.65f);
	}
	else
	{
		HatMesh->SetVisibility(false);
	}
}

void ANPCCharacter::ScaleMeshTo(UStaticMeshComponent* MeshComp, const FVector& TargetSize, float CenterZ) const
{
	const UStaticMesh* StaticMesh = MeshComp ? MeshComp->GetStaticMesh() : nullptr;
	if (!StaticMesh)
	{
		return;
	}
	const FVector MeshSize = StaticMesh->GetBoundingBox().GetSize();
	if (MeshSize.GetMin() <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	MeshComp->SetRelativeScale3D(TargetSize / MeshSize);
	MeshComp->SetRelativeLocation(FVector(0.f, 0.f, CenterZ));
}

void ANPCCharacter::ReactToWrongClick()
{
	LaunchCharacter(FVector(0.f, 0.f, 420.f), false, true);

	if (BodyMID)
	{
		BodyMID->SetVectorParameterValue(TEXT("Color"), FLinearColor::White);
		GetWorldTimerManager().SetTimer(FlashTimerHandle, [this]()
		{
			if (BodyMID)
			{
				BodyMID->SetVectorParameterValue(TEXT("Color"), NPCTypeStyles::Get(NPCType).BodyColor);
			}
		}, 0.35f, false);
	}
}

void ANPCCharacter::CelebrateFound()
{
	if (ANPCAIController* AI = Cast<ANPCAIController>(GetController()))
	{
		AI->SetWanderEnabled(false);
	}

	LaunchCharacter(FVector(0.f, 0.f, 450.f), false, true);
	GetWorldTimerManager().SetTimer(CelebrateTimerHandle, [this]()
	{
		LaunchCharacter(FVector(0.f, 0.f, 450.f), false, true);
	}, 0.9f, true);
}
