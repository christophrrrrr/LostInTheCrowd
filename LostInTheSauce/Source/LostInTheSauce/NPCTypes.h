#pragma once

#include "CoreMinimal.h"
#include "NPCTypes.generated.h"

UENUM(BlueprintType)
enum class ENPCType : uint8
{
	Knight,
	Guard,
	Merchant,
	Farmer,
	Monk,

	Count UMETA(Hidden)
};

constexpr int32 NPCTypeCount = static_cast<int32>(ENPCType::Count);

enum class EHatShape : uint8
{
	None,
	Cube,
	Sphere,
	Cylinder,
	Cone
};

// Visual identity of one character type. Colors are muted and close together
// on purpose: the player should have to look twice. Hat silhouette is the
// second tell. Tune here, recompile, done.
struct FNPCTypeStyle
{
	const TCHAR* DisplayName;
	FLinearColor BodyColor;
	EHatShape HatShape;
	FVector HatSize; // world-space size in cm (X, Y, height)
};

namespace NPCTypeStyles
{
	inline const FNPCTypeStyle& Get(ENPCType Type)
	{
		static const FNPCTypeStyle Styles[NPCTypeCount] = {
			{ TEXT("Knight"),   FLinearColor(0.30f, 0.33f, 0.40f), EHatShape::Cube,     FVector(42, 42, 16) },
			{ TEXT("Guard"),    FLinearColor(0.36f, 0.19f, 0.17f), EHatShape::Cone,     FVector(36, 36, 34) },
			{ TEXT("Merchant"), FLinearColor(0.46f, 0.36f, 0.13f), EHatShape::Sphere,   FVector(40, 40, 22) },
			{ TEXT("Farmer"),   FLinearColor(0.31f, 0.23f, 0.15f), EHatShape::Cylinder, FVector(56, 56, 10) },
			{ TEXT("Monk"),     FLinearColor(0.28f, 0.28f, 0.19f), EHatShape::None,     FVector::ZeroVector },
		};
		return Styles[FMath::Clamp(static_cast<int32>(Type), 0, NPCTypeCount - 1)];
	}
}
