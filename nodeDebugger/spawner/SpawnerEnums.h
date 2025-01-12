#pragma once

#include "SpawnerStructsImpl.h"  //Implements  MAKE_SERIALIZED_ENUM_CLASS and SERIALIZE_ENUM macros
#include "settings/InGameTypes.h" //Common header for enums used with SERIALIZE_ENUM

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
	(AngelSword)
	(AngelicSword)
	(AngelicArmor)
	(AngelicHelmet)
	(StandartSet)
	(StandartSword)
	(StandartArmor)
	(FireSword)
	(CosmicSword)
	(Bow)
	(Potion)
	(Crown)
	(Feather)
	(ChampionArmor)
	(ChampionSword)
	(GreenArmor)
	(GreenSword)
	(FireUlt)
	(ElectricUlt)
	(Coins)
	(ExpSpheres)
	(CoinsAndSpheres)
);

SERIALIZE_ENUM(ViewDirection,
	(LEFT)
	(RIGHT)
);