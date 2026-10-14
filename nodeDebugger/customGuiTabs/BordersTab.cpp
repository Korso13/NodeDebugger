#include "BordersTab.h"
#include "../ND_JsonTools.h"
#include <settings/JsonDeserializator.h>

#include "common/nodeDebugger/CustomAutos.h"
#include "game/border/Border.h"
#include "game/border/BorderController.h"

std::function<void(nlohmann::json)> BordersTab::m_onJsonLoaded = nullptr;
sptr<Node> BordersTab::BorderFile::m_defaultBorderNode = nullptr;

#ifndef FORCENODEDEBUG_OFF

//for serialization to/from json file
namespace nlohmann {
template<>
struct adl_serializer<BordersTab::TabBorderLineData>
{
	static void from_json(const json& j, BordersTab::TabBorderLineData& obj)
	{
		FIELD_DESERIALIZE(borderName);
		FIELD_DESERIALIZE(points);
		FIELD_DESERIALIZE_OPTIONAL(border_type);
	}

	static void to_json(json& j, const BordersTab::TabBorderLineData& obj)
	{
		FIELD_SERIALIZE(borderName);
		FIELD_SERIALIZE(points);
		FIELD_SERIALIZE(border_type);
	}
};

template<>
struct adl_serializer<BordersTab::TabBorderContainer>
{
	static void from_json(const json& j, BordersTab::TabBorderContainer& obj)
	{
		FIELD_DESERIALIZE(borders);
	}

	static void to_json(json& j, const BordersTab::TabBorderContainer& obj)
	{
		FIELD_SERIALIZE(borders);
	}
};
}

//overload needed for std::set to function
bool operator<(const sptr<BordersTab::TabBorderLineData>& b1, const sptr<BordersTab::TabBorderLineData>& b2)
{
	return b1->borderName < b2->borderName;
}

vec4 getNextColorForBorder()
{
	static int s_colorUsedId = -1;
	constexpr float s_alphaTransparency = 0.7;

	if (++s_colorUsedId > 12)
		s_colorUsedId = 0;

	switch (s_colorUsedId)
	{
	case(0): return vec4(color(color::Yellow).rgb(), s_alphaTransparency);
	case(1): return vec4(color(color::Blue).rgb(), s_alphaTransparency);
	case(2): return vec4(color(color::OrangeRed).rgb(), s_alphaTransparency);
	case(3): return vec4(color(color::PeachPuff).rgb(), s_alphaTransparency);
	case(4): return vec4(color(color::Lavender).rgb(), s_alphaTransparency);
	case(5): return vec4(color(color::Pink).rgb(), s_alphaTransparency);
	case(6): return vec4(color(color::DarkGreen).rgb(), s_alphaTransparency);
	case(7): return vec4(color(color::Cyan).rgb(), s_alphaTransparency);
	case(8): return vec4(color(color::Crimson).rgb(), s_alphaTransparency);
	case(9): return vec4(color(color::DarkKhaki).rgb(), s_alphaTransparency);
	case(10): return vec4(color(color::Violet).rgb(), s_alphaTransparency);
	case(11): return vec4(color(color::Teal).rgb(), s_alphaTransparency);
	case(12): return vec4(color(color::Gold).rgb(), s_alphaTransparency);
	default: return vec4::one;
	}
}

BordersTab::BorderRec::BorderRec(TabBorderLineData& data)
	:
borderRecName(data.borderName),
borderColor(getNextColorForBorder()),
m_points(data)
{
	debug("[BordersTab::BorderRec::BorderRec][TabBorderLineData] %s", data.borderName.c_str());
	init();
}

BordersTab::BorderRec::BorderRec(const std::string& borderName)
	:
borderRecName(borderName),
borderColor(getNextColorForBorder())
{
	debug("[BordersTab::BorderRec::BorderRec][std::string] %s", borderName.c_str());
	init();
}

BordersTab::BorderRec::~BorderRec()
{
	debug("[BordersTab::BorderRec::~BorderRec] %s", borderRecName.c_str());
	std::for_each(m_borderPointsRender.begin(), m_borderPointsRender.end(), [](const sptr<Mesh>& mesh)
		{
			if (mesh->getParent())
			mesh->getParent()->removeChild(mesh);
		});
	std::for_each(m_borderLinesRender.begin(), m_borderLinesRender.end(), [](const sptr<Mesh>& mesh)
		{
			if (mesh->getParent())
			mesh->getParent()->removeChild(mesh);
		});
	m_borderPointsRender.clear();
	m_borderLinesRender.clear();
}

