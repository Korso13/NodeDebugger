#pragma once
#include "W4Framework.h"
#include "common/Defines.h"
#include "FontConf.h"

W4_USE_UNSTRICT_INTERFACE

class RusFix
{
public:
	static RusFix& instance()
	{
		static RusFix instance;
		return instance;
	}

	void init(const std::string& configPath, std::vector<std::string> fonts)
	{
		m_configPath = configPath;
		for (auto& font : fonts)
		{
			auto configPathWithName = m_configPath + font;
			auto file = w4::filesystem::open(configPathWithName + m_fileExtension);
			nlohmann::json json = nlohmann::json::parse(file->data(), file->data() + file->size());
			json = json.at("characters");
			m_fonts[font] = json.get<FontConf>();

		}
	}

	const FontConf& getFontConfig(const std::string& fontName)
	{ 
		return m_fonts[fontName];
	}

private:
	std::unordered_map<std::string, FontConf> m_fonts;
	std::string m_fileExtension = ".json";
	std::string m_configPath = "fonts/";
};
