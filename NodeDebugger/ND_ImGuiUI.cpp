#include "ND_ImGuiUI.h"

#ifndef FORCENODEDEBUG_OFF
#include "utils/EditorCamera.h"
#include <others/spineAnimation/SpineAnimationBase.h>
#include "ND_Utilities.h"
#include "TweenFactory.h"
#include <sources/common/NodeDebugger/ND_iGuiTab.h>
#include <sources/common/NodeDebugger/CustomGuiTabs/SpotlightTab.h>
#include <sources/common/NodeDebugger/CustomGuiTabs/SpinesTab.h>
#include <sources/common/NodeDebugger/CustomGuiTabs/VariablesTab.h>
#include <sources/common/NodeDebugger/CustomGuiTabs/ConsoleTab.h>
#include <sources/common/NodeDebugger/CustomGuiTabs/BordersTab.h>
#include "CustomGuiTabs/PathdrawerTab.h"

ND_ImGui::ND_ImGui()
{
    //imGUI init
    Render::enableHud(true);
    m_imguiNode = make::sptr<ImGuiNode>();
    Render::getHudPass().getRoot()->addChild(m_imguiNode);
    m_imguiNode->setRenderOrder(100000);
    m_imguiNode->setFontInline("default",
        ImGuiFontRoboto::rawData,
        static_cast<int>(sizeof(ImGuiFontRoboto::rawData)),
        m_imGuiFontSize);
    m_imguiNode->setCallback([this](float dt) { 
        if (dt == 0.f) 
            dt = 0.01; 
        onGui(dt); 
        });
    m_imguiNode->setAutomaticOnUpdate(false);

    ImGui::SetWindowSize(m_ImGWindowSize);

    m_editorCamera = make::sptr<EditorCamera>("Debug camera");
    m_editorCamera->setParent(Render::getRoot());
    m_editorCamera->m_lookButton = w4::event::Touch::Button::Middle;

    initCustomTabs();
    TweenContainer::preloadTweenContainer();
}

void ND_ImGui::addImGuiNode(sptr<Node> inNode, std::string name)
{
    if (!inNode)
        return;

    if (inNode->is<Spine>())
    {
        addSpine(inNode->as<Spine>(), name);
        return;
    }
    if (inNode->is<SpotLight>())
    {
        addSpotLight(inNode->as<SpotLight>(), name);
        return;
    }

    if (name == "default")
        name = inNode->getName();

    //prevents duplicate names
    std::string zeroes = "00"; //for better sorting
    if (m_staticNodes.contains(name))
    {
        //checks if the list contains this node already
        if (!m_staticNodes.at(name).node.expired() && m_staticNodes.at(name).node.lock().get() == inNode.get())
            return;

        for (size_t i = 0; i < 500; i++) 
        {
            if (i == 10 || i == 100)
                zeroes.pop_back();
            
            if (!m_staticNodes.contains(name + "(" + zeroes + STR(i) + ")"))
            {

                name += "(" + zeroes + STR(i) + ")";
                break;
            }
            //checks if the list contains this node already
            else if (!m_staticNodes.at(name + "(" + zeroes + STR(i) + ")").node.expired() && 
                     m_staticNodes.at(name + "(" + zeroes + STR(i) + ")").node.lock().get() == inNode.get())
                return;
        }
    }

    ImG_Node nodeStruct = { 
        .node = inNode,
        .name = name,
        .markedForDeletion = false 
    };
    m_staticNodes.insert({ name, nodeStruct });
}

void ND_ImGui::removeImGuiNode(const std::string nodeName)
{
    if (m_staticNodes.contains(nodeName))
    {
        m_staticNodes.erase(nodeName);
        return;
    }

    for (auto& [name, ImG_NodeStruct] : m_staticNodes)
    {
        if (!ImG_NodeStruct.node.expired() && ImG_NodeStruct.node.lock()->getName() == nodeName)
        {
            m_staticNodes.erase(ImG_NodeStruct.name);
            return;
        }
    }
}

void ND_ImGui::removeAllImGuiNodes()
{
    m_staticNodes.clear();
    ND_Util::debugTweensMove.clear();
    ND_Util::debugTweensScale.clear();
}

void ND_ImGui::addSpine(sptr<Spine> spineToDebug, std::string altName)
{
    if (!getCustomTabByName("Spines"))
        return;

    getCustomTabByName("Spines")->addItem(spineToDebug, altName); 
}

void ND_ImGui::addSpine(sptr<SpineAnimationBase> spineToDebug, std::string altName)
{
    if (!getCustomTabByName("Spines"))
        return;

    getCustomTabByName("Spines")->addItem(spineToDebug->getSpine(), altName);
}

void ND_ImGui::addSpotLight(sptr<SpotLight> light, const std::string& altName)
{
    if (!getCustomTabByName("SpotLights") || !light) return;

    getCustomTabByName("SpotLights")->addItem(light, altName);
}

