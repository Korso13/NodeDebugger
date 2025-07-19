//Contains Macro definitions used for Spawnable structs, enums. 
//Allows them to be serialized, deserialized, rendered in ImGui ui
#pragma once

#ifdef DEBUG
#include "common/nodeDebugger/ND_Defines.h"
#include "common/nodeDebugger/ND_Utilities.h"
#endif

#include "external/boost/preprocessor/seq/for_each.hpp"
#include "external/boost/preprocessor/tuple/elem.hpp"
#include "external/boost/preprocessor/tuple/eat.hpp"

//interface struct for spawnables
struct SpawnableStr
{
	//override this method to expose struct variables to ImGui (consider using IMGUI_AUTO macro passing your field to it)
	//It's good practice to wrap all code of your onGui override in this structure:
	//if(ImGui::TreeNode(label)){ /*your_imgui_calls_here*/ ImGui::TreePop();}
	virtual bool onGui(const char* label) { return false; };
};

//========================================
// Enum declaration & serialization macros
//========================================

#define ENUM_TO_STRING(r, data, elem)    \
    case data::elem : return std::string(BOOST_PP_STRINGIZE(elem));

//SERIALIZE_ENUM and AUTO_SERIALIZE_ENUM_CLASS helper Macro
#define X_DEFINE_ENUM_SERIALIZER(r, data, elem) \
	{ data::elem, BOOST_PP_STRINGIZE(elem)},

//AUTO_SERIALIZE_ENUM_CLASS helper Macro
#define X_DEFINE_ENUM_CLASS_FIELD(r, data, elem) elem ,

template<typename T>
inline static const char* enumToStr(const T& value)
{
	int val = CastI(value);
	return nlohmann::adl_serializer<T>::getValues(val)->second;
}

//Enums must be serialized before exposing them to ImGui or mapObjects.json! 
//Syntax sample:
//SERIALIZE_ENUM(ViewDirection, (LEFT) (RIGHT));
#define SERIALIZE_ENUM(name, enumerators)                                \
    W4_NLOHMANN_JSON_SERIALIZE_ENUM(name,{                                    \
            BOOST_PP_SEQ_FOR_EACH(                                            \
                X_DEFINE_ENUM_SERIALIZER,                                     \
                name,                                                         \
                enumerators                                                   \
        )                                                                     \
    });

//Macro for generating serialized enum class
//Syntax sample:
//MAKE_SERIALIZED_ENUM_CLASS(SPK_MATERIAL, (ADDALPHA) (ALPHA) (ADD));
#define MAKE_SERIALIZED_ENUM_CLASS(name, enumerators)                          \
	enum class name															  \
	{																		  \
		BOOST_PP_SEQ_FOR_EACH(												  \
           X_DEFINE_ENUM_CLASS_FIELD,										  \
           name,															  \
           enumerators														  \
        )                                                                     \
		SIZE																  \
	};																		  \
																			  \
    W4_NLOHMANN_JSON_SERIALIZE_ENUM(name,{                                    \
            BOOST_PP_SEQ_FOR_EACH(                                            \
                X_DEFINE_ENUM_SERIALIZER,                                     \
                name,														  \
                enumerators                                                   \
        )                                                                     \
    });																		  

//===========================================
// Struct declaration & serialization macros
//===========================================

//BOOST macro needs special field de/serializers + prevents errors when adding new fields to the MapObjectsData struct (ignores if json record doesn't have the field)
#define JSTRUCT_FIELD_DESERIALIZE(name, field)                                                                                     \
{                                                                                                                   \
    if(j.contains(name))                                                                                              \
    {                                                                                                               \
        adl_serializer<decltype(obj.field)>::from_json(j[name], obj.field)     ;                                              \
    }                                                                                                             \
}
#define JSTRUCT_FIELD_SERIALIZE(name, field, default_val)                                                            \
{                                                                                                                   \
   if(obj.field != default_val)																						\
		j[name] = obj.field;                                                                                         \
}
  																					
//type - field name - default  tuple unpackers. state - element of sequence of tuples
#define JSTRUCT_F_TYPE(state) BOOST_PP_TUPLE_ELEM(3, 0, state)
#define JSTRUCT_F_NAME(state) BOOST_PP_TUPLE_ELEM(3, 1, state)
#define JSTRUCT_F_NAME_STR(state) BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(3, 1, state))//we can't use # and ## inside BOOST preprocessors
#define JSTRUCT_F_DEFV(state) BOOST_PP_TUPLE_ELEM(3, 2, state)

