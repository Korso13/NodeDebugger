#pragma once

#include "../ND_iGuiTab.h"

class ConsoleTab : public iGuiTab
{
public:

	ConsoleTab();

	void onGuiCall() override NOCOMP;
	bool isEmpty() override {return false;};

	//Not used
	void addItem(std::any item, const std::string& altName = "default") override {};

	void purgeValues() override { m_consoleOutput.clear(); };

private:

	typedef std::pair<LogLevel, std::string> ConsoleRec;
	std::vector<ConsoleRec> m_consoleOutput = { { LogLevel::Debug, "[Log Start]" } };
	int m_lastConsoleEntry = 0; //selected entry
	bool m_autoScrollLog = true;
	int m_shownLoggingLevel = 0; //what is shown in ImGui console
	const char* m_logLevels[5] = { "All", "Debug", "Info", "Warning", "Error" };

};
