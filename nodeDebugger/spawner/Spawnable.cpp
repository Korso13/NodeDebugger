#include "Spawnable.h"
#include "settings/JsonDeserializator.h"

//Controls Spawnable's ImGui menu render (Root node transform menu, m_spawnableSettings onGui calls and etc)
bool Spawnable::onGui(const char* label)
{
	if (!m_spRoot && m_spawnableSettings.empty())
		return false;

	bool bNeedsDeletion = false; //used to communicate to SpawnerTab that the record is to be deleted via return true

#ifdef DEBUG
	if (ImGui::TreeNode(label))
	{
		ImGui::SameLine();
		if (ImGui::Button("x"))
		{
			bNeedsDeletion = true;
		}

		if (ImGui::BeginChild((m_spRoot->getName() + " Panel").c_str(), ND_Util::getSizeForChild(0.7), true, ImGuiWindowFlags_AlwaysAutoResize)) //scrollable panel with exposed settings
		{
			//Root node transforms
			if (m_spRoot)
			{
				if (ImGui::TreeNode("Root Node Transforms"))
				{
					ND_Util::ImGuiNodeRender(m_spRoot, m_spRoot->getName());

					ImGui::TreePop();
				}
				if (ImGui::TreeNode("Root Node Parent"))
				{
					auto parentNode = m_spRoot->getParent();
					if (ND_Util::ImGuiNodeHierarchy(parentNode, "Parent Node")) //returns true if new node was selected
					{
						m_spRoot->setParent(parentNode);
					}

					ImGui::TreePop();
				}
			}

			//Render for exposed class fields if any
			for (auto& [settingName, Setting] : m_spawnableSettings)
			{
				auto guiFunc = Setting.getOnGuiCb(); //function for rendeing setting in ImGui
				if (guiFunc)
				{
					//TODO: guiFunc doesn't return true for Auto used with vectors and possibly other collections :(
					if (guiFunc((settingName.empty() ? STR(repeatElemId) : settingName), Setting.settingValue)) //checking if setting was changed
					{
						Setting.onSettingChangeCB(Setting.settingValue);
					}
				}

			}

			for (auto& [btnName, btnCall] : m_spawnableBtns)
			{
				if (btnCall)
				{
					if (ImGui::Button(btnName.c_str()))
					{
						auto tempBtnCb = btnCall;
						tempBtnCb();
						break;
					}
				}
			}
		}
		ImGui::EndChild(); //ends entity scrollable zone
		ImGui::TreePop();
	}

#endif

	return bNeedsDeletion;
}

void Spawnable::initSpawnable(sptr<Node> rootNode)
{
	if (!rootNode)
		return;

	if (m_spRoot)
		m_spRoot->unsubscribeFromWorldTransformChanged(m_rootNodeTransformSubsHandler);

	m_spRoot = rootNode;
	m_rootNodeTransformSubsHandler = m_spRoot->subscribeToWorldTransformChanged([this](const Transform& trans)
		{
			getMapObjectsData().pos = trans.translation();
			getMapObjectsData().pos.z = 0.f; //sometimes the gizmo puts in double values into Z field breaking ROs. Disable if you need non-zero Z placement
		});
}