void ND_ImGui::initCustomTabs()
{
    //Order of adding determines order of ImGui tabs!
    
    //Spines tab
#ifdef NDSPINES
    m_extraTabs.push_back(make::sptr<SpinesTab>());
#endif // NDSPINES

    //Variables tab
#ifdef NDVARIABLES
    m_extraTabs.push_back(make::sptr<VariablesTab>());
#endif // NDVARIABLES

    //Borders tab
#ifdef NDBORDERS
    m_extraTabs.push_back(make::sptr<BordersTab>()); 
#endif // NDBORDERS

#ifdef NDPATHDRAWER
    m_extraTabs.push_back(make::sptr<PathdrawerTab>());
#endif // NDPATHDRAWER

    //Spotlight tab
#ifdef NDSPOTLIGHTS
    m_extraTabs.push_back(make::sptr<SpotlightTab>());
#endif // NDSPOTLIGHTS

    //Console tab
#ifdef NDCONSOLE
    m_extraTabs.push_back(make::sptr<ConsoleTab>()); 
#endif // NDCONSOLE
}

sptr<iGuiTab> ND_ImGui::getCustomTabByName(std::string tabName)
{
    for (auto& tab : m_extraTabs)
    {
        if (tabName == tab->getTabName())
            return tab;
    }
    return nullptr;
}

void ND_ImGui::addDebugCBButton(std::string name, std::function<void()> cb)
{
    m_debugCallbacks.emplace(std::pair<std::string, std::function<void()>>{name, cb});
}

const bool ND_ImGui::checkRaycastCollision() const
{
    return ImGui::IsMouseOverAnyWindow();
}

void ND_ImGui::ImGuiCamera()
{

    if (ImGui::Checkbox("Enable Editor Camera", &editorCamOn))
    {
        m_editorCamera->toggleCameraEnabled();
        if (editorCamOn) camControlOn = true;
    }

    auto camera = (editorCamOn) ? m_editorCamera : Render::getScreenCamera();

    ND_Util::ImGuiTransforms(camera);
    ND_Util::ImGuiTweens(camera);

    //printing info to console
    ImGui::Separator();
    if (ImGui::Button("Print camera info"))
    {
        ND_Util::printDebugNodeInfo(camera);
    }

    ImGui::Separator();
    if (ImGui::ColorEdit("Clear Color edit", m_clearColor))
    {
        Render::getScreenCamera()->setClearColor(m_clearColor);
    }
}

void ND_ImGui::onGui(float dt)
{
    if (!isDebuggerOn)
        return;

    if (!m_imguiNode)
        return;

    ND_Util::resetImGuiRepeatElemId();
    if (ImGui::Begin("Node Debugger"))
    {
        if (m_foldImGuiMenu)
            m_foldImGuiMenu = false;
        m_windowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_::ImGuiFocusedFlags_AnyWindow);

        //start size-related code
        if (m_firstUpdates > 0)
        {
            ImGui::SetWindowSize(IMG_STARTSIZE);
            m_ImGWindowSize = ImGui::GetWindowSize();
            m_firstUpdates--;
        }
        else
        {
            m_ImGWindowSize = ImGui::GetWindowSize();
            ImGui::SetWindowSize(m_ImGWindowSize);
        }

        ND_Util::ImGuiSettings();

        //content tabs
        if (ImGui::BeginTabBar("Tabs"))
        {
            //debug camera tab
            if (ImGui::BeginTabItem("Camera"))
            {
                ND_Util::ImGuiNodeSettings();
                ImGuiCamera();
                ImGui::EndTabItem();
            }
            //debug nodes and meshes tab
            if (!m_staticNodes.empty())
                if (ImGui::BeginTabItem("Nodes,Meshes"))
                {
                    ImGuiNodes();

                    ImGui::EndTabItem();
                }
            //debug calls tab
            if (!m_debugCallbacks.empty())
                if (ImGui::BeginTabItem("Debug calls"))
                {
                    for (auto& [name, cb] : m_debugCallbacks)
                    {
                        if (ImGui::Button(name.c_str()))
                        {
                            if (cb) cb();
                        }
                    }

                    ImGui::EndTabItem();
                }
            //Additional custom tabs
            if (!m_extraTabs.empty())
            {
                for (auto& tab : m_extraTabs)
                {
                    if (!tab)
                        continue;
                    if (tab->isEmpty())
                        continue;

                    if (ImGui::BeginTabItem(tab->getTabName()))
                    {
                        tab->onGuiCall();
                        ImGui::EndTabItem();
                    }
                }
            }

            ImGui::EndTabBar();
        }
    }
    else if (!m_foldImGuiMenu)
        m_foldImGuiMenu = true;

    ImGui::End();
}

