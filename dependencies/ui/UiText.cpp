#include "UiText.h"
#include "common/Defines.h"
#include "common/nodeDebugger/NodeDebugger.h"
#include "others/localization/Localization.h"
#include "settings/CustomSettingsContainer.h"
#include "settings/RusFix/RusFix.h"
#include <codecvt>
#include <locale>

static std::set<UiText*> m_allTexts;

UiText::UiText(
    cref<Node> root,
    const TranslationInfo& text,
    const SettingsText& settingsText
)
    : UiPrimitive(root),
      m_textRoot(make::sptr<Node>("text_root")),
      m_setting(settingsText),
      m_translationInfo(text)
{
    m_root->setLocalTranslation(m_setting.pos);

    m_textRoot->setParent(m_root, false);
    m_textRoot->setLocalScale(m_setting.scale);

    buildTextNodes(
        text.getCurr(Localization::instance().getCurrentLang()),
        m_setting.fontName,
        m_setting.color,
        m_setting.renderOrder
    );

    m_allTexts.emplace(this);
}


UiText::UiText(
    cref<Node> root,
    const std::string& text,
    const SettingsText& settingsText
)
    : UiText(
          root,
          TranslationInfo(
              "",
              [text](Lang, const std::string&)
              {
                  return text;
              }
          ),
          settingsText
      )
{
}


UiText::~UiText()
{
    m_allTexts.erase(this);

    for (const auto& node : m_textLines)
        node->setParent(nullptr);
    m_textRoot->setParent(nullptr);
}

void UiText::forEach(std::function<void(UiText& text)> transformFn)
{
    // W4_LOG_ERROR("UiText::forEach total: %d", m_allTexts.size());
    for (UiText* txt : m_allTexts)
    {
        transformFn(*txt);
    }
}

void UiText::translate(Lang lang)
{
    buildTextNodes(
        m_translationInfo.getCurr(lang),
        m_setting.fontName,
        m_setting.color,
        m_setting.renderOrder
    );
}

template void UiText::setTextLine<std::string>(
    std::string const& text,
    size_t idx
) const;

template void UiText::setTextLine<std::wstring>(
    std::wstring const& text,
    size_t idx
) const;

template <typename StringT>
void UiText::setTextLine(StringT const& text, size_t idx) const
{
    static_assert(
        std::is_same_v<StringT, std::string>
            || std::is_same_v<StringT, std::wstring>,
        "try std::string or std::wstring"
    );

    if (idx >= m_textLines.size())
    {
        W4_LOG_ERROR(
            "[UiText]::setTextLine(%s, %i) too high id",
            text.data(),
            idx
        );
        return;
    }

    if constexpr (std::is_same_v<StringT, std::string>)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
        m_textLines[idx]->renderText(convert.from_bytes(text));
    }
    else
    {
        m_textLines[idx]->renderText(text);
    }

    m_textLines[idx]->setLocalTranslation(m_setting.lineOffset * idx);
    m_textLines[idx]->translateLocal(
        SV3("text_alignment")
        * m_textLines[idx]->measureTextLength(m_textLines[idx]->getLastText())
    );
}

template void UiText::buildTextNodes<std::string>(
    std::string const,
    std::string const
);
template void UiText::buildTextNodes<std::wstring>(
    std::wstring const,
    std::string const
);
template void UiText::buildTextNodes<std::string>(
    std::string const,
    std::string const,
    color,
    int
);
template void UiText::buildTextNodes<std::wstring>(
    std::wstring const,
    std::string const,
    color,
    int
);

template <typename StringT>
void UiText::buildTextNodes(StringT const text, std::string const fontName)
{
    static_assert(
        std::is_same_v<StringT, std::string>
            || std::is_same_v<StringT, std::wstring>,
        "try std::string or std::wstring"
    );

    if (m_textLines.empty())
    {
        W4_LOG_ERROR("[UiText]::setText(%s)", text.data());
        return;
    }

    const auto& node = *m_textLines.begin();
    const auto color = node->getFillColor();
    const auto renderOrder = node->getRenderOrder();

    buildTextNodes(text, fontName, color, renderOrder);
}