//Macros called by BOOST_PP_SEQ_FOR_EACH (goes over each item (tuple of type-name-default value) in sequence passed to JSTRUCT_DEFINITION):
//ADD_STRCT_FIELD_MACRO - adds MapObjectsData fields and defines their default values
//ADD_STRCT_ONGUID_MACRO - implements onGui calls for each field
//ADD_STRCT_DESERIALIZE_MACRO and ADD_STRCT_SERIALIZE_MACRO - implement de/serialization of MapObjectsData fields
#define ADD_STRCT_FIELD_MACRO(i, data, field_tuple) JSTRUCT_F_TYPE(field_tuple) JSTRUCT_F_NAME(field_tuple) = JSTRUCT_F_DEFV(field_tuple);
#ifdef DEBUG
	#define ADD_STRCT_ONGUID_MACRO(i, data, field_tuple) isChanged = isChanged || NDGui::Auto(JSTRUCT_F_NAME_STR(field_tuple), JSTRUCT_F_NAME(field_tuple));
#endif
#define ADD_STRCT_DESERIALIZE_MACRO(i, data, field_tuple) JSTRUCT_FIELD_DESERIALIZE(JSTRUCT_F_NAME_STR(field_tuple), JSTRUCT_F_NAME(field_tuple));
#define ADD_STRCT_SERIALIZE_MACRO(i, data, field_tuple) JSTRUCT_FIELD_SERIALIZE(JSTRUCT_F_NAME_STR(field_tuple), JSTRUCT_F_NAME(field_tuple), JSTRUCT_F_DEFV(field_tuple));

//used for overloading operator== in structs 
#define JSTRUCT_COMPARATOR_LINE(field) field == rhValue.field && 
#define ADD_COMPARATOR_LINE_MACRO(i, data, field_tuple) JSTRUCT_COMPARATOR_LINE(JSTRUCT_F_NAME(field_tuple))
#define JSTRUCT_DOUBLE_COMPARATOR_LINE(field) lhValue.field == rhValue.field && 
#define ADD_DOUBLE_COMPARATOR_LINE_MACRO(i, data, field_tuple) JSTRUCT_DOUBLE_COMPARATOR_LINE(JSTRUCT_F_NAME(field_tuple))

//Need conditional definition due to use of ImGui inside the macro :(
#ifdef DEBUG
//Macro for automatically building SpawnableStr child along with onGui and JSON De/Serializators definitions
#define JSTRUCT_DEFINITION(name, fields) \
struct name : public SpawnableStr \
{\
	BOOST_PP_SEQ_FOR_EACH(ADD_STRCT_FIELD_MACRO, "", fields) \
\
	bool onGui(const char* label) override \
	{ \
		bool isChanged = false; \
		if (ImGui::TreeNode(label)) \
		{ \
			BOOST_PP_SEQ_FOR_EACH(ADD_STRCT_ONGUID_MACRO, "", fields) \
			ImGui::TreePop(); \
		} \
		return isChanged; \
	} \
	bool operator==(const name& rhValue) \
	{ \
		return (BOOST_PP_SEQ_FOR_EACH(ADD_COMPARATOR_LINE_MACRO, "", fields) \
				true);\
	} \
	bool operator!=(const name& rhValue) { return !(*this == rhValue); }\
}; \
static bool operator==(const name& lhValue, const name& rhValue) \
{ \
	return (BOOST_PP_SEQ_FOR_EACH(ADD_DOUBLE_COMPARATOR_LINE_MACRO, "", fields) \
			true);\
} \
\
namespace nlohmann {\
	template<> \
	struct adl_serializer<name> \
	{ \
		static void from_json(const json& j, name& obj) \
		{ \
			BOOST_PP_SEQ_FOR_EACH(ADD_STRCT_DESERIALIZE_MACRO, "", fields) \
		} \
		static void to_json(json& j, const name& obj) \
		{ \
			BOOST_PP_SEQ_FOR_EACH(ADD_STRCT_SERIALIZE_MACRO, "", fields) \
		} \
	};\
}
#else
#define JSTRUCT_DEFINITION(name, fields) \
struct name : public SpawnableStr \
{\
	BOOST_PP_SEQ_FOR_EACH(ADD_STRCT_FIELD_MACRO, data, fields) \
	bool operator==(const name& rhValue) \
	{ \
		return (BOOST_PP_SEQ_FOR_EACH(ADD_COMPARATOR_LINE_MACRO, "", fields) \
				true);\
	} \
	bool operator!=(const name& rhValue) { return !(*this == rhValue); }\
};\
static bool operator==(const name& lhValue, const name& rhValue) \
{ \
	return (BOOST_PP_SEQ_FOR_EACH(ADD_DOUBLE_COMPARATOR_LINE_MACRO, "", fields) \
			true);\
} \
\
namespace nlohmann {\
	template<> \
	struct adl_serializer<name> \
	{ \
		static void from_json(const json& j, name& obj) \
		{ \
			BOOST_PP_SEQ_FOR_EACH(ADD_STRCT_DESERIALIZE_MACRO, "", fields) \
		} \
		static void to_json(json& j, const name& obj) \
		{ \
			BOOST_PP_SEQ_FOR_EACH(ADD_STRCT_SERIALIZE_MACRO, "", fields) \
		} \
	};\
}
#endif


