#pragma once
#include "W4Framework.h"
#include "settings/InstanceInitiation.h"
#include "settings/SettingsContainer.h"
#include "common/Defines.h"
#include "TranslationInfo/TranslationInfo.h"

W4_USE_UNSTRICT_INTERFACE

class Localization : public InstanceInitiation
{
public:

	static Localization& instance()
	{
		static Localization instance;
		return instance;
	}

	Localization();
	virtual ~Localization();

	virtual void loadConfig() override;

	std::string translate(
		Lang lang, 
		const std::string& key
	) const;

	Lang getCurrentLang() const;

	// if isNeedToReload = true, all texts will be updated to new lang
	void changeLanguage(Lang lang, bool isNeedToReload = false);

	static Lang getLang(const std::string& key);
	static const std::string& getLang(Lang lang);

private:


private:
	/*SettingsContainer<std::string> m_translationsEng;
	SettingsContainer<std::string> m_translationsRu;*/

	mutable std::umap<Lang, SettingsContainer<std::string>> m_translations;
	mutable std::string m_fileName = "en";

	bool m_isInit = false;
	Lang m_lang = Lang::EN;
	std::string m_fileExtension = ".json";
};