bool BordersTab::BorderRec::onGui()
{
	bool isNeedToDelete = false;
	if (ImGui::Button("Delete border"))
	{
		isNeedToDelete = true;
		if (!borderInstId.empty())
			BORDER_CTRL.removeBorder(borderInstId);
		if (!triggerInstId.empty())
			BORDER_CTRL.removeTrigger(triggerInstId);
	}
	ImGui::InputText("Border Name", &borderRecName, ImGuiInputTextFlags_None); //border record name edit
	NDGui::Auto<BorderType>("Border Type", m_points.border_type);
	onGuiBorderInstancing();

	// Border rendering and color
	if (ImGui::Checkbox("Draw Border", &isRendered))
		showBorders(isRendered);
	if (isRendered)
	{
		ImGui::SameLine();
		if (ImGui::ColorEdit("Border color", borderColor))
		{
			SAFE_PARAM(m_renderMaterial, "color", borderColor);
		}
	}

	if (ImGui::TreeNode("Points"))
	{
		std::optional<int> removePointIndex;
		//rendering menus for existing border points
		if (!m_points.points.empty())
		{
			for (size_t i = 0; i < m_points.points.size(); i++) //border points list render
			{
				//renders border point menu and implements its functionality:
				onGuiPoint(i, removePointIndex);
			}
		}

		//This scrolls down list of points after adding a new one (for convenience). For some reason takes 3 frames to draw new list element and recalculate scrollbar max Y
		if (m_needScrollDownNextFrame > 0)
		{
			m_needScrollDownNextFrame--;
			ImGui::SetScrollY(ImGui::GetScrollMaxY());
		}

		//add new point to the border
		if (ImGui::Button("Add new point"))
		{
			addBorderPoint();
		}
		ImGui::SameLine();
		ImGui::Text("%s", "...or click with CTRL on map");

		//border points removing mechanism
		if (removePointIndex.has_value())
		{
			removeBorderPoint(*removePointIndex);
		}
		ImGui::TreePop();
	}

	return isNeedToDelete;
}

void BordersTab::BorderRec::addBorderPoint(vec2 newPointPos)
{
	//where new point will be placed
	if (newPointPos == INVALIDPOS.withoutZ())
	{
		if (!m_points.points.empty())
			newPointPos = m_points.points.back();
		else
			newPointPos = Render::getScreenCamera()->getWorldTranslation().withoutZ();
	}
	m_points.points.push_back(newPointPos);

	m_needScrollDownNextFrame = 3;
	//auto newPointName = borderRecord->borderName + "_" + STR(borderRecord->points.size() - 1);
	auto newPointName = m_points.borderName + "_" + STR(m_points.points.size() - 1);
	auto newPointMesh = Mesh::create::plane(vec2(0.1, 0.1));
	newPointMesh->setName(newPointName);
	newPointMesh->setMaterialInst(Material::getDefault()->createInstance());
	if (!m_localRefNode.expired())
		m_localRefNode.lock()->addChild(newPointMesh);
	else //fallback node
		BorderFile::m_defaultBorderNode->addChild(newPointMesh);
	newPointMesh->setRenderOrder(10000);
	newPointMesh->setEnabled(isRendered);

	m_borderPointsRender.push_back(newPointMesh);
	m_borderPointsRender.back()->setWorldTranslation(vec3(m_points.points.back().x, m_points.points.back().y, -0.001f));
	debug("[BordersTab::BorderRec::addBorderPoint]\nAdded %s border point", newPointName.c_str());

	updateBorderRender();
}

BordersTab::TabBorderLineData BordersTab::BorderRec::getData()
{
	TabBorderLineData copyForExport = m_points;
	copyForExport.borderName = borderRecName;
	if (isExportedInLocal && !m_localRefNode.expired())
	{
		auto rootPos = m_localRefNode.lock()->getWorldTranslation();
		for (auto& point : copyForExport.points)
			point -= rootPos.withoutZ();
	}
	return copyForExport;
}

