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
	(CosmicSword)
	(Bow)
	(Potion)
	(Crown)
	(Feather)
	(ChampionArmor)
	(ChampionSword)
	(ChampionSet)
	(GreenArmor)
	(GreenSword)
	(GreenSet)
	(x5)
	(x10)
	(x50)
	(Heart)
	(Trident)
	(Lightning)
	(FireUlt)
	(ElectricUlt)
	(Coins)
	(ExpSpheres)
	(CoinsAndSpheres)
	(Emerald)
	(Ruby)
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