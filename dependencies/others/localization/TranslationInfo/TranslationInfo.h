#pragma once

#include "Lang.h"
#include "common/Defines.h"

class TranslationInfo
{
public:
    
    TranslationInfo() = default;

    TranslationInfo(
        const std::string& key,
        std::function<std::string(Lang, const std::string& translatedText)> transformFn = TranslationInfo::getDefaultTransform()
    );

    std::string getCurr(Lang lang) const;

public:

    static inline std::function<std::string(Lang, const std::string&)> getDefaultTransform()
    {
        return [](Lang, const std::string& txt)
        {
            return txt;
        };
    }


private:
    std::string m_key;

    /// @brief you can transform the translated text if needed,
    // some languages have different word orders, "1 apple", or "apple 1"
    std::function<std::string(Lang, const std::string& translatedText)> m_transformFn;
};