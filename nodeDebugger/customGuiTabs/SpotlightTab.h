#pragma once

#include "../ND_iGuiTab.h"

class SpotlightTab : public iGuiTab
{
public:

    SpotlightTab() : iGuiTab("SpotLights") {};
	
	void onGuiCall() override NOCOMP;
	bool isEmpty() override { return m_debugSpotLights.empty(); };

    void addItem(std::any item, const std::string& altName = "default") override NOCOMP;
	void purgeValues() override { m_debugSpotLights.clear(); }

private:

	std::vector<SpotLightDebugging> m_debugSpotLights;
};