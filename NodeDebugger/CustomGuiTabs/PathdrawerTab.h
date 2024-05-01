#pragma once

#include "../ND_iGuiTab.h"
#include "../PathsDeserializer.h"

class PathdrawerTab : public iGuiTab
{
public:

	PathdrawerTab() : iGuiTab("Pathdrawer") {};

	void onGuiCall() override NOCOMP;
	bool isEmpty() override { return false; };

	void addItem(std::any item, const std::string& altName = "default") override {};
	void purgeValues() override { };

private:

	void onGuiPathNodesTab() NOCOMP;
	void onGuiPathEditorTab() NOCOMP;

	void renderPathNode(const std::string name, const pathNode& nodeData) NOCOMP;
	void renderPathLines(const plottedPath& pathRecordToRedraw, bool forceRedrawColor = false) NOCOMP;
	void selectPathLine(const std::string& pathName) NOCOMP;

	void coloredPathCombo();
	bool checkPathComplete(const plottedPath& pathRecord);

	void exportAsJson(nlohmann::json& inJson) NOCOMP;
	void loadJson() NOCOMP;

	static void onJSONLoadCB(void* self, char* result)
	{
		std::string partSettings = result;
		nlohmann::json importJson = nlohmann::json::parse(partSettings);

		if (m_onJsonLoaded) m_onJsonLoaded(importJson);
	}

private:

	PathsData m_pathsData;

	std::string m_newPathNodeNaming = "NewPathNodeName";
	std::string m_newPathNaming = "NewPathName";

	int m_selectedPathIndex = -1;
	float m_nodeDefaultSpeed = 2.f;
	std::string m_lastSelectedPathName = "";

	std::map<std::string, sptr<Mesh>> m_pathNodeRender; //path points' meshes
	std::map<std::string/*m_pathsData path name*/, std::vector<sptr<Mesh>>> m_pathLinesRender;

	static std::function<void(nlohmann::json)> m_onJsonLoaded;
};