void BordersTab::BorderRec::showBorders(bool show)
{
	isRendered = show;
	for (auto& point : m_borderPointsRender)
		point->setEnabled(isRendered);
	for (auto& line : m_borderLinesRender)
		line->setEnabled(isRendered);
}

void BordersTab::BorderRec::init()
{
	m_renderMaterial = RMat("sprite")->createInstance();
	m_renderMaterial->enableBlending(true);
	m_renderMaterial->setBlendFunc(BlendFactor::SRC_ALPHA, BlendFactor::ONE_MINUS_SRC_ALPHA);
	m_renderMaterial->setTexture(TextureId::TEXTURE_0, Texture::createWithColor(color::White));
	SAFE_PARAM(m_renderMaterial, "color", borderColor);
	if (!BorderFile::m_defaultBorderNode)
	{
		BorderFile::m_defaultBorderNode = M_SPTR<Node>("BorderTab_defaultBorderRenderRoot");
		Render::getHudPass().getRoot()->addChild(BorderFile::m_defaultBorderNode);
	}
	m_localRefNode = BorderFile::m_defaultBorderNode;

	//generating border points
	auto root = m_localRefNode.lock();
	if (!root) {ETRACE;}
	else if (!root->getParent()) {WTRACE;}

	int i = 0;
	for (auto& borderPoint : m_points.points)
	{
		auto newPointName = m_points.borderName + "_" + STR(i);
		auto newPointMesh = Mesh::create::plane(vec2(0.1, 0.1));
		newPointMesh->setName(newPointName);
		newPointMesh->setMaterialInst(Material::getDefault()->createInstance());
		root->addChild(newPointMesh);
		newPointMesh->setRenderOrder(10000);

		m_borderPointsRender.push_back(newPointMesh);
		m_borderPointsRender[i]->setWorldTranslation(vec3(borderPoint.x, borderPoint.y, -0.001f));
		// debug("[BordersTab::BorderRec::init]\nAdded %s border point. \nMesh parent: %s",
		// 	(m_points.borderName + "_" + STR(i)).c_str(),
		// 	w4::logger::logNode(newPointMesh->getParent()).c_str());

		i++;
	}
	// generating border lines
	updateBorderRender();
}

void BordersTab::BorderRec::updateBorderRender()
{
	if (m_points.points.size() < 2)
		return;

	//clearing old render or borders
	for (auto& mesh : m_borderLinesRender)
	{
		if(mesh)
			mesh->getParent()->removeChild(mesh);
	}
	m_borderLinesRender.clear();
	//rendering new borderlines
	int i = 0;
	std::string debugOutput = "\nBorders: ";

	for (const auto& border : m_points.points)
	{
		std::string borderPointName = m_points.borderName + "_" + STR(i);

		vec3 pos = vec3(border.x, border.y, 0.0);

		debugOutput += "\n[";
		debugOutput += STR(i);
		debugOutput += "] ";
		debugOutput += STR(border);

		vec3 prevPos = pos;
		vec2 prevPointCoords = (
			i == 0
			? m_points.points[m_points.points.size() - 1]
			: m_points.points[i - 1]);

		prevPos.x = prevPointCoords.x;
		prevPos.y = prevPointCoords.y;

		//debug("\nDrawing line [%d] from \n%s \nto \n%s", i, CSTR(prevPos), CSTR(pos));

		auto line = Mesh::create::plane();
		line->setName(borderRecName + "_line_" + STR(i));
		m_borderPointsRender[i]->addChild(line);
		line->setEnabled(isRendered);
		// auto mat = RMat("sprite")->createInstance();
		// color _color = color(1.f, 1.f, 0.f, 0.7);
		// mat->setTexture(TextureId::TEXTURE_0, Texture::createWithColor(_color));
		line->setMaterialInst(m_renderMaterial);
		m_borderLinesRender.push_back(line);

		auto middle = (pos + prevPos) / 2;
		middle.z = -0.001;

		auto vecRot = line->getWorldRotation().toVector();
		vecRot.z = atan2(prevPos.y - middle.y, prevPos.x - middle.x);

		auto scale = vec3((pos - prevPos).length(), 0.05f, 1.0f);

		line->setWorldScale(scale);
		line->setRenderOrder(10000 - 1);
		line->setWorldTranslation(middle);
		line->setWorldRotation(Rotator(vecRot));

		++i;
	}
}

