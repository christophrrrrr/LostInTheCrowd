#pragma once

#include "CoreMinimal.h"
#include "NPCTypes.generated.h"

UENUM(BlueprintType)
enum class ENPCType : uint8
{
	Farmer,
	King,
	Witch,
	Adventurer,
	Medieval,
	// Knight slot pending: user is producing Mixamo animations for it.

	Count UMETA(Hidden)
};

constexpr int32 NPCTypeCount = static_cast<int32>(ENPCType::Count);

// Visual identity of one character type. Mesh paths come from the GLB import
// (Scripts/import_characters.py); each character's animations live in the
// same folder as its mesh and are found by name suffix at runtime.
// Swatch color is only used on the HUD banner.
struct FNPCTypeStyle
{
	const TCHAR* DisplayName;
	const TCHAR* MeshPath;
	FLinearColor SwatchColor;
};

namespace NPCTypeStyles
{
	inline const FNPCTypeStyle& Get(ENPCType Type)
	{
		static const FNPCTypeStyle Styles[NPCTypeCount] = {
			{ TEXT("Farmer"),     TEXT("/Game/LostInTheSauce/Characters/Farmer/Farmer"),         FLinearColor(0.75f, 0.65f, 0.40f) },
			{ TEXT("King"),       TEXT("/Game/LostInTheSauce/Characters/King/King"),             FLinearColor(0.85f, 0.65f, 0.15f) },
			{ TEXT("Witch"),      TEXT("/Game/LostInTheSauce/Characters/Witch/Witch"),           FLinearColor(0.45f, 0.20f, 0.60f) },
			{ TEXT("Adventurer"), TEXT("/Game/LostInTheSauce/Characters/Adventurer/Adventurer"), FLinearColor(0.40f, 0.30f, 0.20f) },
			{ TEXT("Squire"),     TEXT("/Game/LostInTheSauce/Characters/Medieval/Medieval"),     FLinearColor(0.70f, 0.40f, 0.20f) },
		};
		return Styles[FMath::Clamp(static_cast<int32>(Type), 0, NPCTypeCount - 1)];
	}
}
