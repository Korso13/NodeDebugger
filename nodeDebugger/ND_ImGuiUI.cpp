#include "ND_ImGuiUI.h"

#ifndef FORCENODEDEBUG_OFF
#include "utils/EditorCamera.h"
#include <others/spineAnimation/SpineAnimationBase.h>
#include "ND_Utilities.h"
#include "TweenFactory.h"
#include <sources/common/nodeDebugger/ND_iGuiTab.h>
#include <sources/common/nodeDebugger/customGuiTabs/SpotlightTab.h>
#include <sources/common/nodeDebugger/customGuiTabs/SpinesTab.h>
#include <sources/common/nodeDebugger/customGuiTabs/VariablesTab.h>
#include <sources/common/nodeDebugger/customGuiTabs/ConsoleTab.h>
#include <sources/common/nodeDebugger/customGuiTabs/BordersTab.h>
#include "customGuiTabs/PathdrawerTab.h"
#include "customGuiTabs/SpawnerTab.h"
#include "NodeDebugger.h"

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

    m_editorCamera = make::sptr<EditorCamera>("Debug camera");
    m_editorCamera->setParent(Render::getRoot());
    m_editorCamera->m_lookButton = w4::event::Touch::Button::Middle;

    ImGui::SetWindowSize(IMG_STARTSIZE);

    // Scaling
    const float scale = SFF("ND_scale", 1.f);
    if(scale != 1.f)
    {
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(scale);
        ImGui::GetIO().FontGlobalScale = scale;
    }

    event::Resize::subscribe(std::bind(&ND_ImGui::onResize, this, std::placeholders::_1));
    onResize({ Render::getSize() });
    setUiTheme(SSF("NodeDebuggerTheme", "Default"));

    initCustomTabs();
    TweenContainer::preloadTweenContainer();
}

void ND_ImGui::onResize(const event::Resize& evt)
{
    if (m_editorCamera)
    {
        const auto screenCamera = Render::getScreenCamera();
        m_editorCamera->setPerspective(
            screenCamera->getFov(), screenCamera->getAspect(), screenCamera->getNear(), screenCamera->getFar());
    }

    auto size = evt.size;
    ImGuiIO& io = m_imguiNode->getContext().IO;
    auto scale = (static_cast<float>(size.w) / 1920.0f + static_cast<float>(size.h) / 1080.f) / 2;
    //scale *= 0.8f;
    scale *= SFF("ND_scale", 1.f);
    m_imguiNode->setScale(scale);
    io.DisplaySize = w4::math::vec2{ static_cast<float>(size.w) / scale, static_cast<float>(size.h) / scale };
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
    m_needUpdateNodesFilter = true;

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

#ifdef NDSPINES
    if (getCustomTabByName("Spines"))
    {
        getCustomTabByName("Spines")->removeItem(nodeName);
    }
#endif

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
    
    m_extraTabs.push_back(make::sptr<SpawnerTab>());
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
    //printing info to console
    ImGui::Separator();
    if (ImGui::Button("Print camera info"))
    {
        ND_Util::printDebugNodeInfo(camera);
    }

    if (ImGui::TreeNode("Tween Builder"))
    {
        ND_Util::ImGuiTweens(camera);

        ImGui::TreePop();
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
    ND_Util::resetPlotterWindowFlag();

    //start size-related code
    ImGui::SetNextWindowSize(IMG_STARTSIZE, ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Node Debugger"))
    {
        //Drag gizmo init
        ImGuizmo::BeginFrame();

        if (m_foldImGuiMenu)
            m_foldImGuiMenu = false;
        m_windowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_::ImGuiFocusedFlags_AnyWindow);

        ND_Util::ImGuiSettings();

        //content tabs
        if (ImGui::BeginTabBar("Tabs"))
        {
            //debug camera tab
            if (ImGui::BeginTabItem("Camera"))
            {
                ND_Util::ImGuiNodeSettings(true);

                if (ImGui::BeginChild("Camera scrollable zone"))
                {
                    ImGuiCamera();
                }
                ImGui::EndChild();

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
                    if (ImGui::BeginChild("Debug calls scrollable zone"))
                    {
                        for (auto& [name, cb] : m_debugCallbacks)
                        {
                            if (ImGui::Button(name.c_str()))
                            {
                                if (cb) cb();
                            }
                        }
                    }
                    ImGui::EndChild();

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
                    {
                        tab->markActive(false);
                        continue;
                    }

                    if (ImGui::BeginTabItem(tab->getTabName()))
                    {
                        tab->markActive(true);
                        tab->onGuiCall();
                        ImGui::EndTabItem();
                    }
                    else
                        tab->markActive(false);
                }
            }

            ImGui::EndTabBar();
        }
    }
    else if (!m_foldImGuiMenu)
    {
        m_foldImGuiMenu = true;
        m_windowFocused = false;
    }

    ImGui::End();

    ND_Util::plotterDockWindowWrapper();
}

