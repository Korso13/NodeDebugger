#pragma once

#include "UiPrimitive.h"
#include "common/Defines.h"
#include "common/CommonTypes.h"
#include "Nodes/TextSdfNode.h"  // make sure to forward declare UiText and make it friend class of TextSdfNode
#include "others/localization/Localization.h"
#include "common/fn.h"
#include "common/CommonTypes.h"

struct SettingsText
{
    SettingsText() = default;
    SettingsText(std::string const& fontName_) : fontName(fontName_) {}    
    std::string fontName;
    color color = color::Black;
    int renderOrder = 100;
    vec3 pos = vec3::zero;
    vec3 scale = vec3::one;
    int lineSize = 0;
    vec3 lineOffset = vec3::down;
    TEXT_ALIGN align = TEXT_ALIGN::CENTER;

    SettingsText& setColor(w4::math::color value)
    {
        color = value;
        return *this;
    }
    SettingsText& setRenderOrder(int value)
    {
        renderOrder = value;
        return *this;
    }
    SettingsText& setPosition(vec3 value)
    {
        pos = value;
        return *this;
    }
    SettingsText& setScale(vec3 value)
    {
        scale = value;
        return *this;
    }
    SettingsText& setLineSize(int value)
    {
        lineSize = value;
        return *this;
    }
    SettingsText& setLineOffset(vec3 value)
    {
        lineOffset = value;
        return *this;
    }
    SettingsText& setAlign(TEXT_ALIGN align)
    {
        this->align = align;
        return *this;
    }
};

class UiText : public UiPrimitive
{
    friend class UiText_S;

public:
    UiText(cref<Node> root, const TranslationInfo& text, const SettingsText& settingsText);
    UiText(cref<Node> root, const std::string& text, const SettingsText& settingsText);

    ~UiText() override;

    static void forEach(std::function<void(UiText& text)> transformFn);

    void translate(Lang lang);

    bool isUpdatable() override { return false; }

    void setColor(color const& c, size_t startLine = 0, size_t endLine = 0) const
    {
        if (startLine > endLine || startLine >= m_textLines.size() || endLine >= m_textLines.size())
        {
            W4_LOG_ERROR("[UiText]::setFillColor(%s, %i, %i)", CSTR(c), startLine, endLine);
            return;
        }
        if (endLine == 0)
            endLine = m_textLines.size() - 1;

        if (startLine == endLine)
        {
            m_textLines[startLine]->setFillColor(c);
            m_textLines[startLine]->setStrokeColor(c);
        }
        else
        {
            auto idx = startLine;
            while (idx++ < endLine)
            {
                m_textLines[idx]->setFillColor(c);
                m_textLines[idx]->setStrokeColor(c);
            }
        }
    }

    template <typename StringT>
    void setTextLine(StringT const& text, size_t idx = 0) const;

    template <typename StringT>
    void buildTextNodes(StringT const text, std::string const fontName, color color, int renderOrder);

    // only use this one if text was already constructed
    template <typename StringT>
    void buildTextNodes(StringT const text, std::string const fontName);

    std::vector<sptr<TextSdfNode>> const& getTextNodes() const { return m_textLines; }

    int getLineCount() { return m_textLines.size(); }
    sptr<Node> getTextRoot() { return m_textRoot; }

    float getLen(int lineIndex = 0) const;

    static size_t utf8_strlen(const std::string& utf8_str)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
        std::wstring wchar_str = conv.from_bytes(utf8_str);
        return wchar_str.size();
    }

    [[nodiscard]] vec2 getPlaneSize(size_t line = 0) const
    {
        W4_ASSERT(line < m_textLines.size());

        const auto& mesh = getTextNodes()[line];
        const auto& verticesBuffer = mesh->getVerticesBuffer();

        std::vector<vec3> vertices;
        for (auto it = verticesBuffer->getGeometryIterator(); it; ++it)
            vertices.emplace_back(it->pos * mesh->getWorldScale());

        const auto leftBound =
            std::min_element(vertices.begin(), vertices.end(), [](vec3 const& a, vec3 const& b) { return a.x < b.x; });
        const auto rightBound =
            std::max_element(vertices.begin(), vertices.end(), [](vec3 const& a, vec3 const& b) { return a.x < b.x; });
        const auto topBound =
            std::max_element(vertices.begin(), vertices.end(), [](vec3 const& a, vec3 const& b) { return a.y < b.y; });
        const auto bottomBound =
            std::min_element(vertices.begin(), vertices.end(), [](vec3 const& a, vec3 const& b) { return a.y < b.y; });

        return vec2(rightBound->x - leftBound->x, topBound->y - bottomBound->y);
    }

private:
    template <typename StringT>
    std::vector<std::wstring> convertToLines(StringT const& text) const;
    std::vector<std::wstring> normalizeTextAlg(std::vector<std::wstring> textByLine, sptr<TextSdfNode> refSdfNode);

    void rusFix(cref<TextSdfNode> node, const std::string& fontName) const;
    std::wstring wcharToWstring(wchar_t letter);

private:
    sptr<Node> m_textRoot;
    std::vector<sptr<TextSdfNode>> m_textLines;    

    std::string m_pathToAltas;
    std::string m_pathToJson;    

    SettingsText m_setting;
    TranslationInfo m_translationInfo;
};
