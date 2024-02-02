#include "ConsoleTab.h"

#ifndef FORCENODEDEBUG_OFF

ConsoleTab::ConsoleTab()
	: iGuiTab("Console")
{
    //setting listener to console output
    w4::logger::setLogListener([this](int const logLevel, char const* const file, int const line, char const* const message)
        {
            ConsoleRec newRecord;
            switch (logLevel)
            {
            case(1):
                if (minLoggingLevel > 1)
                    return;
                newRecord.first = LogLevel::Debug;
                break;
            case(2):
                if (minLoggingLevel > 2)
                    return;
                newRecord.first = LogLevel::Info;
                break;
            case(3):
                if (minLoggingLevel > 3)
                    return;
                newRecord.first = LogLevel::Warning;
                break;
            case(4):
                if (minLoggingLevel > 4)
                    return;
                newRecord.first = LogLevel::Error;
                break;
            default:
                break;
            }

            newRecord.second = message;
            m_consoleOutput.push_back(newRecord);
        });
}

void ConsoleTab::onGuiCall()
{
    int logSize = m_consoleOutput.size();

    ImGui::Checkbox("Auto scroll", &m_autoScrollLog);

    if (ImGui::Button("Clear log"))
    {
        m_consoleOutput.clear();
        m_consoleOutput.push_back({ LogLevel::Debug, "[Log Start]" });
        m_lastConsoleEntry = 0;
    }

    //Logging level selector
    ImGui::Combo("Show debug level", &m_shownLoggingLevel, m_logLevels, 5, 5);

    //console log printing
    ImGui::BeginListBox("");
    for (size_t i = 0; i < logSize; i++)
    {
        //records filtering (via combo box)
        if (m_shownLoggingLevel != 0 && (CastI(m_consoleOutput[i].first) != m_shownLoggingLevel))
            continue;

        //color picker
        ImVec4 color;
        switch (m_consoleOutput[i].first)
        {
        case(LogLevel::Debug):
            color = IMG_WHITE;
            break;
        case(LogLevel::Info):
            color = IMG_VIOLET;
            break;
        case(LogLevel::Warning):
            color = IMG_ORANGE;
            break;
        case(LogLevel::Error):
            color = IMG_RED;
            break;
        default:
            break;
        }

        ImGui::TextColored(color, "%s", m_consoleOutput[i].second.c_str());
    }

    //auto scroll functionality
    if (m_autoScrollLog)
    {
        m_lastConsoleEntry = logSize - 1;
        ImGui::SetScrollHereY(1.f);
    }

    ImGui::EndListBox();
}


#endif