void BordersTab::BorderRec::onGuiBorderInstancing()
{
	// since we can't remove and re-add borders/triggers in one update, we defer this operation
	// here is implemented deferred action
	if (m_deferredBorderReAdd)
	{
		BorderLineData data;
		data.borderName = borderRecName;
		data.points = m_points.points;
		sptr<Border> newBorder = make::sptr<Border>(data.border_type, data, BORDER_CTRL.getBorderCtrlRoot());
		borderInstId = BORDER_CTRL.addBorderObject(newBorder, borderInstId)->getUpdatableName();
		m_deferredBorderReAdd = false;
	}
	if (m_deferredTriggerReAdd)
	{
		BorderLineData data;
		data.borderName = borderRecName;
		data.points = m_points.points;
		sptr<Border> newBorder = make::sptr<Border>(BorderType::TriggerZone, data, BORDER_CTRL.getBorderCtrlRoot());
		triggerInstId = BORDER_CTRL.addTriggerObject(newBorder, triggerInstId)->getUpdatableName();
		m_deferredTriggerReAdd = false;
	}

	// Instancing and updating of already instanced border using this BorderRec
	if (borderInstId.empty() && m_points.points.size() > 1)
	{
		if (ImGui::Button("Instantiate as Border"))
		{
			BorderLineData data;
			data.borderName = borderRecName;
			data.points = m_points.points;
			sptr<Border> newBorder = make::sptr<Border>(data.border_type, data, BORDER_CTRL.getBorderCtrlRoot());
			borderInstId = BORDER_CTRL.addBorderObject(newBorder, borderInstId)->getUpdatableName();
		}
	}
	else if (!borderInstId.empty())
	{
		ImGui::Columns(2);
		ImGui::SetColumnWidth(-1, ImGui::GetWindowSize().x * 0.75);
		ImGui::InputText("Border Instance Name", &borderInstId, ImGuiInputTextFlags_ReadOnly);
		ImGui::NextColumn();
		if (ImGui::Button("Update Border"))
		{
			BORDER_CTRL.removeBorder(borderInstId);
			m_deferredBorderReAdd = true;
		}
		ImGui::Columns(1);
	}

	// Instancing and updating of already instanced trigger using this BorderRec
	if (triggerInstId.empty() && m_points.points.size() > 1)
	{
		if (ImGui::Button("Instantiate as Trigger"))
		{
			BorderLineData data;
			data.borderName = borderRecName;
			data.points = m_points.points;
			sptr<Border> newBorder = make::sptr<Border>(BorderType::TriggerZone, data, BORDER_CTRL.getBorderCtrlRoot());
			triggerInstId = BORDER_CTRL.addTriggerObject(newBorder, triggerInstId)->getUpdatableName();
		}
	}
	else if (!triggerInstId.empty())
	{
		ImGui::Columns(2);
		ImGui::SetColumnWidth(-1, ImGui::GetWindowSize().x * 0.75);
		ImGui::InputText("Trigger Instance Name", &triggerInstId, ImGuiInputTextFlags_ReadOnly);
		ImGui::NextColumn();
		if (ImGui::Button("Update Trigger"))
		{
			BORDER_CTRL.removeTrigger(triggerInstId);
			m_deferredTriggerReAdd = true;
		}
		ImGui::Columns(1);
	}
}

void BordersTab::BorderRec::onGuiPoint(size_t idx, std::optional<int>& removePointIndex)
{
	std::string meshSearchName = borderRecName + "_" + STR(idx);

	if (ImGui::TreeNode(meshSearchName.c_str()))
	{
		//point deletion button
		ImGui::SameLine();
		if (ImGui::Button("x"))
		{
			removePointIndex = idx;
		}

		//Drag gizmo
		if (m_borderPointsRender.size() > idx	&& m_borderPointsRender[idx])
		{
			//Gizmo
			ND_Util::NoConflictGizmo(m_borderPointsRender[idx], meshSearchName);

			if (m_borderPointsRender[idx]->getWorldTranslation().withoutZ() != m_points.points[idx])
			{
				m_points.points[idx] = m_borderPointsRender[idx]->getWorldTranslation().withoutZ();
				updateBorderRender();
			}
		}
		//else
		//	debug("[BorderTab]\nNo %s border point", meshSearchName.c_str());

		//border point position
		if (ImGui::DragVec2(("Pos" + STR(idx)).c_str(), m_points.points[idx]))
		{
			if (m_borderPointsRender.size() > idx	&& m_borderPointsRender[idx])
			{
				//updating mesh position
				m_borderPointsRender[idx]->setWorldTranslation(vec3(m_points.points[idx].x, m_points.points[idx].y, -0.001f));
				updateBorderRender();
			}
		};

		ImGui::TreePop();
	}
}

