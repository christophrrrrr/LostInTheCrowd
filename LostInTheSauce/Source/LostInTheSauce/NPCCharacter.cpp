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
	constexpr float BodyHeight = 180.f;
	constexpr float BodyWidth = 56.f;
}

ANPCCharacter::ANPCCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(30.f, 90.f);
	// Make sure the click trace (visibility channel) always hits the capsule.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CapsuleFinder(TEXT("/Engine/BasicShapes/Capsule.Capsule"));
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
	BodyMesh->SetStaticMesh(CapsuleFinder.Object);
	BodyMesh->SetMaterial(0, MaterialFinder.Object);
	BodyMesh->SetCollisionProfileName(TEXT("NoCollision"));
	BodyMesh->SetGenerateOverlapEvents(false);
	BodyMesh->SetCanEverAffectNavigation(false);

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

	ScaleMeshTo(BodyMesh, FVector(BodyWidth, BodyWidth, BodyHeight), 0.f);

	if (!BodyMID)
	{
		BodyMID = BodyMesh->CreateAndSetMaterialInstanceDynamic(0);
	}
	BodyMID->SetVectorParameterValue(TEXT("Color"), Style.BodyColor);

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
		ScaleMeshTo(HatMesh, Style.HatSize, BodyHeight * 0.5f + Style.HatSize.Z * 0.5f + 2.f);

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

void ANPCCharacter::ScaleMeshTo(UStaticMeshComponent* Mesh, const FVector& TargetSize, float CenterZ) const
{
	const UStaticMesh* StaticMesh = Mesh ? Mesh->GetStaticMesh() : nullptr;
	if (!StaticMesh)
	{
		return;
	}
	const FVector MeshSize = StaticMesh->GetBoundingBox().GetSize();
	if (MeshSize.GetMin() <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	Mesh->SetRelativeScale3D(TargetSize / MeshSize);
	Mesh->SetRelativeLocation(FVector(0.f, 0.f, CenterZ));
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