template <typename StringT>
void UiText::buildTextNodes(
    StringT const text,
    std::string const fontName,
    color color,
    int renderOrder
)
{
    static_assert(
        std::is_same_v<StringT, std::string>
            || std::is_same_v<StringT, std::wstring>,
        "try std::string or std::wstring"
    );

    for (const auto& textNode : m_textLines)
        textNode->setParent(nullptr);
    m_textLines.clear();

    auto pathToAtlas = SS("path_to_fonts");
    // pathToAtlas += "marvin";
    pathToAtlas += fontName;
    pathToAtlas += ".png";
    // if (w4::filesystem::isPathExists(pathToAtlas + ".png"))
    /*else
        pathToAtlas += ".jpg";*/

    m_pathToAltas = pathToAtlas;

    auto pathToJson = SS("path_to_fonts");
    // pathToJson += "marvin";
    pathToJson += fontName;
    pathToJson += ".json";
    m_pathToJson = pathToJson;    

    const std::vector<std::wstring> textByLine = convertToLines(text);
    int i = 0;
    for (const std::wstring& str : textByLine)
    {
        auto node = make::sptr<TextSdfNode>(
            ResourceFileDescription { "default.w4a", pathToAtlas },
            pathToJson,
            color
        );
        node->setParent(m_textRoot, false);
        node->setRenderOrder(renderOrder);
        node->renderText(str);
        node->setLocalTranslation(m_setting.lineOffset * i);
        node->setFillColor(color);
        node->setStrokeColor(color);

        rusFix(node, fontName);
        m_textLines.push_back(node);

        switch (m_setting.align)
        {
        case LEFT:
            node->translateLocal(
                SV3("text_alignment")
                * node->measureTextLength(node->getLastText())
            );
            break;
        case RIGHT:
            node->translateLocal(
                -SV3("text_alignment")
                * node->measureTextLength(node->getLastText())
            );
            break;
        default:
            break;
        }

#ifdef DEBUG
        node->setName("text_sdf_" + std::to_string(i));
#endif
        i++;
    }
}

std::vector<std::wstring> UiText::normalizeTextAlg(
    std::vector<std::wstring> textByLine,
    sptr<TextSdfNode> refSdfNode
)
{
    float maxLineWidth = 0;
    for (auto& line : textByLine)
    {
        // Remove leading spaces
        auto firstChar = line.find_first_not_of(L' ');
        if (firstChar != std::wstring::npos)
        {
            line.erase(0, firstChar);
        }

        // Remove trailing spaces
        auto lastChar = line.find_last_not_of(L' ');
        if (lastChar != std::wstring::npos)
        {
            line.erase(lastChar + 1);
        }

        // Measure the line width
        float lineWidth = refSdfNode->measureTextLength(line);
        if (lineWidth > maxLineWidth)
        {
            maxLineWidth = lineWidth;
        }
    }

    // Align each line
    for (auto& line : textByLine)
    {
        float lineWidth = refSdfNode->measureTextLength(line);
        float spaceWidth = refSdfNode->measureTextLength(
            L" "
        ); // Width of a single space
        int spacesNeeded = static_cast<int>(
            (maxLineWidth - lineWidth) / spaceWidth / 2
        );

        std::wstring padding(spacesNeeded, L' '); // Create a padding string
        line = padding + line + padding; // Center-align the line
    }

    return textByLine;
}

template std::vector<std::wstring> UiText::convertToLines<std::string>(
    std::string const& text
) const;
template std::vector<std::wstring> UiText::convertToLines<std::wstring>(
    std::wstring const& text
) const;

