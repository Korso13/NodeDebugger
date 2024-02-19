//JSON tools for NodeDebugger and more
//DO NOT INCLUDE in header files!! cpp only!
#pragma once

#include "W4Framework.h"
#include "W4JSON.h"
#include "W4JSONCommon.h"
#include "JsonHelpers.h"
#include "ND_Defines.h"

W4_USE_UNSTRICT_INTERFACE

struct tweenRecordsContainer
{
	std::vector<tweenDebugRecord> tweenRecordsData;
};

//serializing Easing enum for JSON
W4_NLOHMANN_JSON_SERIALIZE_ENUM(w4::resources::Easing, {
	{ w4::resources::Easing::Linear,		"Linear" },
	{ w4::resources::Easing::QuadIn,		"QuadIn" },
	{ w4::resources::Easing::QuadOut,		"QuadOut" },
	{ w4::resources::Easing::QuadInOut,		"QuadInOut" },
	{ w4::resources::Easing::CubicIn,		"CubicIn" },
	{ w4::resources::Easing::CubicOut,		"CubicOut" },
	{ w4::resources::Easing::CubicInOut,	"CubicInOut" },
	{ w4::resources::Easing::SineIn,		"SineIn" },
	{ w4::resources::Easing::SineOut,		"SineOut" },
	{ w4::resources::Easing::SineInOut,		"SineInOut" },
	{ w4::resources::Easing::BounceIn,		"BounceIn" },
	{ w4::resources::Easing::BounceOut,		"BounceOut" },
	{ w4::resources::Easing::BounceInOut,	"BounceInOut" },
	{ w4::resources::Easing::BackIn,		"BackIn" },
	{ w4::resources::Easing::BackOut,		"BackOut" },
	{ w4::resources::Easing::BackInOut,		"BackInOut" },
	{ w4::resources::Easing::CircIn,		"CircIn" },
	{ w4::resources::Easing::CircOut,		"CircOut" },
	{ w4::resources::Easing::CircInOut,		"CircInOut" },
	{ w4::resources::Easing::ElasticIn,		"ElasticIn" },
	{ w4::resources::Easing::ElasticOut,	"ElasticOut" },
	{ w4::resources::Easing::ElasticInOut,	"ElasticInOut" },
	{ w4::resources::Easing::ExpoIn,		"ExpoIn" },
	{ w4::resources::Easing::ExpoOut,		"ExpoOut" },
	{ w4::resources::Easing::ExpoInOut,		"ExpoInOut" },
	{ w4::resources::Easing::QuartIn,		"QuartIn" },
	{ w4::resources::Easing::QuartOut,		"QuartOut" },
	{ w4::resources::Easing::QuartInOut,	"QuartInOut" },
	{ w4::resources::Easing::QuintIn,		"QuintIn" },
	{ w4::resources::Easing::QuintOut,		"QuintOut" },
	{ w4::resources::Easing::QuintInOut,	"QuintInOut" }
});

//Import JS func to write into files (via save dialogue)
#ifdef __EMSCRIPTEN__
W4_JS_IMPORT
{
void jsWriteEvent(const char* data, const char* filename);
}
#else
void jsWriteEvent(const char* data, const char* filename) {}
#endif

namespace nlohmann 
{
	//serializers and deserializers for tween settings container
	template<>
	struct adl_serializer<tweenDebugRecord>
	{
		static void from_json(const json& j, tweenDebugRecord& obj)
		{
			FIELD_DESERIALIZE(tweenName);
			FIELD_DESERIALIZE(isLoopedTween);
			FIELD_DESERIALIZE(isRelativeValues);
			FIELD_DESERIALIZE(isPercentRelativeValues);
			FIELD_DESERIALIZE(isMoveTween);
			FIELD_DESERIALIZE(isWorldVec3);
			FIELD_DESERIALIZE(tweenStart);
			FIELD_DESERIALIZE(tweenPoints);
			FIELD_DESERIALIZE(tweenDelays);
			FIELD_DESERIALIZE(easingTypes);
		}

