#include "NodeDebugger.h"
#include "common/Defines.h"

NodeDebugger* NodeDebugger::m_instance = nullptr;

#ifndef FORCENODEDEBUG_OFF
#include "ND_Utilities.h"
#include "utils/EditorCamera.h"
#include <sources/common/nodeDebugger/ND_iGuiTab.h>

NodeDebugger::NodeDebugger() :
    ND_ImGui(),
    OnUpdateProvider()
{
    setAutomaticOnUpdate(true);
    event::Keyboard::Down::subscribe(std::bind(&NodeDebugger::onKeyPressed, this, std::placeholders::_1));

    if (!isDebuggerOn) return;

    m_clickMesh = Mesh::create::plane(vec2(1000.f, 1000.f));
    m_clickMesh->setName("clickMesh_debug");
    auto collider = m_clickMesh->addCollider<AABB>("clickMesh_debug", m_clickMesh);
    collider->setReceiveRaycasts(true);
    collider->setBlockRaycasts(false);
    collider->setBlockScreencasts(false);
    collider->setScreencastCallback([this](const CollisionInfo& hit)
        {
            if (!ND_CASTALLOWED)
                return;

            if (m_nodeSwitchBoard[m_selectedNode])
            {
                vec3 local = hit.point - m_nodeSwitchBoard[m_selectedNode]->getWorldTranslation();
                W4_LOG_INFO("Click worldPos \n%s\nlocalPos \n%s", jsprint(hit.point), jsprint(local));
            }
            else
            {
                W4_LOG_INFO("Click worldPos \n%s", jsprint(hit.point));
            }

            if (m_clickPlacementMode && m_nodeSwitchBoard[m_selectedNode])
            {
                m_nodeSwitchBoard[m_selectedNode]->setWorldTranslation(hit.point);
                ND_Util::printDebugNodeInfo(m_nodeSwitchBoard[m_selectedNode]);
            }
        });
    Render::getHudPass().getRoot()->addChild(m_clickMesh, true, Node::VisibilityPolice::Hidden);
}

void NodeDebugger::setFloatAccuracy(int accuracy)
{
    if (accuracy < 0)
        return;

    if (accuracy > 6)
    {
        floatAccuracy = 6; //static variable in ND_Utilities
        return;
    }

    floatAccuracy = accuracy;
}

void NodeDebugger::setCallbackForButton(platform::KeyboardKey key, std::function<void()> cb)
{
    m_buttonCb.emplace(key, cb);
}

void NodeDebugger::setClickMeshZ(float targetZ)
{
    if (m_clickMesh)
        m_clickMesh->setWorldTranslation(vec3(0.f, 0.f, targetZ));
}

void NodeDebugger::addNode(sptr<Node> inNode, const unsigned int assignButton)
{
	if (assignButton > 9)
		return;

	m_nodeSwitchBoard[assignButton] = inNode;
}

void NodeDebugger::purgeDebugger()
{
    for (size_t i = 0; i < 10; i++)
    {
        m_nodeSwitchBoard[i].reset();
    }
    for (auto& tab : m_extraTabs)
    {
        if (tab) tab->purgeValues();
    }
    m_buttonCb.clear();
    m_staticNodes.clear();
    ND_Util::debugTweensMove.clear();
    ND_Util::debugTweensScale.clear();
    m_debugCallbacks.clear();
}