void BordersTab::BorderRec::removeBorderPoint(int idx)
{
	if (m_borderPointsRender.size() <= idx)
	{
		err("[BordersTab::BorderRec::removeBorderPoint] wrong index %d for %d points meshes", idx, m_borderPointsRender.size());
		return;
	}

	auto pointMesh = m_borderPointsRender[idx];
	if (pointMesh)
	{
		if (pointMesh->getParent())
			pointMesh->getParent()->removeChild(pointMesh);
	}
	m_borderPointsRender.erase(m_borderPointsRender.begin() + idx);

	//removing point from border record
	m_points.points.erase(m_points.points.begin() + idx);
	updateBorderRender();
}

BordersTab::BorderFile::BorderFile(TabBorderContainer& borderData,
                                   const std::string& _borderFileName,
                                   const std::string& _borderFilePath)
	:
	borderFileName(_borderFileName),
	borderFilePath(_borderFilePath)
{
	m_borders.reserve(borderData.borders.size() * 2);
	for (auto& border : borderData.borders)
		m_borders.emplace_back(border);
	m_selectedBorder = m_borders.end();
}

BordersTab::BorderFile::BorderFile(const std::string& _borderFileName)
	:
	borderFileName(_borderFileName)
{
	m_borders.reserve(10);
	m_selectedBorder = m_borders.end();
}

bool BordersTab::BorderFile::onGui()
{
	bool isChanged = false;
	if (ImGui::Checkbox("Draw Borders", &isRendered))
	{
		isChanged = true;
		for (auto& border : m_borders)
			border.showBorders(isRendered);
	}

	// if m_needRefocusInputField == true - need to re-focus "Border File Name" input field
	// ImGuiInputTextFlags_CallbackAlways flag needed for re-focusing to fix SetKeyboardFocusHere() selecting all text and resetting cursor pos
	ImGuiInputTextFlags_ textFlags = m_needRefocusInputField ? ImGuiInputTextFlags_CallbackAlways : ImGuiInputTextFlags_CallbackEdit;
	if (m_needRefocusInputField)
		ImGui::SetKeyboardFocusHere();

	//border file name edit text field
	isChanged = isChanged || ImGui::InputText("Border File Name",
				&borderFileName,
				textFlags,
		[](ImGuiInputTextCallbackData* data) -> int
				{
					auto self = reinterpret_cast<BordersTab::BorderFile*>(data->UserData);
					//field text edited - need to refocus it next frame (because tab changed its name)!
					if (self && !self->m_needRefocusInputField && data->EventFlag == ImGuiInputTextFlags_CallbackEdit)
					{
						self->m_needRefocusInputField = true;
						self->m_refocusInputFieldCursorPos = data->CursorPos;
					}
					//ImGui::SetKeyboardFocusHere used for re-selection selects all text and resets cursor position. Fixing it!
					else if (self && self->m_needRefocusInputField)
					{
						self->m_needRefocusInputField = false;
						data->ClearSelection();
						data->CursorPos = std::clamp(self->m_refocusInputFieldCursorPos, 0, data->BufTextLen);
					}
					return 0;
				},
				this); //we get this from ImGuiInputTextCallbackData->UserData - can't capture it in lambda because otherwise won't be able to cast to ImGuiInputTextCallback!
	isChanged = isChanged || ImGui::IsItemEdited();

	if (ImGui::Button("Export to file"))
		exportToFile();

	ImGui::Separator();
	ImGui::Columns(2);
	ImGui::SetColumnWidth(-1, ImGui::GetWindowSize().x * 0.75);
	if (ImGui::InputText("New Border Name", &m_newBorderName, ImGuiInputTextFlags_EnterReturnsTrue))
		addBorder(m_newBorderName); //called only on enter
	ImGui::NextColumn();
	ImGui::Separator();
	if (ImGui::Button("Add"))
		addBorder(m_newBorderName);
	ImGui::Columns(1);
	ImGui::Separator();

	if (m_needUpdateCache) //update list of BorderRec as iterators
	{
		m_cachedBorderList.clear();
		m_cachedBorderList.reserve(m_borders.size()*2);
		for (auto It = m_borders.begin(); It != m_borders.end(); ++It)
			m_cachedBorderList.push_back(It);
		m_needUpdateCache = false;
	}

	// List box containing all BorderRec from the file
	std::pair<bool, BorderRecIt> result =
		ND_Util::ImGuiListBox<BordersTab::BorderFile::BorderRecIt>("Borders",
			m_cachedBorderList, //cached list of iterators to BorderRecs
			((m_selectedBorder == m_borders.end() )? std::string("") : (*m_selectedBorder).borderRecName), // selected record name
		[this](const auto& item) //rec name getter
			{
				return (*item).borderRecName;
			},
		[this](const auto& item) //RMB pop-up window logic with delete btn
			{
				ImGui::Text("%s", (*item).borderRecName.c_str());
				ImGui::Separator();
				if (ImGui::Button("Delete"))
				{
					removeBorder(item);
					return true;
				}
				return false;
			});

	if (result.first)
	{
		isChanged = true;
		m_selectedBorder = result.second; //new border record selected
	}
	if (m_selectedBorder == m_borders.end()) //early exit - no border selected
	{
		ImGui::TextColored(IMG_RED, "%s", "No border selected!");
		return isChanged;
	}

	// Rendering data of the selected BorderRec in a separate scrollable zone
	if (ImGui::BeginChild((*m_selectedBorder).borderRecName.c_str(), ND_Util::getSizeForChild(0.4), true))
	{
		if ((*m_selectedBorder).onGui()) //rendering selected border record
		{
			isChanged = true;
			removeBorder(m_selectedBorder);
		}
	}
	ImGui::EndChild();

	return isChanged;
}