		static void to_json(json& j, const tweenDebugRecord& obj)
		{
			FIELD_SERIALIZE(tweenName);
			FIELD_SERIALIZE(isLoopedTween);
			FIELD_SERIALIZE(isRelativeValues);
			FIELD_SERIALIZE(isPercentRelativeValues);
			FIELD_SERIALIZE(isMoveTween);
			FIELD_SERIALIZE(isWorldVec3);
			FIELD_SERIALIZE(tweenStart);
			FIELD_SERIALIZE(tweenPoints);
			FIELD_SERIALIZE(tweenDelays);
			FIELD_SERIALIZE(easingTypes);
		}

	};

	template<>
	struct adl_serializer<tweenRecordsContainer>
	{
		static void from_json(const json& j, tweenRecordsContainer& obj)
		{
			FIELD_DESERIALIZE(tweenRecordsData);
		}

		static void to_json(json& j, const tweenRecordsContainer& obj)
		{
			FIELD_SERIALIZE(tweenRecordsData);
		}
	};

	//loads tween settings container from tweens.json
	static tweenRecordsContainer loadTweens()
	{
		tweenRecordsContainer tweenContainer;
		auto file = w4::filesystem::open("config/tweens.json");
		auto json = nlohmann::json::parse(file->data(), file->data() + file->size());
		nlohmann::adl_serializer<tweenRecordsContainer>::from_json(json, tweenContainer);

		return tweenContainer;
	}

	//json unflattener that doesn't crash over vec2 and vec3
	static std::string jsonUnflatten(nlohmann::json& inJson)
	{
		std::string jsonString = inJson.dump();
		int tabLevel = 0;
		size_t stringSize = jsonString.size();
		for (size_t i = 0; i < stringSize; i++)
		{
			if (i + 1 == stringSize)
			{
				jsonString.insert(i, "\n");
				break;
			}

			auto symb = jsonString[i];
			if (symb == '{' || symb == '[')
			{
				tabLevel++;
				jsonString.insert(i+1, "\n");
				stringSize++;
				for (size_t j = 0; j < tabLevel; j++)
				{
					jsonString.insert(i+2+j, "\t");
					stringSize++;
				}
			}

			if ((symb == ']' || symb == '}') && jsonString[i+1] != ',')
			{
				//jsonString.erase(i - 1, 1);
				tabLevel--; 
				jsonString.insert(i, "\n");
				stringSize++;
				for (size_t j = 0; j < tabLevel; j++)
				{
					jsonString.insert(i + 1 + j, "\t");
					stringSize++;
				}
				i += tabLevel + 1;
			}

			if ((symb == ']' || symb == '}') && jsonString[i+1] == ',')
			{
				//jsonString.erase(i - 1, 1);
				tabLevel--;
				jsonString.insert(i, "\n");
				stringSize++;
				for (size_t j = 0; j < tabLevel; j++)
				{
					jsonString.insert(i+1+j, "\t");
					stringSize++;
				}
				i += tabLevel + 1;
			}

			if (symb == ',' /*&& jsonString[i + 1] != '"'*/)
			{
				jsonString.insert(i+1, "\n");
				stringSize++;
				for (size_t j = 0; j < tabLevel; j++)
				{
					jsonString.insert(i + 2 + j, "\t");
					stringSize++;
				}
			}

			//info("%c", symb);
		}

		return jsonString;
	}

	//copies tweens settings json to clipboard, console and allows saving via save file dialogue
	static void exportTweens(tweenRecordsContainer& containerData)
	{
		nlohmann::json exportJson;
		nlohmann::adl_serializer<tweenRecordsContainer>::to_json(exportJson, containerData);
		
		//exportJson.unflatten(); doesn't work due to values not being JSON primitives. Needs custom unflattener
		std::string flatJson = jsonUnflatten(exportJson);
#ifndef FORCENODEDEBUG_OFF
		ImGui::SetClipboardText(flatJson.c_str());
#endif
		jsWriteEvent(flatJson.c_str(), "tweens.json");
		return; 

		//writing directly into files doesn't work yet((
		//auto file = w4::filesystem::open("config/tweens.json", FileMode::Write);
		//unsigned char jsonData[exportJson.dump().size()];
		//for (size_t i = 0; i < exportJson.dump().size(); i++)
		//{
		//	jsonData[i] = exportJson.dump().data()[i];
		//}
		//
		//file->write(jsonData, exportJson.dump().size());
	}
}