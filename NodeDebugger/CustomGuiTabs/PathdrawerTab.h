#pragma once

#include "../ND_iGuiTab.h"
#include "../PathsDeserializer.h"

//Defines to separate from Borders class definitions and headers, as well as JSON deserializer for the containers
struct PF_BorderLineData {
	std::string borderName;
	std::vector<vec2> points;
};

struct PF_BorderContainer {
	std::vector<PF_BorderLineData> borders;
};

namespace nlohmann 
{
	template<>
	struct adl_serializer<PF_BorderLineData>
	{
		static void from_json(const json& j, PF_BorderLineData& obj)
		{
			FIELD_DESERIALIZE(borderName);
			FIELD_DESERIALIZE(points);
		}
	};

	template<>
	struct adl_serializer<PF_BorderContainer>
	{
		static void from_json(const json& j, PF_BorderContainer& obj)
		{
			FIELD_DESERIALIZE(borders);
		}
	};
}
//========

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
	void onGuiPathSearchTab() NOCOMP;
	void selectedPathEditor(int indexOverride = -1) NOCOMP;

	void renderPathNode(const std::string name, const pathNode& nodeData) NOCOMP;
	void renderPathLines(const plottedPath& pathRecordToRedraw, bool forceRedrawColor = false) NOCOMP;
	void selectPathLine(const std::string& pathName) NOCOMP;

	void coloredPathCombo();
	bool checkPathComplete(const plottedPath& pathRecord);

	void exportAsJson(nlohmann::json& inJson) NOCOMP;
	void loadJson() NOCOMP;

	static void onJSONLoadCB(void* self, char* result)
	{
		std::string loadedJson = result;
		nlohmann::json importJson = nlohmann::json::parse(loadedJson);

		if (m_onJsonLoaded) m_onJsonLoaded(importJson);
	}

	//Auto paths generator
	void tryBuildDefaultPaths() NOCOMP;
	bool lineIntersectChecker(const vec2& l1p1, const vec2& l1p2, const vec2& l2p1, const vec2& l2p2) const NOCOMP_B;
	bool isBorderIntersect(const vec2& from, const vec2& to) const NOCOMP_B;

private:

	PathsData m_pathsData;

	std::string m_newPathNodeNaming = "NewPathNodeName";
	std::string m_newPathNaming = "NewPathName";

	int m_selectedPathIndex = -1;
	std::string m_selectedSearchNode = "No Node Selected";
	int m_selectedSearchResultIndex = -1;
	float m_nodeDefaultSpeed = 2.f;
	std::string m_lastSelectedPathName = "";

	std::map<std::string, sptr<Mesh>> m_pathNodeRender; //path points' meshes
	std::map<std::string/*m_pathsData path name*/, std::vector<sptr<Mesh>>> m_pathLinesRender;

	static std::function<void(nlohmann::json)> m_onJsonLoaded;

	//for auto path generator:
	PF_BorderContainer m_borders;
	int m_autoPathPerNode = 4;
};
