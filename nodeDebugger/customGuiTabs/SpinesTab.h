#pragma once

#include "../ND_iGuiTab.h"

class SpinesTab : public iGuiTab
{
//public:
	struct SpineDebugging
	{
		std::string spineName;
		wptr<Spine> spine;
		std::vector<std::string> animationNames;
		int lastSelected = 0;
		std::vector<float> animationLength;
		std::vector<std::string> skinNames = { "none" };
		int selectedSkin1 = 0;
		int selectedSkin2 = 0;
		int selectedSkin3 = 0;
		int selectedSkin4 = 0;
		int selectedSkin5 = 0;
		int selectedSkin6 = 0;
		std::map<int, std::string> slots;
	};

public:

	SpinesTab() : iGuiTab("Spines") {};

	void onGuiCall() override NOCOMP;
	bool isEmpty() override { return m_debugSpines.empty(); };

	void addItem(std::any item, const std::string& altName = "default") override NOCOMP;
	void removeItem(const std::string& itemName) override NOCOMP;
	void purgeValues() override { m_debugSpines.clear(); }

	void renderSpineProperties(SpineDebugging& spineInfo, const std::string& name, bool noTransformTabs = false) NOCOMP;
	// If returns falls - use getSpineInfo overload for sptr<Spine>
	bool hasSpineRecord(const std::string& spineRecordName) const NOCOMP;
	SpineDebugging& getSpineInfo(const std::string& spineRecordName) NOCOMP;
	SpineDebugging& getSpineInfo(sptr<Spine>& spine) NOCOMP;

private:

	void onGuiAnimations(SpineDebugging& debugSpine) NOCOMP;
	void onGuiSkinSetter(SpineDebugging& debugSpine) NOCOMP;

private:

	bool m_copyAnimNameNotLength = true;
	std::map<std::string, SpineDebugging> m_debugSpines;
	std::unordered_map<size_t, SpineDebugging> m_cachedSpineInfo; //used with unregestered spines (size_t - hash of sptr)
	std::string m_spinesFilter = "";
};