void ND_ImGui::ImGuiNodes()
{
    ND_Util::ImGuiNodeSettings();
    bool isFilterChanged = false;
    if (m_staticNodes.size() > 6)
        isFilterChanged = ND_Util::ImGuiSearchBar(m_nodesFilter);
    else if (!m_nodesFilter.empty())
        m_nodesFilter.clear();

    ImGui::SameLine();
    if (ImGui::Button("Add all existing nodes"))
    {
        Render::getRoot()->traversalChildrenTyped<Node>([this](sptr<Node> _node)
            {
                if (_node && _node != Render::getRoot() && _node != Render::getScreenCamera())
                    addImGuiNode(_node);
            });
        for (auto& [name, ImG_NodeStruct] : m_staticNodes)
        {
            if (ImG_NodeStruct.node.expired())
                ImG_NodeStruct.markedForDeletion = true;
        }
    }
    ImGui::SameLine();
    ImGui::Text("Nodes shown: %d", m_nodeFilterItems);

    if (m_nodesFilter.empty()) //no filter applied
    {
        m_nodeFilterItems = m_staticNodes.size();
        for (auto& [name, ImG_NodeStruct] : m_staticNodes)
        {
            if (!ImG_NodeStruct.node.expired() && !ImG_NodeStruct.markedForDeletion)
            {
                //Node item render code
                ImGNodeRender(ImG_NodeStruct.node.lock(), name);

                m_nodeFilterItems++;
            }
            else if (ImG_NodeStruct.node.expired())
                ImG_NodeStruct.markedForDeletion = true;
        }
    }
    else
    {
        if (isFilterChanged) //filter changed. resetting list of matching node names
        {
            m_nodeFilterItems = 0;
            m_lastNodeFilterList.clear();

            for (auto& [name, ImG_NodeStruct] : m_staticNodes)
            {
                if (!ImG_NodeStruct.node.expired() && !ImG_NodeStruct.markedForDeletion)
                {
                    if (ND_Util::checkFilterConditions(name, m_nodesFilter))
                    {
                        //Node item render code
                        ImGNodeRender(ImG_NodeStruct.node.lock(), name);

                        m_lastNodeFilterList.insert(name);
                        m_nodeFilterItems++;
                    }
                }
                else if (ImG_NodeStruct.node.expired())
                    ImG_NodeStruct.markedForDeletion = true;
            }
        }
        else //filter has't changed sonce last render. Less compute overhead
        {
            //m_nodeFilterItems = m_lastNodeFilterList.size();
            for (auto& [name, ImG_NodeStruct] : m_staticNodes)
            {
                if (
                    !ImG_NodeStruct.node.expired()
                    && !ImG_NodeStruct.markedForDeletion
                    && m_lastNodeFilterList.contains(name)
                    )
                {
                    //Node item render code
                    ImGNodeRender(ImG_NodeStruct.node.lock(), name);
                }
                else if (ImG_NodeStruct.node.expired())
                    ImG_NodeStruct.markedForDeletion = true;
            }
        }
    }

    std::erase_if(m_staticNodes,
        [](const auto& record)
        {
            return record.second.markedForDeletion;
        });
}

void ND_ImGui::ImGNodeRender(sptr<Node> _node, const std::string& name)
{
    if (!m_imguiNode || !_node)
        return;

    if (ImGui::TreeNode(name.c_str()))
    {
        ImGui::SameLine();
        if (ImGui::Button("x"))
        {
            m_staticNodes[name].markedForDeletion = true;
        }

        //Drag gizmo
        ImGuizmo::BeginFrame();
        ImGuiDrawerGizmo::drawGizmosHandles(_node);

        //visibility button
        if (ImGui::Button("Toggle visibility"))
        {
            _node->setEnabled(!_node->isEnabled());
        }

        if (ImGui::Button("Center on camera"))
        {
            vec3 target = Render::getScreenCamera()->getWorldTranslation();
            target.z = 0.f;
            _node->setWorldTranslation(target);
        }

        if (_node->is<Mesh>())
        {
            ImGui::Separator();
            int meshRO = _node->as<Mesh>()->getRenderOrder();
            bool roChanged = false;
            roChanged = ImGui::DragInt("Render order", &meshRO, sliderSensivity, 0, 10000);
            if (roChanged)
                _node->as<Mesh>()->setRenderOrder(meshRO);
        }

        ND_Util::ImGuiTransforms(_node);
        //printing info to console
        ImGui::Separator();
        if (ImGui::Button("Print node info"))
        {
            ND_Util::printDebugNodeInfo(_node);
        }

        ND_Util::ImGuiTweens(_node);

        if (_node->is<Mesh>())
        {
            ImGui::Separator();
            const auto mesh = cast_sptr<Mesh>(_node);
            ImGuiDrawerMaterial::onGui("Mesh material settings", *mesh->getMaterialInst());
        }

        ImGui::TreePop();
        ImGui::Separator();
        ImGui::Spacing();
    }

}

void ND_ImGui::ImGuiUpdate(float dt)
{
    if (!m_imguiNode)
        return;

    if (dt == 0.f)
        dt = 0.01;
    m_imguiNode->onUpdate(dt);
}

#endif //ends #ifndef FORCENODEDEBUG_OFF - conditional compilation macro. If FORCENODEDEBUG_OFF is defined, NOCOMP macro provides empty implementation and saves some weight in build size
