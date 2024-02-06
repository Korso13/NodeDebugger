#pragma once

#include "../ND_iGuiTab.h"

class SpinesTab : public iGuiTab
{
public:

	SpinesTab() : iGuiTab("Spines") {};

	void onGuiCall() override NOCOMP;
	bool isEmpty() override { return m_debugSpines.empty(); };

	void addItem(std::any item, const std::string& altName = "default") override NOCOMP;
	void purgeValues() override { m_debugSpines.clear(); }

private:

	bool m_copyAnimNameNotLength = false;
	std::vector<SpineDebugging> m_debugSpines;
	std::string m_spinesFilter = "";
};