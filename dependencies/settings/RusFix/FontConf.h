#pragma once
#include <map>
#include "../../common/Defines.h"
#include "../../common/CommonFunctions.h"
#include "W4Framework.h"
#include "W4JSON.h"
#include "JsonHelpers.h"

struct FontConf
{
	struct Letter
	{
		int x;
		int y;
		int width;
		int height;
		int originX;
		int originY;
		int advance;
	};

	std::unordered_map<wchar_t, Letter> characters;
};

namespace nlohmann
{
template<>
struct adl_serializer<FontConf::Letter>
{
	static void from_json(const json& j, FontConf::Letter& obj)
	{
		FIELD_DESERIALIZE(x);
		FIELD_DESERIALIZE(y);
		FIELD_DESERIALIZE(width);
		FIELD_DESERIALIZE(height);
		FIELD_DESERIALIZE(originX);
		FIELD_DESERIALIZE(originY);
		FIELD_DESERIALIZE(advance);
	}
};

template <>
struct adl_serializer<FontConf>
{
	static void from_json(const json& j, FontConf& obj)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> STRtoWSTR;
			
		for (auto& [key, value] : j.items())
		{
			std::wstring wstr = STRtoWSTR.from_bytes(key);
			obj.characters[wstr[0]] = value;
			//W4_LOG_INFO("key: %s", wstr.c_str());
		}
	}
};
}  // namespace nlohmann