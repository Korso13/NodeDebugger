//JSON tools for NodeDebugger and more
//DO NOT INCLUDE in header files!! cpp only!
#pragma once

#include "W4Framework.h"
#include "W4JSON.h"
#include "W4JSONCommon.h"
#include "JsonHelpers.h"
#include "ND_Defines.h"
#include <regex>

W4_USE_UNSTRICT_INTERFACE

#define FIELD_DESERIALIZE_NE(x)                                                                                        \
{                                                                                                                   \
    if(j.contains(#x))                                                                                              \
    {                                                                                                               \
        adl_serializer<decltype(obj.x)>::from_json(j[#x], obj.x)     ;                                              \
    }                                                                                                             \
}

#define FIELD_SERIALIZE_MAP(x, defValue)                                                                            \
{                                                                                                                   \
    bool nonDefault = false;\
	for(auto& [key, item] : obj.x) if(item != defValue) {nonDefault = true; break;}									\
	if(!obj.x.empty() && nonDefault)                                                                                \
    {                                                                                                               \
        j[#x] = obj.x;																							\
    }                                                                                                             \
}

#define FIELD_SERIALIZE_VEC(x, defValue)                                                                             \
{                                                                                                                   \
    bool nonDefault = false;\
	for(auto& item : obj.x) if(item != defValue) {nonDefault = true; break;}										\
	if(!obj.x.empty() && nonDefault)                                                                                \
    {                                                                                                               \
        j[#x] = obj.x;																								\
    }                                                                                                             \
}

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
W4_NLOHMANN_JSON_SERIALIZE_ENUM(tweenTransformType, {
	{ tweenTransformType::MOVE,		"MOVE" },
	{ tweenTransformType::SCALE,	"SCALE" },
	{ tweenTransformType::ROTATE,	"ROTATE" }
});

//Import JS func to write into files (via save dialogue)
typedef void (*JSStringCallback) (void*, char*);

#ifdef __EMSCRIPTEN__
W4_JS_IMPORT
{
void jsWriteEvent(const char* data, const char* filename);
void jsReadImage(void* self, JSStringCallback callback);
void jsReadJson(void* self, JSStringCallback callback);
}
#else
void jsWriteEvent(const char* data, const char* filename) {}
void jsReadImage(void* self, JSStringCallback callback) {};
void jsReadJson(void* self, JSStringCallback callback) {};
#endif

namespace nlohmann
{
	//serializers and deserializers for tween settings container
	template<>
	struct adl_serializer<tweenDebugRecord>
	{
		static void from_json(const json& j, tweenDebugRecord& obj)
		{
			FIELD_DESERIALIZE_NE(tweenName);
			FIELD_DESERIALIZE_NE(isLoopedTween);
			FIELD_DESERIALIZE_NE(isRelativeValues);
			FIELD_DESERIALIZE_NE(isPercentRelativeValues);
			FIELD_DESERIALIZE_NE(transformType);
			FIELD_DESERIALIZE_NE(isWorldVec3);
			FIELD_DESERIALIZE_NE(tweenStart);
			FIELD_DESERIALIZE_NE(tweenPoints);
			FIELD_DESERIALIZE_NE(curveApogeePoints);
			FIELD_DESERIALIZE_NE(tweenDelays);
			FIELD_DESERIALIZE_NE(easingTypes);
		}

		//using BV3pair = std::pair<bool, vec3>;
		typedef std::pair<bool, vec3> BV3pair;
		static void to_json(json& j, const tweenDebugRecord& obj)
		{
			FIELD_SERIALIZE(tweenName);
			FIELD_SERIALIZE(isLoopedTween);
			FIELD_SERIALIZE(isRelativeValues);
			FIELD_SERIALIZE(isPercentRelativeValues);
			FIELD_SERIALIZE(transformType);
			FIELD_SERIALIZE(isWorldVec3);
			FIELD_SERIALIZE(tweenStart);
			FIELD_SERIALIZE(tweenPoints);
			FIELD_SERIALIZE_MAP(curveApogeePoints, BV3pair(false, vec3::zero));
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
		W4_LOG_DEBUG("Loaded [%d] tween samples to Tween Container", tweenContainer.tweenRecordsData.size());
		return tweenContainer;
	}

	static bool isDigit(char& symbol)
	{
		switch (symbol)
		{
		case('0'):
		case('1'):
		case('2'):
		case('3'):
		case('4'):
		case('5'):
		case('6'):
		case('7'):
		case('8'):
		case('9'):
			return true;
			break;
		default:
			break;
		}

		return false;
	}

	//json unflattener that doesn't crash over vec2 and vec3
	//Also limits floats precision by 4 digits if define JSON_FLOAT_OPTIMIZATION is enabled
	static std::string jsonUnflatten(nlohmann::json& inJson)
	{
		std::string jsonString = inJson.dump();

		bool isInVecStruct = false;
		bool isInVec4Struct = false;
		bool isInKeyName = false;
		bool countFloatAccuracy = false;
		int tabLevel = 0;
		int floatingPointDigits = 0;
		std::string vec4WFieldSubstr = "";
		size_t stringSize = jsonString.size();
		for (size_t i = 0; i < stringSize; i++)
		{
			if (i + 1 == stringSize)
			{
				jsonString.insert(i, "\n");
				break;
			}

			auto symb = jsonString[i];
			
			//detects key name or string. Requires no action until the next " (end of key or string)
			if (symb == '"')
			{
				isInKeyName = !isInKeyName;
			}
			if (isInKeyName)
				continue;

			//adds space after colon and inside vec structs
			if (symb == ':' || (isInVecStruct && symb == ','))
			{
				jsonString.insert(i + 1, " ");
				stringSize++;
			}

			//floating point accuracy reducer
			if (!countFloatAccuracy && symb == '.' && isDigit(jsonString[i + 1]))
			{
				floatingPointDigits = 0;
				countFloatAccuracy = true;
			}
			else if (countFloatAccuracy && isDigit(symb))
			{
				if (floatingPointDigits < JSON_FLOAT_ACCURACY)
				{
					floatingPointDigits++;
				}
				else
				{
					jsonString.erase(i, 1);
					stringSize--;
					i--;
					continue;
				}
			}
			else if (countFloatAccuracy)
			{
				countFloatAccuracy = false;
			}

			if (symb == '{' || symb == '[')
			{
				//flattener for vec3 structs
				if (!isInVecStruct && symb == '{' &&
					jsonString[i + 4] == ':' &&
					(jsonString[i + 2] == 'x' || jsonString[i + 2] == 'w'))
				{
					isInVecStruct = true;
					
					//fixed vec4 fields order begining
					if (jsonString[i + 2] == 'w')
					{
						isInVec4Struct = true;
						int stride = (jsonString.find_first_of(",", i + 2) + 1) - (i + 1);
						vec4WFieldSubstr = jsonString.substr(i + 1, stride);
						jsonString.erase(i + 1, stride);
						stringSize -= stride;
						vec4WFieldSubstr.pop_back();
						continue;
					}
				}
				//other cases
				else
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
			}

			if ((symb == ']' || symb == '}') && jsonString[i+1] != ',')
			{
				//vec4 fields order fix end
				if (isInVec4Struct)
				{
					isInVec4Struct = false;
					jsonString.insert(i, ", ");
					jsonString.insert(i + 2, vec4WFieldSubstr);
					stringSize += vec4WFieldSubstr.size() + 2;
					vec4WFieldSubstr.clear();
					continue;
				}

				if (isInVecStruct && symb == '}')
				{
					isInVecStruct = false;
				}
				else
				{
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
			}

			if ((symb == ']' || symb == '}') && jsonString[i+1] == ',')
			{
				//vec4 fields order fix end
				if (isInVec4Struct)
				{
					isInVec4Struct = false;
					jsonString.insert(i, ", ");
					jsonString.insert(i + 2, vec4WFieldSubstr);
					stringSize += vec4WFieldSubstr.size() + 2;
					vec4WFieldSubstr.clear();
					continue;
				}

				if (isInVecStruct && symb == '}')
				{
					isInVecStruct = false;
				}
				else
				{
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
			}

			if (symb == ',' && !isInVecStruct)
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

		std::string flatJson = jsonUnflatten(exportJson);
#ifndef FORCENODEDEBUG_OFF
		ImGui::SetClipboardText(flatJson.c_str());
#endif
		jsWriteEvent(flatJson.c_str(), "tweens.json");
	}
}

#undef FIELD_DESERIALIZE_NE