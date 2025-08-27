#pragma once

#include "W4Framework.h"
#include "W4JSON.h"
#include "JsonHelpers.h"
#include "../CustomAutos.h"

W4_USE_UNSTRICT_INTERFACE

#include "SpawnerStructsImpl.h" //Implements MOD_DEFINITION
#include "SpawnerEnums.h" 

//we can't use templates with 2+ arguements in macro, hence this little trick:
template<typename T>
using settingUMap = std::unordered_map<std::string, T>;
template<typename T>
using settingMap = std::map<std::string, T>;

//JSTRUCT_DEFINITION macro defines the struct, onGui method for it, as well as JSON serialization methods
//Add new fields via syntax: ((type, field_name, defaultValue))

//struct for various objects' settings in MapObjectData
JSTRUCT_DEFINITION(SettingsStuct,
	((settingUMap<bool>,		flags,		settingUMap<bool>()))
	((settingUMap<int>,			integers,	settingUMap<int>()))
	((settingUMap<float>,		floats,		settingUMap<float>()))
	((settingUMap<std::string>,	strs,		settingUMap<std::string>()))
	((settingUMap<vec2>,		vec2s,		settingUMap<vec2>()))
	((settingUMap<vec3>,		vec3s,		settingUMap<vec3>()))
	((settingUMap<vec4>,		vec4s,		settingUMap<vec4>()))
);

JSTRUCT_DEFINITION(ExtraLoot,
	((int, bonus_count, 0))
	((BonusType, bonus_type, BonusType::NONE))
	((std::string, bonus_mesh, "none"))
);

JSTRUCT_DEFINITION(TextSettings,
	((std::string, fontName, "marvin"))
	((vec4, color, vec4(1.f)))
	((int, renderOrder, 100))
	((vec3, position_local, vec3(0.f, 0.f, -0.002)))
	((vec3, scale_local, vec3::one))
	((int, lineSize, 32))
	((vec3, lineOffset, vec3::down))
	((TEXT_ALIGN, align, TEXT_ALIGN::CENTER))
);
//NOTE: position_local - small negative z required because of stupid TextSdfNode implementation in the engine =\

JSTRUCT_DEFINITION(TextNodeSettings,
	((std::string, text, ""))
	((bool, usesTranslations, false))
	((bool, needsStroke, false))
	((bool, useFontColor, false))
	((TextSettings, textSetting, TextSettings()))
	((float, strokeSize, 1.0f))
	((float, strokeEdgeCenter, 0.34f))
	((float, strokeSmoothing, 0.37f))
	((vec4, strokeColor, vec4(1.f)))
);

//Struct for Mesh Spawner
JSTRUCT_DEFINITION(MapMeshData,
	((std::string, className, "CLASS_NAME"))
	((std::string, meshName, ""))
	((std::string, meshId, ""))
	((std::string, material, "sprite"))
	((std::string, node, ""))
	((int32_t, renderOrder, 1))
	((bool, isChild, false))
	((bool, isNeedToTurnLeft, false))
	((bool, useWorldPos, true))
	((vec3, position_world, vec3::zero))
	((vec3, position_local, vec3::zero))
	((vec3, scale_local, vec3::one))
	((vec3, rotation_local, vec3::zero))
);

//MapObjectsData struct constructor. 
MOD_DEFINITION(
	((std::string, objId, "")) //DO NOT REMOVE OR RENAME!
	((std::string, objName, "TEMPLATE_RECORD")) //DO NOT REMOVE OR RENAME!
	((std::string, node, "")) //DO NOT REMOVE OR RENAME!
	((bool, isChildObj, false)) //DO NOT REMOVE OR RENAME!
	((std::string, clickEvent, "none"))
	((BonusType, bonus_inside_1, BonusType::NONE))
	((BonusType, bonus_inside_2, BonusType::NONE))
	((std::vector<ExtraLoot>, extra_loot, std::vector<ExtraLoot>()))
	((bool, isBoss, false))
	((int, room, -1))
	((int, level, 0))
	((bool, isBonusMultiplies, false))
	((vec3, pos, vec3::zero))
	((vec3, local_pos, vec3::zero))
	((bool, isNeedToTurnLeft, false))
	((bool, hasTwoActivatePoints, false))
	((vec3, interactPos_R, vec3(666.f)))
	((vec3, interactPos_L, vec3(666.f)))
	((SettingsStuct, settings, SettingsStuct()))
	((TextNodeSettings, textNodeSetting, TextNodeSettings()))
	((std::vector<SPK_MATERIAL>, SPARK_Materials, std::vector<SPK_MATERIAL>()))
);

//MapObjectsContainer definition and deserialization for Spawner Factory
struct MapObjectsContainer {
	std::vector<MapObjectsData> mapObjectsData;
	std::vector<MapMeshData> mapMeshData;
};

namespace nlohmann {
	template<>
	struct adl_serializer<MapObjectsContainer>
	{
		static void from_json(const json& j, MapObjectsContainer& obj)
		{
			FIELD_DESERIALIZE(mapObjectsData);
			FIELD_DESERIALIZE(mapMeshData);
		}
		static void to_json(json& j, const MapObjectsContainer& obj)
		{
			FIELD_SERIALIZE(mapObjectsData);
			FIELD_SERIALIZE(mapMeshData);
		}
	};
}