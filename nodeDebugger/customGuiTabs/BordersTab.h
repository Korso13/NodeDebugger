#pragma once

#include "../ND_iGuiTab.h"
#include "common/nodeDebugger/spawner/SpawnerEnums.h"

//Tab for building borders for pathfinding, triggers and etc.
//Load existing json file or create from scratch, then export as json
//1. Create Border Record with unique name
//2. Add points via Add new point button or by holding Ctrl and clicking on the map


class BordersTab : public iGuiTab
{
//helper-structs
public:
	struct TabBorderLineData {
		std::string borderName;
		std::vector<vec2> points;
		BorderType border_type = BorderType::Outer;
	};
	struct TabBorderContainer {
		std::vector<TabBorderLineData> borders;
	};

private:
	// Struct holding data and methods to represent and export specific record in a BorderFile
	struct BorderRec
	{
	// fields
		bool isRendered = true;
		bool isExportedInLocal = false;
		std::string borderRecName;
		std::string borderInstId;
		std::string triggerInstId;
		vec4 borderColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);

	// methods
		// ctor for loaded from file
		explicit BorderRec(TabBorderLineData& data) NOCOMP;
		// ctor for newly created Border
		explicit BorderRec(const std::string& borderName) NOCOMP;
		~BorderRec() NOCOMP;
		bool onGui() NOCOMP_B;
		void addBorderPoint(vec2 newPointPos = INVALIDPOS.withoutZ()) NOCOMP;
#ifndef FORCENODEDEBUG_OFF
		TabBorderLineData getData() ;
#else
		TabBorderLineData getData() {return TabBorderLineData();};
#endif
		void showBorders(bool show) NOCOMP;

		sptr<Node> getBorderRoot() NOCOMP_PTR;

	private:
		void init() NOCOMP;
		void updateBorderRender() NOCOMP;
		void onGuiBorderInstancing() NOCOMP;
		void onGuiPoint(size_t idx, std::optional<int>& removePointIndex) NOCOMP;
		void removeBorderPoint(int idx);

	private:
		TabBorderLineData m_points;
		sptr<MaterialInst> m_renderMaterial;
		bool m_deferredBorderReAdd = false;
		bool m_deferredTriggerReAdd = false;
		int m_needScrollDownNextFrame = 0; //todo: do we still need this?
		std::vector<sptr<Mesh>> m_borderPointsRender;
		std::vector<sptr<Mesh>> m_borderLinesRender;
		wptr<Node> m_localRefNode; //used if isExportedInLocal == true to recalculate points in coords local to this node
	};

	// Struct holding data and methods to represent and export all records for a Border File
	struct BorderFile
	{
		friend struct BorderRec;
		using BorderRecIt = std::vector<BorderRec>::iterator;

	// fields
		bool isRendered = true;
		std::string borderFileName;
		std::string borderFilePath; //for use in future

	// methods
		// ctor for loaded from file
		BorderFile(TabBorderContainer& borderData,
			const std::string& borderFileName,
			const std::string& borderFilePath = "") NOCOMP;
		// ctor for newly created Border 'file'
		BorderFile(const std::string& borderFileName) NOCOMP;

		bool onGui() NOCOMP_B;
		void exportToFile() NOCOMP;
		void addBorder(const std::string& borderName) NOCOMP;
		void removeBorder(BorderRecIt borderIterator) NOCOMP;
		void clickAddBorderPoint(vec2 newPointPos = INVALIDPOS.withoutZ()) NOCOMP;

	private:

		static sptr<Node> m_defaultBorderNode;

		std::vector<BorderRec> m_borders;
		BorderRecIt m_selectedBorder = m_borders.end();
		std::string m_newBorderName = "Border";

		bool m_needUpdateCache = true;
		std::vector<BorderRecIt> m_cachedBorderList;

		bool m_needRefocusInputField = false;
		int m_refocusInputFieldCursorPos = 0;
	};

//class description
public:
	BordersTab() : iGuiTab("Borders") 
	{
		event::Keyboard::Down::subscribe(std::bind(&BordersTab::onKeyPressed, this, std::placeholders::_1));
		event::Keyboard::Up::subscribe(std::bind(&BordersTab::onKeyReleased, this, std::placeholders::_1));
		Touch::Begin::subscribe(std::bind(&BordersTab::onTouchBegin, this, std::placeholders::_1));
	};

	void onGuiCall() override NOCOMP;
	bool isEmpty() override { return false; } // This tab must always show up regardless of contents

	void addItem(std::any item, const std::string& altName = "default") override NOCOMP;
	void purgeValues() override { m_borderFiles.clear(); }

private:

	void onKeyPressed(const event::Keyboard::Down& evt)NOCOMP;
	void onKeyReleased(const event::Keyboard::Up& evt)NOCOMP;
	void onTouchBegin(const Touch::Begin& evt) NOCOMP;

	const char* checkTabNameUnique(const std::string& name) NOCOMP_PTR;
	void loadJson() NOCOMP;

	static void onJSONLoadCB(void* self, char* result, char* fileName, char* filePath)
	{
		std::string partSettings = result;
		nlohmann::json importJson = nlohmann::json::parse(partSettings);

		m_lastLoadedFileName = fileName;
		m_lastLoadedFilePath = filePath;
		if (m_onJsonLoaded) m_onJsonLoaded(importJson);
	}

private:

	bool m_clickAddModeON = false;
	int m_selectedTabBar = -1;
	std::vector<BorderFile> m_borderFiles;
	std::set<std::string> m_tabNamesChecker;

	// statics for json loading
	inline static std::string m_lastLoadedFileName = "";
	inline static std::string m_lastLoadedFilePath = "";
	static std::function<void(nlohmann::json)> m_onJsonLoaded;
};
