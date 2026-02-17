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
#include "customGuiTabs/NodesTab.h"

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
    m_lastColliderProbe.reserve(25);
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

void ND_ImGui::addImGuiNode(sptr<Node> inNode, const std::string& name)
{
#ifndef NDNODES
    return;
#endif
    sptr<iGuiTab> nodesTab = getCustomTabByName("Nodes");
    if (!nodesTab) return;
    nodesTab->addItem(inNode, name);
}

void ND_ImGui::removeImGuiNode(const std::string& nodeName)
{
    sptr<iGuiTab> nodesTab = getCustomTabByName("Nodes");
    if (!nodesTab) return;
    nodesTab->removeItem(nodeName);
}

void ND_ImGui::removeAllImGuiNodes()
{
    sptr<iGuiTab> nodesTab = getCustomTabByName("Nodes");
    if (!nodesTab) return;
    nodesTab->purgeValues();

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
    
    //Nodes and hierarchy tab
#ifdef NDNODES
    m_extraTabs.push_back(make::sptr<NodesTab>());
#endif // NDNODES

    //Spawner Tab
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

    if (ImGui::TreeNode("Collider probe"))
    {
        ImGuiColliderProbe();
        ImGui::TreePop();
    }
}

void ND_ImGui::ImGuiColliderProbe()
{
    ImGui::TextColored(IMG_WHITE, "%s", "Press CTRL to raycast all colliders under Mouse");
    ImGui::Separator();
    for (const auto& data : m_lastColliderProbe)
    {
        if (ImGui::TreeNode(data.colliderDisplayName.c_str()))
        {
            ImGui::PushID(repeatElemId++);

            ImGui::Columns(2);
            ImGui::Text("%s", "Parent Node: ");
            ImGui::NextColumn();
            ImGui::Text("%s", data.hitParentName.data());
            ImGui::Columns(1);

            float pos[3]; std::memcpy(&pos, &data.colliderPos.elements, sizeof pos);
            float size[3]; std::memcpy(&pos, &data.colliderSize.elements, sizeof pos);
            ImGui::InputFloat3("Collider Pos", pos, "%.3f", ImGuiInputTextFlags_ReadOnly);
            ImGui::InputFloat3("Collider Size", size, "%.3f", ImGuiInputTextFlags_ReadOnly);

            ImGui::Columns(2);
            ImGui::Text("%s", "IS Blocking Screencasts?");
            ImGui::Text("%s", "IS Blocking Raycasts?");
            ImGui::NextColumn();
            if (data.blocksScreencasts)
                ImGui::TextColored(IMG_RED, "%s", "YES");
            else
                ImGui::TextColored(IMG_GREEN, "%s", "NO");
            if (data.blocksRaycasts)
                ImGui::TextColored(IMG_RED, "%s", "YES");
            else
                ImGui::TextColored(IMG_GREEN, "%s", "NO");
            ImGui::Columns(1);
            ImGui::TreePop();
            ImGui::PopID();
        }
    }
}

void ND_ImGui::probeColliders()
{
    m_lastColliderProbe.clear();
    math::Ray ray = Render::getScreenCamera()->createRayFromScreen(Touch::getMousePosition());
    auto hits = Render::raycastAll(ray);

    for (auto& hit : hits)
    {
        if (hit.target != nullptr)
        {
            ColliderProbeData data;
            data.parent = hit.target->getParent();
            data.hitParentName = data.parent->getName();
            data.colliderPos = data.parent->getWorldTranslation() + hit.target->getLocalTranslation();
            auto colliderBounds = hit.target->getVolume()->as<AABB>()->getBounds();
            data.colliderSize = vec3(
                neAbs(colliderBounds.max().x - colliderBounds.min().x),
                neAbs(colliderBounds.max().y - colliderBounds.min().y),
                neAbs(colliderBounds.max().z - colliderBounds.min().z));
            data.blocksScreencasts = hit.target->isBlockScreencasts();
            data.blocksRaycasts = hit.target->isBlockRaycasts();

            for (const auto&[name, collider] : data.parent->getColliders())
            {
                if (collider.get() == hit.target)
                {
                    data.colliderDisplayName = name + "[" + data.parent->getName() + "]";
                    break;
                }
            }
            m_lastColliderProbe.push_back(data);
        }
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
                m_isCameraTabActive = true;
                ND_Util::ImGuiNodeSettings(true);

                if (ImGui::BeginChild("Camera scrollable zone"))
                {
                    ImGuiCamera();
                }

                ImGui::EndChild();

                ImGui::EndTabItem();
            }
            else
                m_isCameraTabActive = false;
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