void NodeDebugger::onKeyPressed(const event::Keyboard::Down& evt)
{ 
    if (!isDebuggerOn || m_windowFocused)
        return;

    auto light = Render::getPass(0)->getDirectionalLight();
    vec3 lDir = light->getDirection();

    switch (evt.key) 
    {
        //debug node selectors
    case platform::KeyboardKey::Digit1:
        m_selectedNode = 1;
        ImGuiDrawerGizmo::m_currentGizmoOperation = ImGuizmo::TRANSLATE;
        break;
    case platform::KeyboardKey::Digit2:
        m_selectedNode = 2;
        ImGuiDrawerGizmo::m_currentGizmoOperation = ImGuizmo::ROTATE;
        break;
    case platform::KeyboardKey::Digit3:
        m_selectedNode = 3;
        ImGuiDrawerGizmo::m_currentGizmoOperation = ImGuizmo::SCALE;
        break;
    case platform::KeyboardKey::Digit4:
        m_selectedNode = 4;
        break;
    case platform::KeyboardKey::Digit5:
        m_selectedNode = 5;
        break;
    case platform::KeyboardKey::Digit6:
        m_selectedNode = 6;
        break;
    case platform::KeyboardKey::Digit7:
        m_selectedNode = 7;
        break;
    case platform::KeyboardKey::Digit8:
        m_selectedNode = 8;
        break;
    case platform::KeyboardKey::Digit9:
        m_selectedNode = 9;
        break;
    case platform::KeyboardKey::Digit0:
        m_selectedNode = 0;
        break;


        //Mode selector
    case platform::KeyboardKey::C:
        switchToCamControl();
        W4_LOG_INFO("Camera debug mode %s!", camControlOn ? "on" : "off");
        return;
    case platform::KeyboardKey::L:
        switchToLightControl();
        W4_LOG_INFO("Light debug mode!");
        return;
    case platform::KeyboardKey::R:
        setRotationOutputType(!degreesOutput);
        if(degreesOutput) W4_LOG_INFO("Degrees output mode!");
        else W4_LOG_INFO("Radians output mode!");
        return;
        break;
    case platform::KeyboardKey::P:
        m_clickPlacementMode = !m_clickPlacementMode;
        if(m_clickPlacementMode) W4_LOG_INFO("Click placement mode ON!");
        else W4_LOG_INFO("Click placement mode OFF!");
        return;
        break;
    case platform::KeyboardKey::H:
        ND_Util::printKeymapping();
        return;
        break;
    case platform::KeyboardKey::N:
        printConnectedNodes();
        return;
        break;
    case platform::KeyboardKey::F2:
        m_foldImGuiMenu = !m_foldImGuiMenu;
        ImGui::SetWindowCollapsed("Node Debugger", m_foldImGuiMenu);
        return;
        break;
    case platform::KeyboardKey::Alt:
        if (camControlOn)
        {
            m_editorCamera->toggleCameraEnabled();
            editorCamOn = !editorCamOn;
        }
        return;
        break;

        //Step adjuster
    case platform::KeyboardKey::Minus:
        m_translateStep -= 0.01f;
        if (m_translateStep <= 0.f)
            m_translateStep = 0.01f;
        W4_LOG_DEBUG("Translation step set to: %f", m_translateStep);
        return;
        break;
    case platform::KeyboardKey::Equal:
        m_translateStep += 0.01f;
        W4_LOG_DEBUG("Translation step set to: %f", m_translateStep);
        return;
        break;
    case platform::KeyboardKey::Numpad7:
        m_rotStep -= 0.001f;
        if (m_rotStep <= 0.f)
            m_rotStep = 0.001f;
        W4_LOG_DEBUG("Rotation step set to: %f", m_rotStep);
        return;
        break;
    case platform::KeyboardKey::Numpad9:
        m_rotStep += 0.001f;
        W4_LOG_DEBUG("Rotation step set to: %f", m_rotStep);
        return;
        break;
    case platform::KeyboardKey::NumpadDivide:
        m_scaleStep -= 0.01f;
        if (m_scaleStep <= 0.f)
            m_scaleStep = 0.01f;
        W4_LOG_DEBUG("Scaling percent set to: %f", m_scaleStep * 100.f);
        return;
        break;
    case platform::KeyboardKey::NumpadMultiply:
        m_scaleStep += 0.01f;
        W4_LOG_DEBUG("Scaling percent set to: %f", m_scaleStep * 100.f);
        return;
        break;


        //directional light | camera controlls
    case platform::KeyboardKey::W:
        if (camControlOn && !editorCamOn)
        {
            Render::getScreenCamera()->translateWorld(vec3{ 0.0,m_translateStep, 0.0 });
            W4_LOG_INFO("camera worldPos \n%s", jsprint(Render::getScreenCamera()->getWorldTranslation()));
        }
        else
        {
            lDir += vec3{ 0.f, 0.1f, 0.f };
            light->setDirection(lDir);
            W4_LOG_INFO("Dir light direction: \n%s!", jsprint(lDir));
        }
        return;
        break;
    case platform::KeyboardKey::S:
        if (camControlOn && !editorCamOn)
        {
            Render::getScreenCamera()->translateWorld(vec3{ 0.0, -m_translateStep, 0.0 });
            W4_LOG_INFO("camera worldPos \n%s", jsprint(Render::getScreenCamera()->getWorldTranslation()));
        }
        else
        {
            lDir += vec3{ 0.f, -0.1f, 0.f };
            light->setDirection(lDir);
            W4_LOG_INFO("Dir light direction: \n%s!", jsprint(lDir));
        }
        return;
        break;
    case platform::KeyboardKey::A:
        if (camControlOn && !editorCamOn)
        {
            Render::getScreenCamera()->translateWorld(vec3{ -m_translateStep, 0.f, 0.0 });
            W4_LOG_INFO("camera worldPos \n%s", jsprint(Render::getScreenCamera()->getWorldTranslation()));
        }
        else
        {
            lDir += vec3{ -0.1f, 0.f, 0.f };
            light->setDirection(lDir);
            W4_LOG_INFO("Dir light direction: \n%s!", jsprint(lDir));
        }
        return;
        break;
    case platform::KeyboardKey::D:
        if (camControlOn && !editorCamOn)
        {
            Render::getScreenCamera()->translateWorld(vec3{ m_translateStep, 0.f, 0.0 });
            W4_LOG_INFO("camera worldPos \n%s", jsprint(Render::getScreenCamera()->getWorldTranslation()));
        }
        else
        {
            lDir += vec3{ 0.1f, 0.f, 0.f };
            light->setDirection(lDir);
            W4_LOG_INFO("Dir light direction: \n%s!", jsprint(lDir));
        }
        return;
        break;
    case platform::KeyboardKey::Q:
        if (camControlOn && !editorCamOn)
        {
            Render::getScreenCamera()->translateWorld(vec3{ 0.f, 0.f, -m_translateStep });
            W4_LOG_INFO("camera worldPos \n%s", jsprint(Render::getScreenCamera()->getWorldTranslation()));
        }
        else
        {
            lDir += vec3{ 0.f, 0.f, -0.1f };
            light->setDirection(lDir);
            W4_LOG_INFO("Dir light direction: \n%s!", jsprint(lDir));
        }
        return; 
        break;
    case platform::KeyboardKey::E:
        if (camControlOn && !editorCamOn)
        {
            Render::getScreenCamera()->translateWorld(vec3{ 0.f, 0.f, m_translateStep });
            W4_LOG_INFO("camera worldPos \n%s", jsprint(Render::getScreenCamera()->getWorldTranslation()));
        }
        else
        {
            lDir += vec3{ 0.f, 0.f, 0.1f };
            light->setDirection(lDir);
            W4_LOG_INFO("Dir light direction: \n%s!", jsprint(lDir));
        }
        return;
        break;
    default:
        //W4_LOG_DEBUG("Debug::some key pressed");
        break;
    }

    for (const auto& cb : m_buttonCb) {
        if (evt.key == cb.first && cb.second) {
            W4_LOG_DEBUG("Callback from setCallbackForButton()");
            cb.second();
            return;
        }
    }

    if (!m_nodeSwitchBoard[m_selectedNode])
    {
        //W4_LOG_INFO("No debug move node selected!");
        return;
    }

    vec3 scale = m_nodeSwitchBoard[m_selectedNode]->getLocalScale();

    switch (evt.key) {
        //Rotation
    case platform::KeyboardKey::Numpad5:
        m_nodeSwitchBoard[m_selectedNode]->rotateLocal(Rotator{ vec3{m_rotStep,0.f, 0.f} });
        break;
    case platform::KeyboardKey::Numpad2:
        m_nodeSwitchBoard[m_selectedNode]->rotateLocal(Rotator{ vec3{-m_rotStep,0.f, 0.f} });
        break;
    case platform::KeyboardKey::Numpad1:
        m_nodeSwitchBoard[m_selectedNode]->rotateLocal(Rotator{ vec3{0.0,m_rotStep, 0.f} });
        break;
    case platform::KeyboardKey::Numpad3:
        m_nodeSwitchBoard[m_selectedNode]->rotateLocal(Rotator{ vec3{0.0,-m_rotStep, 0.f} });
        break;
    case platform::KeyboardKey::Numpad6:
        m_nodeSwitchBoard[m_selectedNode]->rotateLocal(Rotator{ vec3{0.0,0.0, -m_rotStep} });
        break;
    case platform::KeyboardKey::Numpad4:
        m_nodeSwitchBoard[m_selectedNode]->rotateLocal(Rotator{ vec3{0.0,0.0, m_rotStep} });
        break;

        //Translation
    case platform::KeyboardKey::ArrowDown:
        m_nodeSwitchBoard[m_selectedNode]->translateLocal(vec3{ 0.0,-m_translateStep, 0.0 });
        break;
    case platform::KeyboardKey::ArrowUp:
        m_nodeSwitchBoard[m_selectedNode]->translateLocal(vec3{ 0.0,m_translateStep, 0.0 });
        break;
    case platform::KeyboardKey::ArrowLeft:
        m_nodeSwitchBoard[m_selectedNode]->translateLocal(vec3{ -m_translateStep,0.0, 0.0 });
        break;
    case platform::KeyboardKey::ArrowRight:
        m_nodeSwitchBoard[m_selectedNode]->translateLocal(vec3{ m_translateStep,0.0, 0.0 });
        break;
    case platform::KeyboardKey::PageUp:
        m_nodeSwitchBoard[m_selectedNode]->translateLocal(vec3{ 0.0,0.0, m_translateStep });
        break;
    case platform::KeyboardKey::PageDown:
        m_nodeSwitchBoard[m_selectedNode]->translateLocal(vec3{ 0.0,0.0, -m_translateStep });
        break;

        //scale
    case platform::KeyboardKey::NumpadAdd:
        scale *= 1.00 + m_scaleStep;
        m_nodeSwitchBoard[m_selectedNode]->setLocalScale(scale);
        break;
    case platform::KeyboardKey::NumpadSubtract:
        scale *= 1.00 - m_scaleStep;
        m_nodeSwitchBoard[m_selectedNode]->setLocalScale(scale);
        break;
    case platform::KeyboardKey::Home:
        scale.x *= 1.00 + m_scaleStep;
        m_nodeSwitchBoard[m_selectedNode]->setLocalScale(scale);
        break;
    case platform::KeyboardKey::End:
        scale.x *= 1.00 - m_scaleStep;
        m_nodeSwitchBoard[m_selectedNode]->setLocalScale(scale);
        break;
    case platform::KeyboardKey::Insert:
        scale.y *= 1.00 + m_scaleStep;
        m_nodeSwitchBoard[m_selectedNode]->setLocalScale(scale);
        break;
    case platform::KeyboardKey::Delete:
        scale.y *= 1.00 - m_scaleStep;
        m_nodeSwitchBoard[m_selectedNode]->setLocalScale(scale);
        break;
    case platform::KeyboardKey::BracketRight:
        scale.z *= 1.00 + m_scaleStep;
        m_nodeSwitchBoard[m_selectedNode]->setLocalScale(scale);
        break;
    case platform::KeyboardKey::BracketLeft:
        scale.z *= 1.00 - m_scaleStep;
        m_nodeSwitchBoard[m_selectedNode]->setLocalScale(scale);
        break;

    default:
        W4_LOG_DEBUG("DebugNode::Unrecognized key pressed");
        break;
    }

    ND_Util::printDebugNodeInfo(m_nodeSwitchBoard[m_selectedNode]);
}

void NodeDebugger::printConnectedNodes()
{
    if (!isDebuggerOn)
        return;

    std::string nodes = "\nConnected nodes:\n";
    for (size_t i = 1; i <= 9; i++)
    {
        if (m_nodeSwitchBoard[i])
        {   
            nodes += std::to_string(i);
            nodes += " - ";
            nodes += m_nodeSwitchBoard[i]->getName();
            nodes += "\n";
        }
        else
        {
            nodes += std::to_string(i);
            nodes += " - not bound";
            nodes += "\n";
        }
    }
    if (m_nodeSwitchBoard[0])
    {
        nodes += std::to_string(0);
        nodes += " - ";
        nodes += m_nodeSwitchBoard[0]->getName();
        nodes += "\n";
    }
    else
    {
        nodes += std::to_string(0);
        nodes += " - not bound";
        nodes += "\n";
    }

    W4_LOG_INFO("%s", nodes.c_str());
}

void NodeDebugger::onUpdate(float dt)
{
    ND_Util::updateTweens(dt);
    ImGuiUpdate(dt);
}

#endif //ends #ifndef FORCENODEDEBUG_OFF - conditional compilation macro. If FORCENODEDEBUG_OFF is defined, NOCOMP macro provides empty implementation and saves some weight in build size