#pragma once

#include "../ND_iGuiTab.h"
#include <settings/JsonDeserializator.h>

//Tab for building borders for pathfinding, triggers and etc.
//Load existing json file or create from scratch, then export as json
//1. Create Border Record with unique name
//2. Add points via Add new point button or by holding Ctrl and clicking on the map

//for serialization to json file
struct TabBorderLineData {
	std::string borderName;
	std::vector<vec2> points;
};

struct TabBorderContainer {
	std::vector<TabBorderLineData> borders;
};

namespace nlohmann {

	template<>
	struct adl_serializer<TabBorderLineData>
	{
		static void from_json(const json& j, TabBorderLineData& obj)
		{
			FIELD_DESERIALIZE(borderName);
			FIELD_DESERIALIZE(points);
		}

		static void to_json(json& j, const TabBorderLineData& obj)
		{
			FIELD_SERIALIZE(borderName);
			FIELD_SERIALIZE(points);
		}
	};

	template<>
	struct adl_serializer<TabBorderContainer>
	{
		static void from_json(const json& j, TabBorderContainer& obj)
		{
			FIELD_DESERIALIZE(borders);
		}

		static void to_json(json& j, const TabBorderContainer& obj)
		{
			FIELD_SERIALIZE(borders);
		}
	};
}


class BordersTab : public iGuiTab
{
public:

	BordersTab() : iGuiTab("Borders") 
	{
		event::Keyboard::Down::subscribe(std::bind(&BordersTab::onKeyPressed, this, std::placeholders::_1));
		event::Keyboard::Up::subscribe(std::bind(&BordersTab::onKeyReleased, this, std::placeholders::_1));
		Touch::Begin::subscribe(std::bind(&BordersTab::onTouchBegin, this, std::placeholders::_1));
	};

	void onGuiCall() override NOCOMP;
	bool isEmpty() override { return false; };

	void addItem(std::any item, const std::string& altName = "default") override NOCOMP;
	void purgeValues() override { m_debugBorders.clear(); m_borderRender.clear();  m_borderLinesRender.clear(); }

private:

	void ImGuiBorderPoint(sptr<TabBorderLineData> borderRecord, size_t pointIndex, bool& bNeedRemovePoint, int& removePointIndex) NOCOMP;
	void addBorderPoint(sptr<TabBorderLineData> borderRecord, vec2 newPointPos = vec2(666)) NOCOMP;
	void removeBorderPoint(sptr<TabBorderLineData> borderRecord, size_t removePointIndex) NOCOMP;
	
	void renderBorderLines(std::string borderRecordToRedraw) NOCOMP;

	void onKeyPressed(const event::Keyboard::Down& evt)NOCOMP;
	void onKeyReleased(const event::Keyboard::Up& evt)NOCOMP;
	void onTouchBegin(const Touch::Begin& evt) NOCOMP;
	
	void exportAsJson(nlohmann::json& inJson) NOCOMP;
	void loadJson() NOCOMP;

	static void onJSONLoadCB(void* self, char* result)
	{
		std::string partSettings = result;
		nlohmann::json importJson = nlohmann::json::parse(partSettings);

		if (m_onJsonLoaded) m_onJsonLoaded(importJson);
	}

private:

	bool m_clickAddModeON = false;
	int m_needScrollDownNextFrame = 0; 

	std::string m_newBorderNaming = "New_border";
	std::set<sptr<TabBorderLineData>> m_debugBorders;
	sptr<TabBorderLineData> m_lastEditedBorder;
	std::map<std::string, sptr<Mesh>> m_borderRender; //border points' meshes
	std::map<std::string/*m_debugBorders border name*/, std::vector<sptr<Mesh>>> m_borderLinesRender;

	static std::function<void(nlohmann::json)> m_onJsonLoaded;
};