void BordersTab::BorderFile::exportToFile()
{
	TabBorderContainer borderContainer;
	for (auto& border : m_borders)
	{
		borderContainer.borders.push_back(border.getData());
	}

	nlohmann::json jsonForExport;
	nlohmann::adl_serializer<TabBorderContainer>::to_json(jsonForExport, borderContainer);
	std::string flatJson = nlohmann::jsonUnflatten(jsonForExport);
	//ImGui::SetClipboardText(flatJson.c_str());
	jsWriteEvent(flatJson.c_str(), (borderFileName + ".json").c_str());
}

void BordersTab::BorderFile::addBorder(const std::string& borderName)
{
	auto selectedBorderIdx = std::distance(m_borders.begin(), m_selectedBorder);  //emplace might invalidate our iterator - storing index

	//checking for unique name
	int optionalSuffix = 0;
	static std::function<bool(const std::string&, int&)> uniqueNamesChecker;
	uniqueNamesChecker = [this, originalName = borderName](const std::string& borderName, int& optionalSuffix) -> bool
		{
			for (const auto& border : m_borders)
			{
				if (border.borderRecName == borderName)
				{
					optionalSuffix++;
					if (!uniqueNamesChecker(originalName + "_" + STR(optionalSuffix), optionalSuffix))
						return false;
				}
			}
			return false;
		};
	uniqueNamesChecker(borderName, optionalSuffix);

	//adding new border
	if (optionalSuffix == 0)
		m_borders.emplace_back(borderName);
	else
		m_borders.emplace_back(borderName+ "_" + STR(optionalSuffix));

	m_selectedBorder = m_borders.begin() + selectedBorderIdx;
	m_needUpdateCache = true;
}

void BordersTab::BorderFile::removeBorder(BorderRecIt borderIterator)
{
	if (m_selectedBorder == m_borders.end())
		return;

	if (borderIterator == m_selectedBorder)
	{
		m_borders.erase(borderIterator);
		m_needUpdateCache = true;
		return;
	}

	auto selectedBorderName = (*m_selectedBorder).borderRecName; //erase might invalidate our iterator - will look up new Iterator later
	m_borders.erase(borderIterator);
	m_selectedBorder = std::find_if(m_borders.begin(), m_borders.end(),
		[selectedBorderName](const auto& border){return border.borderRecName == selectedBorderName;});
	m_needUpdateCache = true;
}

