#include "PathdrawerTab.h"
#include "../ND_JsonTools.h"

std::function<void(nlohmann::json)> PathdrawerTab::m_onJsonLoaded = nullptr;

#ifndef FORCENODEDEBUG_OFF

void PathdrawerTab::onGuiCall()
{
	if (ImGui::Button("Import paths from JSON"))
	{
		loadJson();
	}
	ImGui::SameLine();
	if (ImGui::Button("Export paths to JSON"))
	{
		nlohmann::json exportJson; 
		auto pathDataCleared = m_pathsData;
		//removing empty paths
		std::erase_if(pathDataCleared.paths, [this](const plottedPath& pathRec) {
			return pathRec.midPoints.empty() && pathRec.startPoint == "" && pathRec.endPoint == "";
		});
		nlohmann::adl_serializer<PathsData>::to_json(exportJson, pathDataCleared);
		exportAsJson(exportJson);
	}
	ImGui::Spacing();
	ImGui::Separator();

	if (ImGui::BeginTabBar("Path editor tabbar"), ImGuiTabBarFlags_::ImGuiTabBarFlags_Reorderable)
	{

		if (ImGui::BeginTabItem("Path Nodes"))
		{
			m_activeSubtab = Subtab::PathNodes;
			onGuiPathNodesTab();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Paths editor"))
		{
			m_activeSubtab = Subtab::PathBuilder;
			onGuiPathEditorTab();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Search Node Paths"))
		{
			m_activeSubtab = Subtab::NodePathSearch;
			onGuiPathSearchTab();
			ImGui::EndTabItem();
		}
		else
		{
			if (m_selectedSearchNode != "No Node Selected" && m_pathNodeRender[m_selectedSearchNode])
				m_pathNodeRender[m_selectedSearchNode]->getMaterialInst()->setParam("color", vec4(1.f, 0.f, 0.f, 1.f));
		}
	}
	ImGui::EndTabBar();
}

void PathdrawerTab::onGuiPathNodesTab()
{
	bool bNeedRemovePoint = false;
	std::string removePointName = "";

	if (ImGui::BeginChild("Path nodes scrollable zone", ND_Util::getSizeForChild(0.4)))
	{
		//List of path nodes + gizmo to move them
		for (auto& [nodeName, pathNode] : m_pathsData.pathnodes)
		{
			if (ImGui::TreeNode(nodeName.c_str()))
			{
				//point deletion button
				ImGui::SameLine();
				if (ImGui::Button("x"))
				{
					bNeedRemovePoint = true;
					removePointName = nodeName;
				}

				//Drag gizmo
				if (m_pathNodeRender.contains(nodeName)
					&& m_pathNodeRender[nodeName])
				{
					ND_Util::NoConflictGizmo(m_pathNodeRender[nodeName], nodeName);
					if (m_pathNodeRender[nodeName]->getWorldTranslation().withoutZ() != pathNode.pos)
					{
						pathNode.pos = m_pathNodeRender[nodeName]->getWorldTranslation().withoutZ();
						if (m_selectedPathIndex != -1)
							renderPathLines(m_pathsData.paths[m_selectedPathIndex]);
					}
				}

				//Path node position
				if (ImGui::DragVec2(nodeName.c_str(), pathNode.pos))
				{
					if (m_pathNodeRender.contains(nodeName)
						&& m_pathNodeRender[nodeName])
					{
						//updating mesh position
						m_pathNodeRender[nodeName]->setWorldTranslation(vec3(pathNode.pos.x, pathNode.pos.y, -0.001f));
						if (m_selectedPathIndex != -1)
							renderPathLines(m_pathsData.paths[m_selectedPathIndex]);
					}
				};

				ImGui::InputFloat("Speed", &pathNode.speed);

				ImGui::TreePop();
			}
		}
	}
	ImGui::EndChild(); //ends path nodes list scrollable zone

	//removing deleted PathNode
	if (bNeedRemovePoint)
	{
		removePathNode(removePointName);
	}

	//New path nodes adder
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Columns(2);
	//ImGui::SetKeyboardFocusHere();
	//New node name input field. Creates new node upon hitting Enter
	if(ImGui::InputText("Name", &m_newPathNodeNaming, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		//Called on pressing enter
		if (m_newPathNodeNaming == "" || m_newPathNodeNaming.empty() || m_pathsData.pathnodes.contains(m_newPathNodeNaming))
			return;

		vec2 newPointPos = Render::getScreenCamera()->getWorldTranslation().withoutZ();
		addPathNode(m_newPathNodeNaming, newPointPos, m_nodeDefaultSpeed);
	}
	ImGui::NextColumn();
	//Button for adding new node
	if (ImGui::Button("Add new path node"))
	{
		if (m_newPathNodeNaming == "" || m_newPathNodeNaming.empty() || m_pathsData.pathnodes.contains(m_newPathNodeNaming))
			return;

		vec2 newPointPos = Render::getScreenCamera()->getWorldTranslation().withoutZ();
		addPathNode(m_newPathNodeNaming, newPointPos, m_nodeDefaultSpeed);
	}
	ImGui::Columns(1);
	ImGui::DragFloat("Default node speed", &m_nodeDefaultSpeed, 0.1, 0.001);
}

void PathdrawerTab::onGuiPathEditorTab()
{
	//====Path selector====
	if (m_pathsData.paths.empty())
	{
		int placeholderId = 0;
		const char* placeholderPathName = { "No paths" };
		ImGui::Combo("<-Selected Path", &placeholderId, placeholderPathName, 1);
	}
	else
		coloredPathCombo(); //List of paths. Unfinished paths are colored red

	if (m_pathsData.pathnodes.empty())
	{
		ImGui::Text("No path nodes created! \nCan't edit path without path nodes! \nCreate some in <Path Nodes> tab!");
		return;
	}

	//====Editor for selected path====
	selectedPathEditor();

	//====New path adder====
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Separator();

	ImGui::Columns(2);
	ImGui::InputText("<- New path name", &m_newPathNaming);
	ImGui::NextColumn();
	if (ImGui::Button("Create new path"))
	{
		plottedPath newPath{ .pathName = m_newPathNaming };
		m_pathsData.paths.push_back(newPath);
	}
	ImGui::Columns(1);

	//====Paths auto generator====
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Columns(2);
	if (ImGui::Button("Autogenerate paths"))
	{
		tryBuildDefaultPaths();
	}
	ImGui::NextColumn();
	ImGui::Separator();
	if (ImGui::InputInt("Max paths per node", &m_autoPathPerNode, 1))
	{
		if (m_autoPathPerNode < 1)
			m_autoPathPerNode = 1;
	}
	ImGui::Columns(1);
}

void PathdrawerTab::renderPathLines(const plottedPath& pathRecordToRedraw, bool forceRedrawColor)
{
	if (pathRecordToRedraw.startPoint == "" || pathRecordToRedraw.endPoint == "")
	{
		for (auto& mesh : m_pathLinesRender[pathRecordToRedraw.pathName])
		{
			if (mesh)
				mesh->getParent()->removeChild(mesh);
		}
		return;
	}

	bool reRenderSelectedPath = false;
	if (m_selectedPathIndex != -1 && m_pathsData.paths[m_selectedPathIndex].pathName == pathRecordToRedraw.pathName)
		reRenderSelectedPath = true;
	if (forceRedrawColor)
		reRenderSelectedPath = false;

	//clearing old render of path
	for (auto& mesh : m_pathLinesRender[pathRecordToRedraw.pathName])
	{
		if (mesh)
			mesh->getParent()->removeChild(mesh);
	}
	m_pathLinesRender.erase(pathRecordToRedraw.pathName);
	m_pathLinesRender.insert({ pathRecordToRedraw.pathName, std::vector<sptr<Mesh>>() });

	//rendering new borderlines
	int i = 0;
	std::string debugOutput = "\nBorders: ";

	//generating pathNodes array
	std::vector<std::string> pathPoints;
	pathPoints.push_back(pathRecordToRedraw.startPoint);

	if (!pathRecordToRedraw.midPoints.empty())
		for (auto& pointName : pathRecordToRedraw.midPoints)
			if(pointName != "") 
				pathPoints.push_back(pointName);

	pathPoints.push_back(pathRecordToRedraw.endPoint);

	//rendering path lines
	for (const auto& nodeName : pathPoints)
	{
		if (!m_pathsData.pathnodes.contains(nodeName))
		{
			warn("No path node named %s!", nodeName.c_str());
			return;
		}

		vec3 nodePos{ m_pathsData.pathnodes[nodeName].pos.x, m_pathsData.pathnodes[nodeName].pos.y, 0.0 };
		//m_pathNodeRender[borderPointName]->removeAllChildren();

		debugOutput += "\n[";
		debugOutput += STR(i);
		debugOutput += "] ";
		debugOutput += CVEC(nodePos);

		vec3 prevPos = nodePos;
		vec2 prevPointCoords = (
			i == 0
			? nodePos.withoutZ()
			: m_pathsData.pathnodes[pathPoints[i - 1]].pos);

		prevPos.x = prevPointCoords.x;
		prevPos.y = prevPointCoords.y;

		//debug("\nDrawing line [%d] from \n%s \nto \n%s", i, CSTR(prevPos), CSTR(nodePos));

		auto line = Mesh::create::plane();
		line->setName("line_" + STR(i));
		m_pathNodeRender[nodeName]->addChild(line);
		auto mat = RMat("sprite")->createInstance();
		color _color = color(0.f, 0.f, 1.f, 0.8);
		if(reRenderSelectedPath)
			_color = color(0.f, 1.f, 0.f, 0.8);
		mat->setTexture(TextureId::TEXTURE_0, Texture::createWithColor(_color));
		mat->enableBlending(true);
		line->setMaterialInst(mat);
		m_pathLinesRender[pathRecordToRedraw.pathName].push_back(line);

		auto middle = (nodePos + prevPos) / 2;
		middle.z = -0.001;

		auto vecRot = line->getWorldRotation().toVector();
		vecRot.z = atan2(prevPos.y - middle.y, prevPos.x - middle.x);

		auto scale = vec3((nodePos - prevPos).length(), 0.1f, 1.0f);

		line->setWorldScale(scale);
		line->setRenderOrder(9000 - 1 + (reRenderSelectedPath ? 100 : 0));
		line->setWorldTranslation(middle);
		line->setWorldRotation(Rotator(vecRot));

		++i;
	}

	//info("%s", debugOutput.c_str());
}

void PathdrawerTab::exportAsJson(nlohmann::json& inJson)
{
	std::string flatJson = nlohmann::jsonUnflatten(inJson);
	ImGui::SetClipboardText(flatJson.c_str());
	jsWriteEvent(flatJson.c_str(), "pathsContainerSettings.json");
}

void PathdrawerTab::loadJson()
{
	jsReadJson(this, &PathdrawerTab::onJSONLoadCB);

	if (!m_onJsonLoaded)
	{
		//executed on file load:
		m_onJsonLoaded = [this](nlohmann::json loadedJson){
				W4_LOG_INFO("[PathdrawerTab]\nParsed json: %s", loadedJson.dump().c_str());
				nlohmann::adl_serializer<PathsData>::from_json(loadedJson, m_pathsData);

				for (auto& [pathNodeName, pathNode] : m_pathsData.pathnodes)
				{
					renderPathNode(pathNodeName, pathNode);
				}

				for (auto& pathRecord : m_pathsData.paths)
				{
					renderPathLines(pathRecord);
				}

		};
	}
}

void PathdrawerTab::selectPathLine(const std::string& pathName)
{
	if (m_lastSelectedPathName != "")
	{
		for (auto& [name, lineMeshes] : m_pathLinesRender)
		{
			for (auto& line : lineMeshes)
			{
				auto mat = RMat("sprite")->createInstance();
				color _color = color(0.f, 0.f, 1.f, 0.7);
				mat->setTexture(TextureId::TEXTURE_0, Texture::createWithColor(_color));
				line->setMaterialInst(mat);
				line->setRenderOrder(9000 - 1);
			}
		}
	}

	if (!m_pathLinesRender.contains(pathName))
		return;

	m_lastSelectedPathName = pathName;
	for (auto& line : m_pathLinesRender[pathName])
	{
		auto mat = RMat("sprite")->createInstance();
		color _color = color(0.f, 1.f, 0.f, 0.7);
		mat->setTexture(TextureId::TEXTURE_0, Texture::createWithColor(_color));
		line->setMaterialInst(mat);
		line->setRenderOrder(9100);
	}
}

void PathdrawerTab::renderPathNode(const std::string name, const pathNode& nodeData)
{
	//instansing and setting up new Path Node mesh
	auto newPointName = name;
	auto newPointMesh = Mesh::create::plane(vec2(0.25, 0.25));
	newPointMesh->setName(name);
	auto mat = RMat("sprite")->createInstance();
	color _color = color(1.f, 1.f, 1.f, 1.f);
	mat->setTexture(TextureId::TEXTURE_0, Texture::createWithColor(_color));
	mat->setParam("color", vec4(1.f, 0.f, 0.f, 0.7));
	mat->enableBlending(true);
	newPointMesh->setMaterialInst(mat);
	Render::getRoot()->addChild(newPointMesh);
	newPointMesh->setRenderOrder(9000);

	//path node collider
	auto collider = newPointMesh->addCollider<AABB>("node_collider", vec3(0.3, 0.3, 0.01));
	collider->setBlockScreencasts(false);
	collider->setBlockRaycasts(false);
	collider->setRaycastCallback(std::bind(&PathdrawerTab::pathNodeRaycastHandler, this, std::placeholders::_1, name));

	//adding new Path Node mesh
	m_pathNodeRender.insert(
		{
			newPointName,
			newPointMesh
		});
	m_pathNodeRender[newPointName]->setWorldTranslation(vec3(nodeData.pos.x, nodeData.pos.y, -0.002f));

	//redraw paths related to the Path Node on its mesh transform
	newPointMesh->subscribeToWorldTransformChanged(
		[this, name](const Transform& trans)
		{
			for (auto& path : m_pathsData.paths)
			{
				if (path.startPoint == name || path.endPoint == name || std::find(path.midPoints.begin(), path.midPoints.end(), name) != path.midPoints.end())
				{
					renderPathLines(path);
				}			
			}
		});
}


void PathdrawerTab::selectedPathEditor(int indexOverride)
{
	//========Path editor========
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Separator();

	int pathIndex = m_selectedPathIndex;
	if (indexOverride != -1 && indexOverride < m_pathsData.paths.size())
		pathIndex = indexOverride;

	if (pathIndex == -1 || m_pathsData.paths.empty()) //no path selected or created. Nothing else to render
		return;

	//pathname editor
	ImGui::InputText("Path name", &m_pathsData.paths[pathIndex].pathName);

	//Path nodes names, indices and etc used in path editor
	int pathNodesCount = m_pathsData.pathnodes.size();
	const char* paths_nodes[pathNodesCount];
	std::map<std::string, int> pathNodesIndices;
	auto pathNodesIt = m_pathsData.pathnodes.begin();
	for (size_t i = 0; i < pathNodesCount; i++)
	{
		paths_nodes[i] = pathNodesIt->first.c_str();
		pathNodesIndices.insert({ pathNodesIt->first, i });
		pathNodesIt++;
	}

	//Start point selector
	auto startPoint = m_pathsData.paths[pathIndex].startPoint;
	int startNodeSelectedIndex = -1;
	if (startPoint != "" && pathNodesIndices.contains(startPoint))
	{
		startNodeSelectedIndex = pathNodesIndices[startPoint];
	}

	if (ImGui::Combo("Path start point", &startNodeSelectedIndex, paths_nodes, pathNodesCount))
	{
		m_pathsData.paths[pathIndex].startPoint = paths_nodes[startNodeSelectedIndex];
		renderPathLines(m_pathsData.paths[pathIndex], true);
		selectPathLine(m_pathsData.paths[pathIndex].pathName);
	}

	//Midpoint selectors
	{
		int midIt = 0;
		bool bNeedRemovePoint = false;
		auto removeMidPointIt = m_pathsData.paths[pathIndex].midPoints.end();
		for (auto& midPoint : m_pathsData.paths[pathIndex].midPoints)
		{
			int midpointNodeSelectedIndex = -1;
			if (midPoint != "" && pathNodesIndices.contains(midPoint))
			{
				midpointNodeSelectedIndex = pathNodesIndices[midPoint];
			}

			std::string midPointID = "Path mid point " + STR(midIt);
			if (ImGui::Combo(midPointID.c_str(), &midpointNodeSelectedIndex, paths_nodes, pathNodesCount))
			{
				midPoint = paths_nodes[midpointNodeSelectedIndex];
				renderPathLines(m_pathsData.paths[pathIndex], true);
				selectPathLine(m_pathsData.paths[pathIndex].pathName);
			}
			ImGui::SameLine();
			ImGui::PushID(midIt);
			if (ImGui::Button("x"))
			{
				bNeedRemovePoint = true;
				removeMidPointIt = (m_pathsData.paths[pathIndex].midPoints.begin() + midIt);
			}
			ImGui::PopID();
			midIt++;
		}
		//removing midpoints
		if (bNeedRemovePoint)
		{
			m_pathsData.paths[pathIndex].midPoints.erase(removeMidPointIt);
			renderPathLines(m_pathsData.paths[pathIndex], true);
			selectPathLine(m_pathsData.paths[pathIndex].pathName);
		}
	}

	//End point selector
	auto endPoint = m_pathsData.paths[pathIndex].endPoint;
	int endNodeSelectedIndex = -1;
	if (endPoint != "" && pathNodesIndices.contains(endPoint))
	{
		endNodeSelectedIndex = pathNodesIndices[endPoint];
	}

	if (ImGui::Combo("Path end point", &endNodeSelectedIndex, paths_nodes, pathNodesCount))
	{
		m_pathsData.paths[pathIndex].endPoint = paths_nodes[endNodeSelectedIndex];
		renderPathLines(m_pathsData.paths[pathIndex], true);
		selectPathLine(m_pathsData.paths[pathIndex].pathName);
	}

	//Midpoint adder
	if (ImGui::Button("Insert new path mid point"))
	{
		m_pathsData.paths[pathIndex].midPoints.push_back("");
	}

	//====Path deleter====
	if (!m_pathsData.paths.empty() && pathIndex != -1)
	{
		if (ImGui::Button(("Delete path [" + m_pathsData.paths[pathIndex].pathName + "]").c_str()))
		{
			std::erase_if(m_pathsData.paths, [this, pathIndex](plottedPath path)
				{
					if (path.pathName == m_pathsData.paths[pathIndex].pathName)
					{
						for (auto& mesh : m_pathLinesRender[path.pathName])
						{
							mesh->getParent()->removeChild(mesh);
							mesh.reset();
						}
						m_pathLinesRender.erase(path.pathName);
						m_lastSelectedPathName = "";
						return true;
					}
					else
						return false;
				});
			pathIndex = -1;
		}
	}
}

void PathdrawerTab::onGuiPathSearchTab()
{
	if (ImGui::BeginCombo("Select node", m_selectedSearchNode.c_str()))
	{
		for(auto& [nodeName, node] : m_pathsData.pathnodes)
		{
			bool isSelected = nodeName == m_selectedSearchNode;
			if (ImGui::Selectable(nodeName.c_str(), &isSelected))
			{
				if(m_pathNodeRender[nodeName])
					m_pathNodeRender[nodeName]->getMaterialInst()->setParam("color", vec4(1.f, 1.f, 0.f, 0.7));
				if(m_selectedSearchNode != "No Node Selected" && m_pathNodeRender[m_selectedSearchNode])
					m_pathNodeRender[m_selectedSearchNode]->getMaterialInst()->setParam("color", vec4(1.f, 0.f, 0.f, 0.7));
				m_selectedSearchNode = nodeName;

				m_selectedSearchResultIndex = -1;
				selectPathLine("__RESET_SELECTION__");
			}

			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	int i = 0;
	bool bOpenedPath = false;

	if(ImGui::BeginListBox("Paths with selected Node", ND_Util::getSizeForChild(0.35)))
	{
		for (auto& path : m_pathsData.paths)
		{
			if (path.startPoint == m_selectedSearchNode || path.endPoint == m_selectedSearchNode || std::find(path.midPoints.begin(), path.midPoints.end(), m_selectedSearchNode) != path.midPoints.end())
			{
				bool isSelected = i == m_selectedSearchResultIndex;
				if (ImGui::Selectable(path.pathName.c_str(), &isSelected))
				{
					m_selectedSearchResultIndex = i;
					selectPathLine(path.pathName);
				}
			}

			i++;
		}
	}
	ImGui::EndListBox();

	if (m_selectedSearchResultIndex != -1)
	{
		selectedPathEditor(m_selectedSearchResultIndex);
	}
}

void PathdrawerTab::coloredPathCombo()
{
	std::string selectedPointPreview = (m_selectedPathIndex == -1) ? "NONE" : m_pathsData.paths[m_selectedPathIndex].pathName;

	if (ImGui::BeginCombo("<-Selected Path", selectedPointPreview.c_str()))
	{
		for (size_t i = 0; i < m_pathsData.paths.size(); i++)
		{
			bool needColorReset = false;
			bool isSelected = i == m_selectedPathIndex;
			if (!checkPathComplete(m_pathsData.paths[i]))
			{
				ImGui::PushStyleColor(ImGuiCol_Header, IMG_RED);
				ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IMG_RED);
				ImGui::PushStyleColor(ImGuiCol_HeaderActive, IMG_RED);
				needColorReset = true;
				isSelected = true; //cheat to color the Selectable
			}

			if (ImGui::Selectable(m_pathsData.paths[i].pathName.c_str(), &isSelected))
			{
				m_selectedPathIndex = i;
				selectPathLine(m_pathsData.paths[m_selectedPathIndex].pathName);
				renderPathLines(m_pathsData.paths[m_selectedPathIndex]);
			}
			if(isSelected)
				ImGui::SetItemDefaultFocus();

			if (needColorReset)
				ImGui::PopStyleColor(3);
		}

		//ImGui::SetScrollX();

		ImGui::EndCombo();
	}
}

bool PathdrawerTab::checkPathComplete(const plottedPath& pathRecord)
{
	if (pathRecord.startPoint == "" || pathRecord.endPoint == "")
		return false;
	for (auto& midPoint : pathRecord.midPoints)
		if (midPoint == "")
			return false;

	return true;
}

void PathdrawerTab::pathNodeRaycastHandler(const CollisionInfo& hit, const std::string nodeName)
{
	if (!isActiveTab())
		return;

	if(m_activeSubtab == Subtab::PathBuilder)
	{
		if (!m_pathDragBuildOn)
		{
			m_pathDragBuildStartPoint = nodeName;
			m_pathDragBuildOn = true;
		}
		else if (m_pathDragBuildStartPoint != nodeName)
		{
			m_pathDragBuildEndPoint = nodeName;
		}
	}
	else if (m_activeSubtab == Subtab::NodePathSearch)
	{
		//deselect already selected node
		if (m_selectedSearchNode != "No Node Selected" && m_pathNodeRender[m_selectedSearchNode])
			m_pathNodeRender[m_selectedSearchNode]->getMaterialInst()->setParam("color", vec4(1.f, 0.f, 0.f, 0.7));
		//select new node
		if (m_pathNodeRender[nodeName])
			m_pathNodeRender[nodeName]->getMaterialInst()->setParam("color", vec4(1.f, 1.f, 0.f, 0.7));
		m_selectedSearchNode = nodeName;
		//deselect previously selected paths
		m_selectedSearchResultIndex = -1;
		selectPathLine("__RESET_SELECTION__");
	}
}

void PathdrawerTab::addPathNode(std::string nodeName, vec2 nodePos, float nodeSpeed)
{
	if (nodeName.empty()) return;

	//handle duplicate node names
	if (m_pathsData.pathnodes.contains(nodeName))
	{
		std::string zeroes = "00"; //improves sorting
		for (size_t i = 0; i < 500; i++)
		{
			if (i == 10 || i == 100) 
				zeroes.pop_back();
			if (!m_pathsData.pathnodes.contains(nodeName + "_" + zeroes + STR(i)))
			{
				nodeName += "_" + zeroes + STR(i);
				break;
			}
		}
	}
	//adding node
	m_pathsData.pathnodes.insert({
			nodeName,
			pathNode{nodePos, nodeSpeed}
		});
	//rendering node
	renderPathNode(nodeName, m_pathsData.pathnodes[nodeName]);
}

void PathdrawerTab::removePathNode(const std::string& nodeName)
{
	m_pathsData.pathnodes.erase(nodeName);
	//redraw paths
	for (auto& path : m_pathsData.paths)
	{
		if (path.startPoint == nodeName)
		{
			path.startPoint = "";
			renderPathLines(path, true);
		}
		if (path.endPoint == nodeName)
		{
			path.endPoint = "";
			renderPathLines(path, true);
		}
		for (auto& midPoint : path.midPoints)
		{
			if (midPoint == nodeName)
			{
				midPoint = "";
				renderPathLines(path, true);
			}
		}
	}
	//remove Path Node's mesh
	for (auto& child : m_pathNodeRender[nodeName]->getAllChildren())
	{
		if (child)
			m_pathNodeRender[nodeName]->removeChild(child);
	}
	m_pathNodeRender[nodeName]->removeCollider("node_collider");
	m_pathNodeRender[nodeName]->getParent()->removeChild(m_pathNodeRender[nodeName]);
	m_pathNodeRender[nodeName].reset();
	m_pathNodeRender.erase(nodeName);
}

//helper macro for better code readability
#define IS_UNIQUE_PATH_NAME(path_name) (std::find_if(m_pathsData.paths.begin(), m_pathsData.paths.end(),[this, path_name](auto plottedPath) { return plottedPath.pathName == path_name; }) == m_pathsData.paths.end())

void PathdrawerTab::addPath(std::string pathName, const std::string& startNodeName, const std::string& endNodeName)
{
	if (pathName.empty() || startNodeName.empty() || endNodeName.empty())
		return;

	if (!m_pathsData.pathnodes.contains(startNodeName) || !m_pathsData.pathnodes.contains(endNodeName))
		return;

	//checking if path name is unique
	if (!IS_UNIQUE_PATH_NAME(pathName))
	{
		std::string zeroes = "00"; //improves sorting
		std::string testedName = ""; 
		for (size_t i = 0; i < 500; i++)
		{
			if (i == 10 || i == 100)
				zeroes.pop_back();
			testedName = pathName + "_" + zeroes + STR(i);
			if (IS_UNIQUE_PATH_NAME(testedName))
			{
				pathName = testedName;
				break;
			}
		}
	}

	plottedPath newPath{ .pathName = pathName, .startPoint = startNodeName, .endPoint = endNodeName };
	m_pathsData.paths.push_back(newPath);
	renderPathLines(m_pathsData.paths.back(), true);
}

void PathdrawerTab::onKeyPressed(const event::Keyboard::Down& evt)
{
	if (!isActiveTab())
		return;

	if (evt.key == w4::platform::KeyboardKey::Control && !m_touchCtrOn)
		m_touchCtrOn = true;
}

void PathdrawerTab::onKeyReleased(const event::Keyboard::Up& evt)
{
	if (evt.key == w4::platform::KeyboardKey::Control)
		m_touchCtrOn = false;
}

void PathdrawerTab::onTouchBegin(const Touch::Begin& evt)
{
	if (!isActiveTab())
		return;

	auto ray = Render::getScreenCamera()->createRayFromScreen(evt.point);
	auto hits = Render::raycastAll(ray);

	for (auto& hit : hits)
	{
		std::string hitName = hit.target->getParent()->getName();

		if (hit.target != nullptr)
		{
			auto hitName = hit.target->getParent()->getName();
			if (hitName == "clickMesh_debug")
			{
				//click adding new path node
				if (m_activeSubtab == Subtab::PathNodes && m_touchCtrOn)
					addPathNode(m_newPathNodeNaming, hit.point.withoutZ(), m_nodeDefaultSpeed);
			}
		}
	}
}

void PathdrawerTab::onTouchMove(const Touch::Move& evt)
{
	if (!m_pathDragBuildOn)
		return;

	if (m_activeSubtab == Subtab::PathBuilder && !m_pathDragBuildStartPoint.empty())
	{
		if (!m_pathsData.pathnodes.contains(m_pathDragBuildStartPoint))
			return;

		vec3 startNodePosPos; 
		startNodePosPos.x = m_pathsData.pathnodes[m_pathDragBuildStartPoint].pos.x;
		startNodePosPos.y = m_pathsData.pathnodes[m_pathDragBuildStartPoint].pos.y;
		vec3 cursorPos = vec3::zero;

		auto ray = Render::getScreenCamera()->createRayFromScreen(evt.point);
		auto hits = Render::raycastAll(ray);
		for (auto& hit : hits)
		{
			if (hit.target != nullptr)
			{
				auto hitName = hit.target->getParent()->getName();
				if (hitName == "clickMesh_debug")
				{
					cursorPos = hit.point;
				}
			}
		}

		if (!m_pathDragBuildLine)
		{
			m_pathDragBuildLine = Mesh::create::plane();
			m_pathDragBuildLine->setName("pathDragBuildLine");
			Render::getRoot()->addChild(m_pathDragBuildLine);
			auto mat = RMat("sprite")->createInstance();
			color _color = color(1.f, 0.5, 0.f, 0.7);
			mat->setTexture(TextureId::TEXTURE_0, Texture::createWithColor(_color));
			mat->enableBlending(true);
			m_pathDragBuildLine->setMaterialInst(mat);
			m_pathDragBuildLine->setRenderOrder(9150);
		}

		vec3 middle = (startNodePosPos + cursorPos) / 2;

		auto vecRot = m_pathDragBuildLine->getWorldRotation().toVector();
		vecRot.z = atan2(cursorPos.y - middle.y, cursorPos.x - middle.x);

		vec3 scale = vec3((startNodePosPos - cursorPos).length(), 0.1f, 1.0f);

		m_pathDragBuildLine->setWorldScale(scale);
		m_pathDragBuildLine->setWorldTranslation(middle);
		m_pathDragBuildLine->setWorldRotation(Rotator(vecRot));
	}
}

void PathdrawerTab::onTouchEnd(const Touch::End& evt)
{
	if (!m_pathDragBuildOn || m_activeSubtab != Subtab::PathBuilder)
		return;

	auto ray = Render::getScreenCamera()->createRayFromScreen(evt.point);
	auto hits = Render::raycastAll(ray);

	addPath(m_newPathNaming, m_pathDragBuildStartPoint, m_pathDragBuildEndPoint);

	m_pathDragBuildStartPoint = "";
	m_pathDragBuildEndPoint = "";
	m_pathDragBuildOn = false;
	Render::getRoot()->removeChild(m_pathDragBuildLine); m_pathDragBuildLine = nullptr;
}

bool PathdrawerTab::lineIntersectChecker(const vec2& l1p1, const vec2& l1p2, const vec2& l2p1, const vec2& l2p2) const
{
	float q = (l1p1.y - l2p1.y) * (l2p2.x - l2p1.x) - (l1p1.x - l2p1.x) * (l2p2.y - l2p1.y);
	float d = (l1p2.x - l1p1.x) * (l2p2.y - l2p1.y) - (l1p2.y - l1p1.y) * (l2p2.x - l2p1.x);

	if (d == 0)
	{
		return false;
	}

	float r = q / d;

	q = (l1p1.y - l2p1.y) * (l1p2.x - l1p1.x) - (l1p1.x - l2p1.x) * (l1p2.y - l1p1.y);
	float s = q / d;

	if (r < 0 || r > 1 || s < 0 || s > 1)
	{
		return false;
	}

	return true;
}

bool PathdrawerTab::isBorderIntersect(const vec2& from, const vec2& to) const
{
	vec2 borderPointFrom = vec2::zero;
	vec2 borderPointTo = vec2::zero;

	for (const auto& border : m_borders.borders) {
		bool isFirstPoint = true;
		borderPointFrom = *(border.points.end() - 1);
		for (const auto& line : border.points) {
			borderPointTo = line;
			if (lineIntersectChecker(from, to, borderPointFrom, borderPointTo)) {
				//W4_LOG_DEBUG("Intersect");
				return true;
			}
			borderPointFrom = borderPointTo;
		}
	}
	return false;
}

void PathdrawerTab::tryBuildDefaultPaths()
{
	if (m_pathsData.pathnodes.empty())
		return;
	//todo: why have to use existing file? Change it! Borders can be in different files now
	if (!w4::filesystem::isPathExists("config/staticIntersectBorderSettings.json"))
	{
		W4_LOG_WARNING("Couldn't find staticIntersectBorderSettings.json to build paths!");
		return;
	}

	auto file = w4::filesystem::open("config/staticIntersectBorderSettings.json");
	auto json = nlohmann::json::parse(file->data(), file->data() + file->size());
	nlohmann::adl_serializer<PF_BorderContainer>::from_json(json, m_borders);

	int pathsIterator = 0;
	std::set<std::pair<std::string, std::string>> generatedPathPairs;
	//Iterating over all path nodes
	for (auto& [nodeName, pathNode] : m_pathsData.pathnodes)
	{
		std::map<float, std::string> candidates;
		std::string debugOutput = "\n Path generation for " + nodeName;

		//And for each node we iterate again and look for reachable nodes
		for (auto& [otherName, otherPathNode] : m_pathsData.pathnodes)
		{
			if (nodeName == otherName)
				continue;

			if (generatedPathPairs.contains({ nodeName, otherName }))
				continue;
			if (generatedPathPairs.contains({ otherName, nodeName }))
				continue;

			if (!isBorderIntersect(otherPathNode.pos, pathNode.pos))
			{
				candidates.insert({ (otherPathNode.pos - pathNode.pos).length(), otherName });
				debugOutput = debugOutput + "\n Candidate pair: {" + nodeName + ", " + otherName + "}";
			}
		}

		debugOutput += "\n==============";
		debugOutput += "\nGenerated paths:";
		//We go through first(nearest) %m_autoPathPerNode% nodes and plot a path to them
		int pathsGenerated = 0;
		for (auto& [length, otherNodeName] : candidates)
		{
			plottedPath generatedPath = {
				.pathName = "AutoPath_" + std::to_string(pathsIterator++),
				.startPoint = nodeName,
				.endPoint = otherNodeName
			};
			generatedPathPairs.insert({ nodeName, otherNodeName});
			generatedPathPairs.insert({ otherNodeName, nodeName });
			debugOutput = debugOutput + "\n Adding Path between: {" + nodeName + ", " + otherNodeName + "}";
			debugOutput = debugOutput + "\n Adding Path between: {" + otherNodeName + ", " + nodeName + "}";

			m_pathsData.paths.push_back(generatedPath);
			renderPathLines(m_pathsData.paths.back());

			pathsGenerated++;
			if (pathsGenerated == m_autoPathPerNode)
				break;
		}

		//info("%s", debugOutput.c_str()); //debug output
	}
}

#endif
