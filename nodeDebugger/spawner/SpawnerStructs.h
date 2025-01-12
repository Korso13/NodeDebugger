#pragma once

#include "W4Framework.h"
#include "W4JSON.h"
#include "JsonHelpers.h"

W4_USE_UNSTRICT_INTERFACE

#include "SpawnerStructsImpl.h" //Implements MOD_DEFINITION
#include "SpawnerEnums.h" 

//interface struct for spawnables
struct SpawnableStr
{
	//override this method to expose struct variables to ImGui (consider using IMGUI_AUTO macro passing your field to it)
	//It's good practice to wrap all code of your onGui override in this structure:
	//if(ImGui::TreeNode(label)){ /*your_imgui_calls_here*/ ImGui::TreePop();}
	virtual bool onGui(const char* label) { return false; };
};

//MapObjectsData struct constructor. 
//It defines the struct, onGui method for it, as well as JSON serialization methods
//Add new fields via syntax: ((type, field_name, defaultValue))
MOD_DEFINITION(
	((std::string, objName, "TEMPLATE_RECORD")) //DO NOT REMOVE OR RENAME!
	((std::string, node, ""))
	((std::string, clickEvent, "none"))
	((BonusType, bonus_inside_1, BonusType::NONE))
	((BonusType, bonus_inside_2, BonusType::NONE))
	((bool, isBoss, false))
	((int, room, -1))
	((int, level, 0))
	((bool, isBonusMultiplies, false))
	((vec3, pos, vec3::zero))
	((bool, isNeedToTurnLeft, false))
	((bool, hasTwoActivatePoints, false))
	((vec3, interactPos_R, vec3(666.f)))
	((vec3, interactPos_L, vec3(666.f)))
	((std::vector<SPK_MATERIAL>, SPARK_Materials, std::vector<SPK_MATERIAL>()))
);

//Definitions of other structs used by Spawnables (can be rendered in ImGui, exported/imported via JSON)
JSTRUCT_DEFINITION(MapMeshData,
	((std::string, meshName, ""))
	((std::string, material, "sprite"))
	((std::string, node, ""))
	((int32_t, renderOrder, 1))
	((bool, useWorldPos, true))
	((vec3, position_world, vec3::zero))
	((vec3, position_local, vec3::zero))
	((vec3, scale_local, vec3::one))
	((vec3, rotation_local, vec3::zero))
);

//Just MapObjectsContainer definition and deserialization
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