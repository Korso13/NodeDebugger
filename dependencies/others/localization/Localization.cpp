#include "Localization.h"
#include "ui/UiText.h"

Localization::Localization() {}
Localization::~Localization() {}

static SettingsContainer<std::string> getTranslation(
	const std::string& configPath,
	const std::string& fileExt,
	const std::string& langKey
)
{
	SettingsContainer<std::string> ret;

	std::string configPathWithName = configPath + langKey;
	if (!w4::filesystem::isPathExists(configPathWithName + fileExt))
	{
		auto fullPath = configPathWithName + fileExt;
		info("[getTranslation] No localization file exists for %s with path %s! Returning empty!",
			langKey.c_str(), fullPath.c_str());
		return ret;
	}
	sptr<w4::filesystem::Stream> file = w4::filesystem::open(configPathWithName + fileExt);
	nlohmann::json json = nlohmann::json::parse(file->data(), file->data() + file->size());
	ret.init(json);

	return ret;
}

static const std::vector<std::pair<Lang, std::string>>& getLangMap()
{	
	static const std::vector<std::pair<Lang, std::string>> map
	{
		{Lang::CZ, "cs"},
		{Lang::DE, "de"},
		{Lang::CA, "ca"},
		{Lang::ES, "es"},
		{Lang::FR, "fr"},
		{Lang::IT, "it"},
		{Lang::NL, "nl"},
		{Lang::PL, "pl"},
		{Lang::PT, "pt"},
		{Lang::EN, "en"},
	};

	return map;
}

Lang Localization::getLang(const std::string& key)
{
	auto& map = getLangMap();
	
	auto it = std::find_if(
		map.begin(), map.end(),
		[&key](const auto& kv)
		{
			return kv.second == key;
		}
	);

	if(it == map.end())
	{
		W4_LOG_ERROR(
			"Localization::getLang error. lang \"%s\" doesn't exist.", 
			key.c_str()
		);

		return Lang::EN;
	}

	return it->first;
}

const std::string& Localization::getLang(Lang lang)
{
	auto& map = getLangMap();
	
	auto it = std::find_if(
		map.begin(), map.end(),
		[&lang](const auto& kv)
		{
			return kv.first == lang;
		}
	);

	if(it == map.end())
	{
		W4_LOG_ERROR(
			"Localization::getLang error. lang enum \"%d\" doesn't exist.", 
			lang
		);

		return "en";
	}

	return it->second;
}

void Localization::loadConfig()
{
	if (m_configPath.empty()) m_configPath = "config/localization/";

	m_lang = getLang(m_fileName);

	m_translations[m_lang] = getTranslation(
		m_configPath, 
		m_fileExtension, 
		m_fileName
	);
}

std::string Localization::translate(
	Lang lang, 
	const std::string& key
) const
{
	initCheck();

	if (key.empty())
	{
		debug("[Localization::translate] Empty key");
		return "";
	}
	/*const std::string& eng = m_translationsEng[key];
	const std::string& ru = m_translationsRu[key];*/
	auto it = m_translations.find(lang);

	if(it == m_translations.end())
	{			
		m_translations[lang] = getTranslation(
			m_configPath, 
			m_fileExtension, 
			getLang(lang)
		);
	}

	const std::string& translatedText = m_translations[lang][key];

	/*if (eng.empty())
	{
		W4_LOG_ERROR("Translation error. Key \"%s\" not found in eng.", key.c_str());
		return TranslationInfo{};
	}

	if (ru.empty())
	{
		W4_LOG_ERROR("Translation error. Key \"%s\" not found in ru.", key.c_str());
		return TranslationInfo{};
	}*/

	if (translatedText.empty())
	{
		W4_LOG_ERROR("Translation error. Key \"%s\" not found in {%s}!", key.c_str(), m_fileName.c_str());
		return "";
	}

	return translatedText;
}

Lang Localization::getCurrentLang() const
{
    return m_lang;
}

void Localization::changeLanguage(Lang lang, bool isNeedToReload)
{
	initCheck();
	
	m_fileName = getLang(lang);
	m_lang = lang;

	loadConfig();
	
	info("[Localization::changeLanguage] {%d}", lang);

	if(isNeedToReload)
	{
		UiText::forEach(
			[currLang = getCurrentLang(),&lang](UiText& text)
			{
				// W4_LOG_ERROR("[Localization::changeLanguage] translate {%s}", lang.c_str());
				text.translate(currLang);
			}
		);

	}
	
};

