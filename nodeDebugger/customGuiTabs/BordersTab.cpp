#include "BordersTab.h"
#include "../ND_JsonTools.h"

std::function<void(nlohmann::json)> BordersTab::m_onJsonLoaded = nullptr;

#ifndef FORCENODEDEBUG_OFF

//overload needed for std::set to function
bool operator<(const sptr<TabBorderLineData>& b1, const sptr<TabBorderLineData>& b2)
{
	return b1->borderName < b2->borderName;
}

void BordersTab::onGuiCall()
{
	if (ImGui::BeginChild("Borders scrollable zone", ND_Util::getSizeForChild()))
	{
		for (const auto& borderRecord : m_debugBorders)
		{
			bool bNeedRemovePoint = false;
			int removePointIndex = -1;

			//Border record render begin
			if (ImGui::TreeNode(borderRecord->borderName.c_str())) 
			{
				m_lastEditedBorder = borderRecord;

				//rendering menus for existing border points
				if (!borderRecord->points.empty())
				{
					for (size_t i = 0; i < borderRecord->points.size(); i++) //border points list render
					{
						//renders border point menu and implements its functionality:
						ImGuiBorderPoint(borderRecord, i, bNeedRemovePoint, removePointIndex); 
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
					addBorderPoint(borderRecord);
				}

				ImGui::TreePop();
			}//Border record render end

			//border points removing mechanism
			if (bNeedRemovePoint)
			{
				removeBorderPoint(borderRecord, removePointIndex);
			}
		}
	}

	ImGui::EndChild(); //ends borders list scrollable zone

	//Adding new set of borders
	ImGui::Columns(2);
	ImGui::InputText("Name", &m_newBorderNaming);
	ImGui::NextColumn();
	if (ImGui::Button("Create new border"))
	{
		sptr<TabBorderLineData> newBorder = make::sptr<TabBorderLineData>();
		newBorder->borderName = m_newBorderNaming;
		m_newBorderNaming = "New_border" + STR(m_debugBorders.size());
		m_debugBorders.insert(newBorder);
	}
	ImGui::Columns(1);

	if (ImGui::Button("Export borders as JSON"))
	{
		TabBorderContainer borderContainer;
		for (auto& borderSetting : m_debugBorders)
		{
			borderContainer.borders.push_back(*borderSetting.get());
		}

		nlohmann::json exportJson;
		nlohmann::adl_serializer<TabBorderContainer>::to_json(exportJson, borderContainer);
		exportAsJson(exportJson);
	}

	if (ImGui::Button("Import borders from JSON"))
	{
		loadJson();
	}
}

void BordersTab::addItem(std::any item, const std::string& altName)
{
}

void BordersTab::renderBorderLines(std::string borderRecordToRedraw)
{
	for(auto& borderRegion : m_debugBorders)
	{
		if (borderRegion->borderName != borderRecordToRedraw)
			continue;

		if (borderRegion->points.size() < 2)
			return;

		//clearing old render or borders
		for (auto& mesh : m_borderLinesRender[borderRegion->borderName])
		{
			if(mesh)
				mesh->getParent()->removeChild(mesh);
		}
		m_borderLinesRender.erase(borderRegion->borderName);
		m_borderLinesRender.insert({ borderRegion->borderName, std::vector<sptr<Mesh>>() });

		//rendering new borderlines
		int i = 0;
		std::string debugOutput = "\nBorders: ";

		for (const auto& border : borderRegion->points) 
		{
			std::string borderPointName = borderRegion->borderName + "_" + STR(i);

			vec3 pos = vec3(border.x, border.y, 0.0);
			//m_borderRender[borderPointName]->removeAllChildren();

			debugOutput += "\n[";
			debugOutput += STR(i);
			debugOutput += "] ";
			debugOutput += STR(border);

			vec3 prevPos = pos;
			vec2 prevPointCoords = (
				i == 0
				? borderRegion->points[borderRegion->points.size() - 1]
				: borderRegion->points[i - 1]);

			prevPos.x = prevPointCoords.x;
			prevPos.y = prevPointCoords.y;

			//debug("\nDrawing line [%d] from \n%s \nto \n%s", i, CSTR(prevPos), CSTR(pos));

			//auto l = w4::render::Plotter::buildLine(prevPos, pos, color(color::Red));
			//m_borderRender[borderPointName]->addChild(l);

			auto line = Mesh::create::plane();
			line->setName("line_" + STR(i));
			m_borderRender[borderPointName]->addChild(line);
			auto mat = RMat("sprite")->createInstance();
			color _color = color(1.f, 1.f, 0.f, 1.f);
			mat->setTexture(TextureId::TEXTURE_0, Texture::createWithColor(_color));
			line->setMaterialInst(mat);
			m_borderLinesRender[borderRegion->borderName].push_back(line);

			auto middle = (pos + prevPos) / 2;
			middle.z = -0.001;

			auto vecRot = line->getWorldRotation().toVector();
			vecRot.z = atan2(prevPos.y - middle.y, prevPos.x - middle.x);

			auto scale = vec3((pos - prevPos).length(), 0.1f, 1.0f);

			line->setWorldScale(scale);
			line->setRenderOrder(10000 - 1);
			line->setWorldTranslation(middle);
			line->setWorldRotation(Rotator(vecRot));

			++i;
		}
		
		//info("%s", debugOutput.c_str());
	}

}

void BordersTab::exportAsJson(nlohmann::json& inJson)
{
	std::string flatJson = nlohmann::jsonUnflatten(inJson);
	ImGui::SetClipboardText(flatJson.c_str());
	jsWriteEvent(flatJson.c_str(), "bordersContainerSettings.json");
}

void BordersTab::loadJson()
{
	jsReadJson(this, &BordersTab::onJSONLoadCB);

	if (!m_onJsonLoaded)
	{
		//executed on file load:
		m_onJsonLoaded = [this](nlohmann::json loadedJson)
		{
			//W4_LOG_INFO("[BorderTab]\nParsed json: %s", loadedJson.dump().c_str());
			TabBorderContainer borderContainer;
			nlohmann::adl_serializer<TabBorderContainer>::from_json(loadedJson, borderContainer);
			
			for (auto& borderRecord : borderContainer.borders)
			{
				//adding border records to the borders tab
				m_debugBorders.insert(make::sptr<TabBorderLineData>(borderRecord));
				//info("[BorderTab]\nAdded new records to m_debugBorders");

				//generating border points
				int i = 0;
				for (auto& borderPoint : borderRecord.points)
				{
					auto newPointName = borderRecord.borderName + "_" + STR(i);
					auto newPointMesh = Mesh::create::plane(vec2(0.1, 0.1));
					newPointMesh->setMaterialInst(Material::getDefault()->createInstance());
					Render::getRoot()->addChild(newPointMesh);
					newPointMesh->setRenderOrder(10000);

					m_borderRender.insert(
						{
							newPointName,
							newPointMesh
						});
					m_borderRender[newPointName]->setWorldTranslation(vec3(borderPoint.x, borderPoint.y, -0.001f));
					//debug("[BorderTab]\nAdded %s border point", newPointName.c_str());
					i++;
				}

				//generating border lines
				renderBorderLines(borderRecord.borderName);
			}
		};
	}
}

void BordersTab::ImGuiBorderPoint(sptr<TabBorderLineData> borderRecord, size_t pointIndex, bool& bNeedRemovePoint, int& removePointIndex)
{
	std::string meshSearchName = borderRecord->borderName + "_" + STR(pointIndex);

	if (ImGui::TreeNode(meshSearchName.c_str()))
	{
		//point deletion button
		ImGui::SameLine();
		if (ImGui::Button("x"))
		{
			bNeedRemovePoint = true;
			removePointIndex = pointIndex;
			//removePointBorderName = borderRecord->borderName;
		}

		//Drag gizmo
		if (m_borderRender.find(meshSearchName) != m_borderRender.end()
			&& m_borderRender[meshSearchName])
		{
			//Gizmo
			ND_Util::NoConflictGizmo(m_borderRender[meshSearchName], meshSearchName);

			if (m_borderRender[meshSearchName]->getWorldTranslation().withoutZ() != borderRecord->points[pointIndex])
			{
				borderRecord->points[pointIndex] = m_borderRender[meshSearchName]->getWorldTranslation().withoutZ();
				renderBorderLines(borderRecord->borderName);
			}
		}
		//else
		//	debug("[BorderTab]\nNo %s border point", meshSearchName.c_str());

		//border point position
		if (ImGui::DragVec2(("Pos" + STR(pointIndex)).c_str(), borderRecord->points[pointIndex]))
		{
			if (m_borderRender.find(meshSearchName) != m_borderRender.end()
				&& m_borderRender[meshSearchName])
			{
				//updating mesh position
				m_borderRender[meshSearchName]->setWorldTranslation(vec3(borderRecord->points[pointIndex].x, borderRecord->points[pointIndex].y, -0.001f));
				renderBorderLines(borderRecord->borderName);
			}
		};

		ImGui::TreePop();
	}
}

void BordersTab::addBorderPoint(sptr<TabBorderLineData> borderRecord, vec2 newPointPos)
{
	if (!borderRecord)
		return;

	//where new poitn will be placed
	if (newPointPos == vec2(666))
	{
		if (!borderRecord->points.empty())
			newPointPos = borderRecord->points.back();
		else
			newPointPos = Render::getScreenCamera()->getWorldTranslation().withoutZ();
	}
	borderRecord->points.push_back(newPointPos);

	m_needScrollDownNextFrame = 3;
	auto newPointName = borderRecord->borderName + "_" + STR(borderRecord->points.size() - 1);
	auto newPointMesh = Mesh::create::plane(vec2(0.1, 0.1));
	newPointMesh->setMaterialInst(Material::getDefault()->createInstance());
	Render::getRoot()->addChild(newPointMesh);
	newPointMesh->setRenderOrder(10000);

	m_borderRender.insert(
		{
			newPointName,
			newPointMesh
		});
	m_borderRender[newPointName]->setWorldTranslation(vec3(borderRecord->points.back().x, borderRecord->points.back().y, -0.001f));
	//debug("[BorderTab]\nAdded %s border point", newPointName.c_str());

	renderBorderLines(borderRecord->borderName);
}

void BordersTab::removeBorderPoint(sptr<TabBorderLineData> borderRecord, size_t removePointIndex)
{
	std::string meshSearchName = borderRecord->borderName + "_" + STR(removePointIndex);

	if (m_borderRender.find(meshSearchName) != m_borderRender.end()
		&& m_borderRender[meshSearchName])
	{
		Render::getRoot()->removeChild(m_borderRender[meshSearchName]);
		m_borderRender.erase(meshSearchName);
	}

	//removing point from border record
	borderRecord->points.erase(borderRecord->points.begin() + removePointIndex);
	//shifting remaining points' names to reflect deletion
	for (size_t i = removePointIndex; i < borderRecord->points.size(); i++)
	{
		std::string newSearchName = borderRecord->borderName + "_" + STR(i);
		std::string oldSearchName = borderRecord->borderName + "_" + STR(i + 1);
		m_borderRender[newSearchName] = m_borderRender[oldSearchName];

		if (i + 1 == borderRecord->points.size())
			m_borderRender.erase(oldSearchName);
	}

	renderBorderLines(borderRecord->borderName);
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

	if (!m_clickAddModeON)
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
				addBorderPoint(m_lastEditedBorder, hit.point.withoutZ());
			}
		}
	}
}

#endif