//==============================================
//MapObjectsData dedicated struct factory macro
//==============================================
#ifdef DEBUG
#define MOD_DEFINITION(fields) \
struct MapObjectsData : public SpawnableStr \
{\
	std::string className = "CLASS_NAME"; \
	BOOST_PP_SEQ_FOR_EACH(ADD_STRCT_FIELD_MACRO, "", fields) \
	std::vector<sptr<MapObjectsData>> childObjects;\
	std::vector<sptr<MapMeshData>> childMeshes;\
\
	bool onGui(const char* label) override \
	{ \
		bool isChanged = false; \
		if (ImGui::TreeNode(label)) \
		{ \
			BOOST_PP_SEQ_FOR_EACH(ADD_STRCT_ONGUID_MACRO, "", fields) \
			ImGui::TreePop(); \
		} \
		return isChanged; \
	} \
	bool operator==(const MapObjectsData& rhValue) \
	{ \
		return (BOOST_PP_SEQ_FOR_EACH(ADD_COMPARATOR_LINE_MACRO, "", fields) \
				className == rhValue.className);\
	} \
	bool operator!=(const MapObjectsData& rhValue) { return !(*this == rhValue); }\
}; \
\
namespace nlohmann {\
	template<> \
	struct adl_serializer<MapObjectsData> \
	{ \
		static void from_json(const json& j, MapObjectsData& obj) \
		{ \
			JSTRUCT_FIELD_DESERIALIZE("className", className); \
			BOOST_PP_SEQ_FOR_EACH(ADD_STRCT_DESERIALIZE_MACRO, "", fields) \
			if(j.contains("childObjects"))  \
				 adl_serializer<decltype(obj.childObjects)>::from_json(j["childObjects"], obj.childObjects); \
			if(j.contains("childMeshes"))  \
				 adl_serializer<decltype(obj.childMeshes)>::from_json(j["childMeshes"], obj.childMeshes); \
		} \
		static void to_json(json& j, const MapObjectsData& obj) \
		{ \
			JSTRUCT_FIELD_SERIALIZE("className", className, "CLASS_NAME"); \
			BOOST_PP_SEQ_FOR_EACH(ADD_STRCT_SERIALIZE_MACRO, "", fields) \
			if(!obj.childObjects.empty()) j["childObjects"] = obj.childObjects; \
			if(!obj.childMeshes.empty()) j["childMeshes"] = obj.childMeshes; \
		} \
	};\
}
#else
#define MOD_DEFINITION(fields) \
struct MapObjectsData : public SpawnableStr \
{\
	std::string className = "CLASS_NAME"; \
	BOOST_PP_SEQ_FOR_EACH(ADD_STRCT_FIELD_MACRO, data, fields) \
	std::vector<sptr<MapObjectsData>> childObjects;\
	std::vector<sptr<MapMeshData>> childMeshes;\
	bool operator==(const MapObjectsData& rhValue) \
	{ \
		return (BOOST_PP_SEQ_FOR_EACH(ADD_COMPARATOR_LINE_MACRO, "", fields) \
				className == rhValue.className);\
	} \
	bool operator!=(const MapObjectsData& rhValue) { return !(*this == rhValue); }\
};\
\
namespace nlohmann {\
	template<> \
	struct adl_serializer<MapObjectsData> \
	{ \
		static void from_json(const json& j, MapObjectsData& obj) \
		{ \
			JSTRUCT_FIELD_DESERIALIZE("className", className); \
			BOOST_PP_SEQ_FOR_EACH(ADD_STRCT_DESERIALIZE_MACRO, "", fields) \
			if(j.contains("childObjects"))  \
				 adl_serializer<decltype(obj.childObjects)>::from_json(j["childObjects"], obj.childObjects); \
			if(j.contains("childMeshes"))  \
				 adl_serializer<decltype(obj.childMeshes)>::from_json(j["childMeshes"], obj.childMeshes); \
		} \
		static void to_json(json& j, const MapObjectsData& obj) \
		{ \
			JSTRUCT_FIELD_SERIALIZE("className", className, "CLASS_NAME"); \
			BOOST_PP_SEQ_FOR_EACH(ADD_STRCT_SERIALIZE_MACRO, "", fields) \
			if(!obj.childObjects.empty()) j["childObjects"] = obj.childObjects; \
			if(!obj.childMeshes.empty()) j["childMeshes"] = obj.childMeshes; \
		} \
	};\
}
#endif