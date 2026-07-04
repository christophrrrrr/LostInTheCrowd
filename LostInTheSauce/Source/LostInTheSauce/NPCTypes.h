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
	Knight,

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
	// The pack rigs share slot naming (Skin/Hair/Metal/...) that the crowd
	// tint system maps; foreign models (Mixamo knight) keep their textures.
	bool bUseCrowdTint = true;
};

namespace NPCTypeStyles
{
	inline const FNPCTypeStyle& Get(ENPCType Type)
	{
		static const FNPCTypeStyle Styles[NPCTypeCount] = {
			{ TEXT("Farmer"),     TEXT("/Game/LostInTheSauce/Characters/Farmer/Farmer"),         FLinearColor(0.35f, 0.45f, 0.60f), FLinearColor(0.75f, 0.65f, 0.40f) },
			{ TEXT("King"),       TEXT("/Game/LostInTheSauce/Characters/King/King"),             FLinearColor(0.85f, 0.65f, 0.15f), FLinearColor(0.25f, 0.30f, 0.55f) },
			{ TEXT("Witch"),      TEXT("/Game/LostInTheSauce/Characters/Witch/Witch"),           FLinearColor(0.45f, 0.20f, 0.60f), FLinearColor(0.25f, 0.18f, 0.12f) },
			{ TEXT("Adventurer"), TEXT("/Game/LostInTheSauce/Characters/Adventurer/Adventurer"), FLinearColor(0.40f, 0.30f, 0.20f), FLinearColor(0.35f, 0.38f, 0.20f) },
			{ TEXT("Squire"),     TEXT("/Game/LostInTheSauce/Characters/Medieval/Medieval"),     FLinearColor(0.70f, 0.40f, 0.20f), FLinearColor(0.80f, 0.75f, 0.60f) },
			{ TEXT("Knight"),     TEXT("/Game/LostInTheSauce/Characters/Knight/Knight1"),        FLinearColor(0.55f, 0.58f, 0.65f), FLinearColor(0.30f, 0.32f, 0.36f), false },
		};
		return Styles[FMath::Clamp(static_cast<int32>(Type), 0, NPCTypeCount - 1)];
	}

	// Shared muted base the whole crowd converges toward on later rounds.
	inline const FLinearColor MutedBase(0.38f, 0.33f, 0.27f);
}