template <typename StringT>
std::vector<std::wstring> UiText::convertToLines(StringT const& text) const
{
    static_assert(
        std::is_same_v<StringT, std::string>
            || std::is_same_v<StringT, std::wstring>,
        "try std::string or std::wstring"
    );

    auto refSdfNode = make::sptr<TextSdfNode>(
        ResourceFileDescription { "default.w4a", m_pathToAltas },
        m_pathToJson,
        m_setting.color
    );

    auto normalizeTextAlg = [&](std::vector<std::wstring>& textByLine)
    {
        float maxLineWidth = 0;
        for (auto& line : textByLine)
        {
            // Remove leading spaces
            for (auto& item : line)
            {
                auto firstChar = line.find_first_not_of(L' ');
                if (firstChar != std::wstring::npos)
                {
                    line.erase(0, firstChar);
                }
                else
                    break;
            }

            for (auto& item : line)
            {
                // Remove trailing spaces
                auto lastChar = line.find_last_not_of(L' ');
                if (lastChar != std::wstring::npos)
                {
                    // info("Erasing %d char from string of size %d", (lastChar
                    // + 1), line.size());
                    line.erase(lastChar + 1);
                }
                else
                    break;
            }

            // Measure the line width
            float lineWidth = refSdfNode->measureTextLength(line);
            if (lineWidth > maxLineWidth)
            {
                maxLineWidth = lineWidth;
            }
        }

        switch (m_setting.align)
        {
        default:
            for (auto& line : textByLine)
            {
                float lineWidth = refSdfNode->measureTextLength(line);
                float spaceWidth = refSdfNode->measureTextLength(
                    L" "
                ); // Width of a single space
                int spacesNeeded = static_cast<int>(
                    (maxLineWidth - lineWidth) / spaceWidth
                    / (m_setting.align == TEXT_ALIGN::CENTER ? 2 : 1)
                );

                std::wstring padding(
                    spacesNeeded,
                    L' '
                ); // Create a padding string
                line = padding + line + padding; // Center/Right-align the line
            }
        case LEFT:
            break;
        }
    };
    const auto convert = [](int lineSize,
                            std::wstring const& wText,
                            std::vector<std::wstring>& textByLine)
    {
        const int sizeLineConst = lineSize;
        int sizeLine = sizeLineConst;
        std::wstring word;
        std::wstring line;

        for (const auto symbol : wText)
        {
            --sizeLine;
            word += symbol;
            if (symbol == ' ')
            {
                if (sizeLine >= 0)
                {
                    line += word;
                    word.clear();
                }
                else
                {
                    textByLine.push_back(line);
                    line.clear();
                    line += word;
                    word.clear();
                    sizeLine = sizeLineConst - line.size();
                }
            }
            else if (symbol == '\n')
            {
                if (sizeLine >= 0)
                {
                    word.pop_back();
                    line += word;
                    word.clear();
                    textByLine.push_back(line);
                    line.clear();
                    sizeLine = sizeLineConst;
                }
                else
                {
                    if (!line.empty())
                        textByLine.push_back(line);
                    line.clear();
                    word.pop_back();
                    line += word;
                    word.clear();
                    textByLine.push_back(line);
                    line.clear();
                    sizeLine = sizeLineConst;
                }
            }
        }
        if (sizeLine < 0)
        {
            textByLine.push_back(line);
            line.clear();
        }
        line += word;
        textByLine.push_back(line);
    };

    std::vector<std::wstring> textByLine;

    if constexpr (std::is_same_v<StringT, std::string>)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        const auto wText = converter.from_bytes(text);

        if (m_setting.lineSize > 0 && wText.length() > m_setting.lineSize)
        {
            convert(m_setting.lineSize, wText, textByLine);

            normalizeTextAlg(textByLine);
        }
        else
        {
            textByLine.push_back(std::move(wText));
            normalizeTextAlg(textByLine);
        }
    }
    else
    {
        if (m_setting.lineSize > 0 && text.length() > m_setting.lineSize)
        {
            convert(m_setting.lineSize, text, textByLine);
            normalizeTextAlg(textByLine);
        }
        else
        {
            textByLine.push_back(text);
            normalizeTextAlg(textByLine);
        }
    }

    return textByLine;
}

float UiText::getLen(int lineIndex) const
{
    if (m_textLines.empty() || m_textLines.size() < lineIndex + 1)
        return 0;
    const sptr<TextSdfNode> line = m_textLines.at(lineIndex);
    return line->getLastText().length() * m_textRoot->getLocalScale().x
        * SF("text_length");
}

void UiText::rusFix(cref<TextSdfNode> node, const std::string& fontName) const
{
    static const float fullOffset = SF("rusFullOffset");
    const auto isRu = [](wchar_t sym)
    {
        return (sym >= 1040 && sym <= 1103) || sym == 1025 || sym == 1105;
    };
    if (m_setting.align == TEXT_ALIGN::LEFT)
    {
        const wchar_t sym = node->getLastText().front();
        if (isRu(sym))
        {
            int REALletterSize = RusFix::instance()
                                     .getFontConfig(fontName)
                                     .characters.at(sym)
                                     .advance;
            float factor = m_setting.align == TEXT_ALIGN::CENTER ? 0.5f : 1.f;
            node->translateLocal(
                0.4f * vec3::right * REALletterSize * fullOffset * factor
                * SF("rusOffset_" + STR(sym))
            ); // calculate new position
        }
    }
    else
    {
        for (const wchar_t sym : node->getLastText())
        {
            if (isRu(sym))
            {
                int REALletterSize = RusFix::instance()
                                         .getFontConfig(fontName)
                                         .characters.at(sym)
                                         .advance;
                float factor = m_setting.align == TEXT_ALIGN::CENTER ? 0.5f : 1.f;
                // node->translateLocal(vec3::right *
                // node->measureTextLength({sym}) * factor); // reset original
                // broken position
                node->translateLocal(
                    vec3::left * REALletterSize * fullOffset * factor
                    * SF("rusOffset_" + STR(sym))
                ); // calculate new position
            }
        }
    }
}

std::wstring UiText::wcharToWstring(wchar_t letter)
{
    std::wstring wstr;
    wstr += letter;
    return wstr;
}