void ND_ImGui::ImGuiNodes()
{
    ND_Util::ImGuiNodeSettings();
    bool isFilterChanged = false;
    if (m_staticNodes.size() > 6)
        isFilterChanged = (ND_Util::ImGuiSearchBar(m_nodesFilter) || m_needUpdateNodesFilter);
    else if (!m_nodesFilter.empty())
        m_nodesFilter.clear();

    if (m_needUpdateNodesFilter) m_needUpdateNodesFilter = false;

    ImGui::SameLine();
    if (ImGui::Button("Add all existing nodes"))
    {
        Render::getRoot()->traversalTyped<Node>([this](sptr<Node> _node)
            {
                if (_node && _node != Render::getRoot() && _node != Render::getScreenCamera())
                    addImGuiNode(_node);
            });
        Render::getHudPass().getRoot()->traversalTyped<Node>([this](sptr<Node> _node)
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

    if (ImGui::BeginChild("Filtered Nodes"))
    {
        if (m_nodesFilter.empty()) //no filter applied
        {
            m_nodeFilterItems = m_staticNodes.size();
            for (auto& [name, ImG_NodeStruct] : m_staticNodes)
            {
                if (!ImG_NodeStruct.node.expired() && !ImG_NodeStruct.markedForDeletion)
                {
                    //Node item render code
                    if (ImGui::TreeNode(name.c_str()))
                    {
                        ImGui::SameLine();
                        if (ImGui::Button("x"))
                        {
                            m_staticNodes[name].markedForDeletion = true;
                        }

                        ND_Util::ImGuiNodeRender(ImG_NodeStruct.node.lock(), name);

                        ImGui::TreePop();
                    }

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
                            if (ImGui::TreeNode(name.c_str()))
                            {
                                ImGui::SameLine();
                                if (ImGui::Button("x"))
                                {
                                    m_staticNodes[name].markedForDeletion = true;
                                }

                                ND_Util::ImGuiNodeRender(ImG_NodeStruct.node.lock(), name);

                                ImGui::TreePop();
                            }

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
                        if (ImGui::TreeNode(name.c_str()))
                        {
                            ImGui::SameLine();
                            if (ImGui::Button("x"))
                            {
                                m_staticNodes[name].markedForDeletion = true;
                            }

                            ND_Util::ImGuiNodeRender(ImG_NodeStruct.node.lock(), name);
                            
                            ImGui::TreePop();
                        }
                        ImGui::Separator();
                        ImGui::Spacing();
                        //ImGui::Separator();
                    }
                    else if (ImG_NodeStruct.node.expired())
                        ImG_NodeStruct.markedForDeletion = true;
                }
            }
        }
    }
    ImGui::EndChild(); //ends nodes list scrollable zone

    std::erase_if(m_staticNodes,
        [](const auto& record)
        {
            return record.second.markedForDeletion;
        });
}

void ND_ImGui::ImGuiUpdate(float dt)
{
    if (!m_imguiNode)
        return;

    if (dt == 0.f)
        dt = 0.01;
    m_imguiNode->onUpdate(dt);
}

void ND_ImGui::setUiTheme(const std::string themeName)
{
    if (themeName == "Default")
    {
        ImGuiStyleHelpers::setDefaultW4Style();
    }
    else if (themeName == "Classic")
    {
        ImGui::StyleColorsClassic();
    }
    else if (themeName == "Light")
    {
        ImGui::StyleColorsLight();
    }
    else if (themeName == "Dark")
    {
        ImGui::StyleColorsDark();
    }
    else if (themeName == "Cherry")
    {
        ImGuiStyleHelpers::setColorsCherry();
    }
    else if (themeName == "Green")
    {
        ImGuiStyleHelpers::setColorsLightGreen();
    }
    else if (themeName == "Photoshop")
    {
        ImGuiStyleHelpers::setColorsPhotoshop();
    }
    else if (themeName == "Grey")
    {
        ImGuiStyleHelpers::setColorsGrey();
    }
    else if (themeName == "DarkOrange")
    {
        ImGuiStyleHelpers::setColorsDarkOrange();
    }
    else if (themeName == "DarkExtra")
    {
        ImGuiStyleHelpers::setColorsDark();
    }
    else if (themeName == "Darcula")
    {
        ImGuiStyleHelpers::setColorsDarcula();
    }
}

#endif //ends #ifndef FORCENODEDEBUG_OFF - conditional compilation macro. If FORCENODEDEBUG_OFF is defined, NOCOMP macro provides empty implementation and saves some weight in build size