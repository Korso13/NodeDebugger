#pragma once

#include "SpawnerStructsImpl.h"  //Implements  MAKE_SERIALIZED_ENUM_CLASS and SERIALIZE_ENUM macros
#include "settings/InGameTypes.h" //Common header for enums used with SERIALIZE_ENUM
#include "common/CommonTypes.h" //Common header for enums used with SERIALIZE_ENUM
#include "common/MonospaceDigitsPlane.h"

//enum class serialisations for imgui or json export/import

//enum class wrappers for SPK enums
MAKE_SERIALIZED_ENUM_CLASS(SPK_MATERIAL,
	(ADDALPHA)
	(ALPHA)
	(ADD)
);

MAKE_SERIALIZED_ENUM_CLASS(SPK_ORIENTATION,
	(CAMERA_PLANE_ALIGNED)
	(CAMERA_POINT_ALIGNED)
	(DIRECTION_ALIGNED)
	(AROUND_AXIS)
	(TOWARDS_POINT)
	(FIXED_ORIENTATION)
);

//Bonuses enum
MAKE_SERIALIZED_ENUM_CLASS(BonusType,
	(Starter)
	(Astharot)
	(AngelSword)
	(AngelicSword)
	(AngelicArmor)
	(AngelicHelmet)
	(AngelicSet)
	(StandartSet)
	(StandartSword)
	(StandartArmor)
	(FireSword)
	(IronSword)
	(SamuraiSword)
	(CosmicSword)
	(Bow)
	(Potion)
	(Crown)
	(ChampionArmor)
	(ChampionSword)
	(ChampionHelmet)
	(ChampionSet)
	(GreenArmor)
	(GreenSword)
	(GreenHelmet)
	(NephriteHelmet)
	(GreenSet)
	(GoldenSword)
	(GoldenAxe)
	(Scythe)
	(FireUlt)
	(ElectricUlt)
	(Emerald)
	(Ruby)
	(Key)
	(AuroraBuff)
	(Dash)
	(Immortal)


	(Coins)
	(ExpSpheres)
	(CoinsAndSpheres)
	(NONE)
);

MAKE_SERIALIZED_ENUM_CLASS(UiAlignment,
	(LEFT)
	(RIGHT)
	(CENTER)
	(CENTER_STRETCHED)
	(TOP)
	(BOTTOM)
);

MAKE_SERIALIZED_ENUM_CLASS(UiStretch,
	(HORIZONTAL)
	(VERTICAL)
	(FULLSCREEN)
	(NONE)
)

SERIALIZE_ENUM(ViewDirection,
	(LEFT)
	(RIGHT)
);

SERIALIZE_ENUM(TEXT_ALIGN,
	(LEFT)
	(CENTER)
	(RIGHT)
);

SERIALIZE_ENUM(Aligning,
	(LEFT)
	(CENTER)
	(RIGHT)
);

MAKE_SERIALIZED_ENUM_CLASS(BorderType,
	(Inner)
	(Outer)
	(TriggerZone)
);

MAKE_SERIALIZED_ENUM_CLASS(GAL_ATTACK_TYPE,
	(DEFAULT) //standard attack
	(LAST_USED) //last used attack or ultimate
	(LAST_LOOT_DEFINED) //last attack type set via loot
	(LAST_REGULAR_USED) //last used non-ultimate attack
	(LAST_ULT_USED) //last used ultimate attack
	(ULT_ORANGE)
	(ULT_PINK)
	(ULT_ORANGE_OR_PINK)
	(ULT_RANDOM)
	(ULT_BLUE)
	(ULT_SCYTHE)
	(ATTACK_SIDE)
	(ATTACK_DOUBLE)
	(ATTACK_COMBO)
	(ATTACK_FLYING)
	(ATTACK_WHIRLWIND)
	(ATTACK_SPELL)
	(ATTACK_RAM)
);