void BordersTab::BorderFile::clickAddBorderPoint(vec2 newPointPos)
{
	if (m_selectedBorder == m_borders.end())
		return;
	(*m_selectedBorder).addBorderPoint(newPointPos);
}

void BordersTab::onGuiCall()
{
	m_tabNamesChecker.clear();
	std::optional<int> closeFileRequest = std::nullopt;
	auto tabBarFlags = ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_FittingPolicyResizeDown;
	if (ImGui::BeginTabBar("Border Tab Bar", tabBarFlags))
	{
		for (int i = 0; i < m_borderFiles.size(); ++i)
		{
			auto& borderFile = m_borderFiles[i];
			static bool tabCloseBtnFlag;
			tabCloseBtnFlag = true; //this flag will be flipped to false if use pressed 'x' btn on the tab
			if (ImGui::BeginTabItem(checkTabNameUnique(borderFile.borderFileName), &tabCloseBtnFlag, ImGuiTabItemFlags_None))
			{
				m_selectedTabBar = i;
				if (borderFile.onGui())
				{
					//m_selectedTabBar = i;
				}
				ImGui::EndTabItem();
			}
			if (!tabCloseBtnFlag) closeFileRequest = i; //queueing file-tab close request

		}

		//tab-button for loading file with borders
		if (ImGui::TabItemButton("Load", ImGuiTabItemFlags_NoCloseWithMiddleMouseButton | ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoReorder))
		{
			loadJson();
		}
		//tab-button for creating new empty 'file' with borders
		if (ImGui::TabItemButton("+", ImGuiTabItemFlags_NoCloseWithMiddleMouseButton | ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoReorder))
		{
			m_borderFiles.emplace_back("NewBorderFile" + STR(m_borderFiles.size()));
		}

		ImGui::EndTabBar();
	}

	if (closeFileRequest.has_value())
	{
		m_borderFiles.erase(m_borderFiles.begin() + *closeFileRequest);
	}
}

void BordersTab::addItem(std::any item, const std::string& altName)
{
}

void BordersTab::loadJson()
{
	jsReadJsonEx(this, &BordersTab::onJSONLoadCB);

	if (!m_onJsonLoaded)
	{
		m_onJsonLoaded = [this](nlohmann::json loadedJson)
			{
				//W4_LOG_INFO("[BorderTab]\nParsed json: %s", loadedJson.dump().c_str());
				W4_LOG_INFO("[BorderTab]\nLoad json file %s from %s", m_lastLoadedFileName.c_str(), m_lastLoadedFilePath.c_str());
				TabBorderContainer borderContainer;
				nlohmann::adl_serializer<TabBorderContainer>::from_json(loadedJson, borderContainer);

				m_borderFiles.emplace_back(
					borderContainer,
					m_lastLoadedFileName.substr(0, m_lastLoadedFileName.find(".json")),
					m_lastLoadedFilePath);
			};
	}
}

void BordersTab::onKeyPressed(const event::Keyboard::Down& evt)
{
	if (!isActiveTab())
		return;

	if (evt.key == w4::platform::KeyboardKey::Control && !m_clickAddModeON)
		m_clickAddModeON = true;
}

void BordersTab::onKeyReleased(const event::Keyboard::Up& evt)
{
	if (evt.key == w4::platform::KeyboardKey::Control && m_clickAddModeON)
		m_clickAddModeON = false;
}

void BordersTab::onTouchBegin(const Touch::Begin& evt)
{
	if (!isActiveTab())
		return;

	if (!m_clickAddModeON || m_selectedTabBar == -1 || m_selectedTabBar >= m_borderFiles.size())
		return;

	auto clickPos = getMouseWorldPos();
	m_borderFiles[m_selectedTabBar].clickAddBorderPoint(clickPos.withoutZ());
}

const char* BordersTab::checkTabNameUnique(const std::string& name)
{
	if (m_tabNamesChecker.contains(name))
	{
		for (int i = 0; i < 100; ++i)
		{
			if (!m_tabNamesChecker.contains(name + "[" + STR(i) + "]"))
			{
				return (*m_tabNamesChecker.emplace(name + "[" + STR(i) + "]").first).c_str(); //add altered name to checker and return it as cost char*
			}
		}
		err("Failed to build unique name!");
		return "ERROR";
	}
	else
	{
		m_tabNamesChecker.emplace(name);
		return name.c_str();
	}
}

#endif
