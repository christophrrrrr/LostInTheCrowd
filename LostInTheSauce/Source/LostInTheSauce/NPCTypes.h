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
	Squire,

	Count UMETA(Hidden)
};

constexpr int32 NPCTypeCount = static_cast<int32>(ENPCType::Count);

// Visual identity of one character type. Mesh paths come from the FBX import
// (Scripts/import_characters.py); animation clips live beside each mesh and
// are found by name suffix at runtime (donor fallback for anim-less rigs).
//
// Outfit colors are the type's SIGNATURE look. At spawn they get pulled
// toward a shared muted base by the round's color-similarity factor — the
// higher the round, the more the crowd converges into look-alikes.
struct FNPCTypeStyle
{
	const TCHAR* DisplayName;
	const TCHAR* MeshPath;
	FLinearColor OutfitPrimary;  // also drives the HUD swatch
	FLinearColor OutfitSecondary;
};

namespace NPCTypeStyles
{
	inline const FNPCTypeStyle& Get(ENPCType Type)
	{
		// Saturated signature palettes — the vibrancy lives here; round
		// similarity pulls them toward MutedBase later.
		static const FNPCTypeStyle Styles[NPCTypeCount] = {
			{ TEXT("Farmer"),     TEXT("/Game/LostInTheSauce/Characters/Farmer/Farmer"),         FLinearColor(0.25f, 0.42f, 0.72f), FLinearColor(0.82f, 0.68f, 0.32f) },
			{ TEXT("King"),       TEXT("/Game/LostInTheSauce/Characters/King/King"),             FLinearColor(0.92f, 0.65f, 0.08f), FLinearColor(0.18f, 0.24f, 0.68f) },
			{ TEXT("Witch"),      TEXT("/Game/LostInTheSauce/Characters/Witch/Witch"),           FLinearColor(0.50f, 0.13f, 0.72f), FLinearColor(0.20f, 0.14f, 0.10f) },
			{ TEXT("Adventurer"), TEXT("/Game/LostInTheSauce/Characters/Adventurer/Adventurer"), FLinearColor(0.30f, 0.48f, 0.16f), FLinearColor(0.48f, 0.30f, 0.14f) },
			{ TEXT("Squire"),     TEXT("/Game/LostInTheSauce/Characters/Medieval/Medieval"),     FLinearColor(0.82f, 0.34f, 0.10f), FLinearColor(0.88f, 0.80f, 0.58f) },
		};
		return Styles[FMath::Clamp(static_cast<int32>(Type), 0, NPCTypeCount - 1)];
	}

	// Shared muted base the whole crowd converges toward on later rounds.
	inline const FLinearColor MutedBase(0.38f, 0.33f, 0.27f);
}
