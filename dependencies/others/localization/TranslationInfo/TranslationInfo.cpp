#include "TranslationInfo.h"
#include "../Localization.h"

TranslationInfo::TranslationInfo(
    const std::string& key,
    std::function<std::string(Lang, const std::string& translatedText)>
        transformFn
)
    : m_key(key),
      m_transformFn(std::move(transformFn))
{
    
}

std::string TranslationInfo::getCurr(Lang lang) const
{
    std::string translated = Localization::instance().translate(
        lang,
        m_key
    );
    
    return m_transformFn(lang, translated);
}