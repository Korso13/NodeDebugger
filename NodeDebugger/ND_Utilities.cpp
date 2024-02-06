#include "ND_Utilities.h"
#include "external/imgui_stdlib.h"
#include <regex>

#ifndef FORCENODEDEBUG_OFF

namespace ND_Util
{
    std::string jsstring(vec3 vec)
    {
        std::string jstring;
        std::string extract;
        int pops = 6 - floatAccuracy;
        if (pops < 0) pops = 0;

        jstring += "{ \"x\": ";
        extract = std::to_string(vec.x);
        for (size_t i = 0; i < pops; i++)
        {
            extract.pop_back();
        }
        jstring += extract;
        extract.clear();

        jstring += ", \"y\": ";
        extract = std::to_string(vec.y);
        for (size_t i = 0; i < pops; i++)
        {
            extract.pop_back();
        }
        jstring += extract;
        extract.clear();

        jstring += ", \"z\": ";
        extract = std::to_string(vec.z);
        for (size_t i = 0; i < pops; i++)
        {
            extract.pop_back();
        }
        jstring += extract;
        extract.clear();

        jstring += "},";
        return jstring;
    }

    vec2 vec2FromGui(float inVec[2])
    {
        return vec2(inVec[0], inVec[1]);
    }

    vec3 vec3FromGui(float inVec[3])
    {
        return vec3(inVec[0], inVec[1], inVec[2]);
    }

    vec3 vec3FromJSString(std::string jsstring)
    {
        if (jsstring.empty())
            return vec3::zero;

        vec3 result = vec3::zero;

        int xCoordStart = jsstring.find("\"x\":", 0);
        int yCoordStart = jsstring.find("\"y\":", xCoordStart + 4);
        int zCoordStart = jsstring.find("\"z\":", yCoordStart + 4);
        int recEnd = jsstring.find("}", zCoordStart + 4);

        if (xCoordStart == jsstring.npos
            || yCoordStart == jsstring.npos
            || zCoordStart == jsstring.npos
            || recEnd == jsstring.npos)
            return vec3::zero;

        int iter = xCoordStart + 4;
        std::string nextValue;
        for (; iter < yCoordStart; iter++)
        {
            if (iter >= jsstring.size())
                break;
            if (jsstring[iter] == ' ')
                continue;
            if (jsstring[iter] == ',')
                break;

            nextValue += jsstring[iter];
        }
        result.x = std::atof(nextValue.c_str());

        nextValue.clear();
        iter = yCoordStart + 4;
        for (; iter < zCoordStart; iter++)
        {
            if (iter >= jsstring.size())
                break;
            if (jsstring[iter] == ' ')
                continue;
            if (jsstring[iter] == ',')
                break;

            nextValue += jsstring[iter];
        }
        result.y = std::atof(nextValue.c_str());

        nextValue.clear();
        iter = zCoordStart + 4;
        for (; iter < recEnd; iter++)
        {
            if (iter >= jsstring.size())
                break;
            if (jsstring[iter] == ' ')
                continue;
            if (jsstring[iter] == '}')
                break;

            nextValue += jsstring[iter];
        }
        result.z = std::atof(nextValue.c_str());

        return result;
    }

    void updateTweens(float& dt)
    {
        for (auto& [node, tweenRec] : debugTweensMove)
        {
            if (tweenRec.tween)
            {
                tweenRec.tween->updValue(dt);
                tweenRec.tween->checkCb();
                if (tweenRec.tween->checkIsDone())
                    tweenRec.tween = nullptr;
            }

            if (tweenRec.tweenEndless)
            {
                vec3 newValue = tweenRec.tweenEndless->updValue(dt);
                if (tweenRec.moveTween)
                    if (tweenRec.worldVec3)
                        node->setWorldTranslation(newValue);
                    else
                        node->setLocalTranslation(newValue);
                else
                    if (tweenRec.worldVec3)
                        node->setWorldScale(newValue);
                    else
                        node->setLocalScale(newValue);
            }
        }
        for (auto& [node, tweenRec] : debugTweensScale)
        {
            if (tweenRec.tween)
            {
                tweenRec.tween->updValue(dt);
                tweenRec.tween->checkCb();
                if (tweenRec.tween->checkIsDone())
                    tweenRec.tween = nullptr;
            }

            if (tweenRec.tweenEndless)
            {
                vec3 newValue = tweenRec.tweenEndless->updValue(dt);
                if (tweenRec.moveTween)
                    if (tweenRec.worldVec3)
                        node->setWorldTranslation(newValue);
                    else
                        node->setLocalTranslation(newValue);
                else
                    if (tweenRec.worldVec3)
                        node->setWorldScale(newValue);
                    else
                        node->setLocalScale(newValue);
            }
        }
    }

    sptr<CustomTweens::Tween<vec3>> generateTween(tweenDebugRecord& tweenRecord, sptr<Node> _node)
    {
        if (tweenRecord.tweenDelays.empty() || tweenRecord.tweenPoints.empty())
            return nullptr;

        auto tween = make::sptr<CustomTweens::Tween<vec3>>(
            tweenRecord.tweenStart,
            [tweenRecord, _node](vec3 newValue)
            {
                if (!_node)
                    return;

                if (tweenRecord.moveTween)
                    if (tweenRecord.worldVec3)
                        _node->setWorldTranslation(newValue);
                    else
                        _node->setLocalTranslation(newValue);
                else
                    if (tweenRecord.worldVec3)
                        _node->setWorldScale(newValue);
                    else
                        _node->setLocalScale(newValue);
            });

        for (size_t i = 0; i < tweenRecord.tweenPoints.size(); i++)
        {
            tween->addMore(
                tweenRecord.tweenPoints[i],
                tweenRecord.tweenDelays[i],
                tweenRecord.easingTypes[i],
                []()
                {});
        }

        return tween;
    }

    sptr<CustomTweens::EndlessTween<vec3>> generateEndlessTween(tweenDebugRecord& tweenRecord, sptr<Node> _node)
    {
        if (tweenRecord.tweenDelays.empty() || tweenRecord.tweenPoints.empty())
            return nullptr;

        sptr<CustomTweens::EndlessTween<vec3>> tween = make::sptr<CustomTweens::EndlessTween<vec3>>(tweenRecord.tweenStart, "new_tween");
        float nowTime = 0.f;
        for (size_t i = 0; i < tweenRecord.tweenPoints.size(); i++)
        {
            nowTime += tweenRecord.tweenDelays[i];
            tween->add(tweenRecord.tweenPoints[i], nowTime, tweenRecord.easingTypes[i]);
        }
        return tween;
    }

    void printDebugNodeInfo(sptr<Node> printNode)
    {
        if (!isDebuggerOn)
            return;

        vec3 rot = printNode->getLocalRotation().euler();
        if (degreesOutput) rot *= RAD2DEG;
        vec3 rotWorld = printNode->getWorldRotation().euler();
        if (degreesOutput) rotWorld *= RAD2DEG;
        std::string name = printNode->getName();
        std::string format = (degreesOutput) ? ("degrees") : ("radians");

        W4_LOG_DEBUG("========================");
        W4_LOG_INFO("\nDebug node \"%s\" transform:\nlocal position:\n%s\nworld position:\n%s\nlocal rotation in %s:\n%s\nworld rotation in %s:\n%s\nlocal scale:\n%s\nworld scale:\n%s",
            name.c_str(),
            jsprint(printNode->getLocalTranslation()),
            jsprint(printNode->getWorldTranslation()),
            format.c_str(), jsprint(rot),
            format.c_str(), jsprint(rotWorld),
            jsprint(printNode->getLocalScale()),
            jsprint(printNode->getWorldScale())
        );
    }

    void printKeymapping()
    {
        W4_LOG_INFO("%s", keyMappings.c_str());
    }

    void setSkin(sptr<Spine> spine, const std::string& name)
    {
        spine->skeleton->setSkin(name.c_str());
        spine->skeleton->setBonesToSetupPose();
        spine->skeleton->setSlotsToSetupPose();
        spine->state->apply(*spine->skeleton);
    }

    void addSkin(sptr<Spine> spine, const std::string& name)
    {
        auto skeletonData = spine->skeleton->getData();
        auto newSkin = new spine::Skin(*spine->skeleton->getSkin());

        newSkin->addSkin(skeletonData->findSkin(spine->skeleton->getSkin()->getName()));
        newSkin->addSkin(skeletonData->findSkin(name.c_str()));

        debug("Skin %s \n exist=%s", name.c_str(), std::to_string(skeletonData->findSkin(name.c_str()) != nullptr).c_str());
        spine->skeleton->setSkin(newSkin);

        spine->skeleton->setBonesToSetupPose();
        spine->skeleton->setSlotsToSetupPose();
        spine->state->apply(*spine->skeleton);
    }

    void ImGuiCopyVec3Btn(vec3 vec)
    {
        ImGui::PushID(repeatElemId++);

        if (ImGui::Button("Copy"))
        {
            ImGui::SetClipboardText(jsprint(vec));
        }
        ImGui::SameLine();

        ImGui::PopID();
    }

    void ImGuiTransforms(sptr<Node> _node)
    {
        //translation sliders
        ImGui::Separator();
        bool posChangedL = false;
        bool posChangedW = false;
        vec3 posWorld = _node->getWorldTranslation();
        vec3 posLocal = _node->getLocalTranslation();
        float posW[3] = { posWorld.x, posWorld.y, posWorld.z };
        float posL[3] = { posLocal.x, posLocal.y, posLocal.z };
        ImGuiCopyVec3Btn(vec3FromGui(posW));
        posChangedW = ImGui::DragFloat3("Position world", posW, sliderSensivity, -IMG_POSRANGE, IMG_POSRANGE);
        ImGuiCopyVec3Btn(vec3FromGui(posL));
        posChangedL = ImGui::DragFloat3("Position local", posL, sliderSensivity, -IMG_POSRANGE, IMG_POSRANGE);
        if (posChangedW)
            _node->setWorldTranslation(vec3FromGui(posW));
        if (posChangedL)
            _node->setLocalTranslation(vec3FromGui(posL));

        //rotation sliders
        ImGui::Separator();
        bool rotChangedL = false;
        bool rotChangedW = false;
        vec3 rotWorld = _node->getWorldRotation().euler();
        vec3 rotLocal = _node->getLocalRotation().euler();
        if (degreesOutput) rotWorld *= RAD2DEG;
        if (degreesOutput) rotLocal *= RAD2DEG;
        float rotW[3] = { rotWorld.x, rotWorld.y, rotWorld.z };
        float rotL[3] = { rotLocal.x, rotLocal.y, rotLocal.z };
        ImGuiCopyVec3Btn(vec3FromGui(rotW));
        rotChangedW = ImGui::DragFloat3("Rotation world", rotW, sliderSensivity, (degreesOutput) ? -IMG_ROTRANGE_DEG : -IMG_ROTRANGE_RAD, (degreesOutput) ? IMG_ROTRANGE_DEG : IMG_ROTRANGE_RAD);
        ImGuiCopyVec3Btn(vec3FromGui(rotL));
        rotChangedL = ImGui::DragFloat3("Rotation local", rotL, sliderSensivity, (degreesOutput) ? -IMG_ROTRANGE_DEG : -IMG_ROTRANGE_RAD, (degreesOutput) ? IMG_ROTRANGE_DEG : IMG_ROTRANGE_RAD);
        if (rotChangedW)
        {
            vec3 newValW = vec3FromGui(rotW);
            if (degreesOutput) newValW *= DEG2RAD;
            _node->setWorldRotation(Rotator(newValW));
        }
        if (rotChangedL)
        {
            vec3 newValL = vec3FromGui(rotL);
            if (degreesOutput) newValL *= DEG2RAD;
            _node->setLocalRotation(Rotator(newValL));
        }

        if (!_node->is<Camera>())
        {
            //scale sliders
            ImGui::Separator();
            bool scaleChangedW = false;
            bool scaleChangedL = false;
            vec3 scaleWorld = _node->getWorldScale();
            vec3 scaleLocal = _node->getLocalScale();
            float scaleW[3] = { scaleWorld.x, scaleWorld.y, scaleWorld.z };
            float scaleL[3] = { scaleLocal.x, scaleLocal.y, scaleLocal.z };
            ImGuiCopyVec3Btn(vec3FromGui(scaleW));
            scaleChangedW = ImGui::DragFloat3("Scale world", scaleW, sliderSensivity, 0.0001, IMG_SCALERANGE);
            ImGuiCopyVec3Btn(vec3FromGui(scaleL));
            scaleChangedL = ImGui::DragFloat3("Scale local", scaleL, sliderSensivity, 0.0001, IMG_SCALERANGE);
            if (scaleChangedW)
            {
                if (bindXYZScaling)
                {
                    vec3 newScale = vec3FromGui(scaleW);
                    if (newScale.x != scaleWorld.x)
                    {
                        newScale.y = newScale.x;
                        newScale.z = newScale.x;
                    }
                    if (newScale.y != scaleWorld.y)
                    {
                        newScale.x = newScale.y;
                        newScale.z = newScale.y;
                    }
                    if (newScale.z != scaleWorld.z)
                    {
                        newScale.x = newScale.z;
                        newScale.y = newScale.z;
                    }
                    if (newScale.x != 0.f)
                        _node->setWorldScale(newScale);
                }
                else
                    _node->setWorldScale(vec3FromGui(scaleW));
            }
            if (scaleChangedL)
            {
                if (bindXYZScaling)
                {
                    vec3 newScale = vec3FromGui(scaleL);
                    if (newScale.x != scaleLocal.x)
                    {
                        newScale.y = newScale.x;
                        newScale.z = newScale.x;
                    }
                    if (newScale.y != scaleLocal.y)
                    {
                        newScale.x = newScale.y;
                        newScale.z = newScale.y;
                    }
                    if (newScale.z != scaleLocal.z)
                    {
                        newScale.x = newScale.z;
                        newScale.y = newScale.z;
                    }
                    if (newScale.x != 0.f)
                        _node->setLocalScale(newScale);
                }
                else
                    _node->setLocalScale(vec3FromGui(scaleL));
            }
        }
    }

    void ImGuiSettings()
    {
        ImGui::SliderInt("% Game timescale", &timescale, 0, 500);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    void ImGuiSearchBar(std::string& lastSearchVal)
    {
        ImGui::PushID(repeatElemId++);
        ImGui::Text("%s", "Filter: ");
        ImGui::SameLine();
        ImGui::InputText(" ", &lastSearchVal);
        ImGui::Separator();

        ImGui::PopID();
    }

    bool checkFilterConditions(const std::string& itemName, const std::string& filter)
    {
        if(filter.empty())
            return true;

        std::regex regex(filter, std::regex::icase);
        std::smatch name_match;

        return std::regex_search(itemName, name_match, regex);
    }

    void resetImGuiRepeatElemId()
    {
        repeatElemId = 0;
    }

    float getTimeScale()
    {
        return timescale / 100.f;
    }

    void ImGuiNodeSettings()
    {
        ImGui::SliderFloat("Sliders sensivity", &sliderSensivity, 0.0001f, 1.0f, "%.4f");
        ImGui::Checkbox("Rotation in degrees", &degreesOutput);
        ImGui::Checkbox("Single scale value", &bindXYZScaling);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    //reusable ImGui form for different types of vec3 tweens (used in ImGuiTweens)
    void ImGuiTweensBody(sptr<Node> _node, tweenDebugRecord& tweenRecord)
    {
        if (ImGui::Checkbox("Looped tween", &tweenRecord.loopedTween))
        {
            tweenRecord.tweenEndless = nullptr;
            tweenRecord.tween = nullptr;
        }
        ImGui::Checkbox("World vec3 values?", &tweenRecord.worldVec3);

        if (ImGui::Button("Set current node value"))
        {
            if (tweenRecord.moveTween)
                if (tweenRecord.worldVec3)
                    tweenRecord.tweenStart = _node->getWorldTranslation();
                else
                    tweenRecord.tweenStart = _node->getLocalTranslation();
            else
                if (tweenRecord.worldVec3)
                    tweenRecord.tweenStart = _node->getWorldScale();
                else
                    tweenRecord.tweenStart = _node->getLocalScale();
        }
        if (ImGui::Button("Insert from clipboard(JSON string)")) //Due to ImGui bug it copies correct CB data only on second push :(
        {
            std::string cbText = ImGui::GetClipboardText();
            //info("Extracted string from CB: %s", cbText.c_str());
            tweenRecord.tweenStart = vec3FromJSString(cbText);
        }
        ImGui::DragVec3("Start point", tweenRecord.tweenStart, -500.f, 500.f, sliderSensivity);

        //tween points settings UI render
        if (!tweenRecord.tweenPoints.empty())
            for (size_t i = 0; i < tweenRecord.tweenPoints.size(); i++)
            {
                ImGui::Text("Point %d", CastI(i + 1));

                //Point value setters
                if (ImGui::Button(CSTR("Set point " + STR(i + 1) + " to current value")))
                {
                    if (tweenRecord.moveTween)
                        if (tweenRecord.worldVec3)
                            tweenRecord.tweenPoints[i] = _node->getWorldTranslation();
                        else
                            tweenRecord.tweenPoints[i] = _node->getLocalTranslation();
                    else
                        if (tweenRecord.worldVec3)
                            tweenRecord.tweenPoints[i] = _node->getWorldScale();
                        else
                            tweenRecord.tweenPoints[i] = _node->getLocalScale();
                }
                if (ImGui::Button(CSTR("Set point " + STR(i + 1) + " to clipboard value(JSON string)"))) //Due to ImGui bug it copies correct CB data only on second push :(
                {
                    std::string cbText = ImGui::GetClipboardText();
                    //info("Extracted string from CB: %s", cbText.c_str());
                    tweenRecord.tweenPoints[i] = vec3FromJSString(cbText);
                }

                //vec, time, easing values
                ImGui::DragVec3(CSTR("Vec " + STR(i + 1)), tweenRecord.tweenPoints[i], -500.f, 500.f, sliderSensivity);
                ImGui::InputFloat(CSTR("Time " + STR(i + 1)), &tweenRecord.tweenDelays[i], 0.01, 0.1, "%.2f");
                int intEasingType = CastI(tweenRecord.easingTypes[i]);
                if (ImGui::Combo(CSTR("Easing " + STR(i + 1)), &intEasingType, easingNames, 31))
                {
                    tweenRecord.easingTypes[i] = static_cast<Easing>(intEasingType);
                }
                if (ImGui::Button(CSTR("Delete point" + STR(i + 1))))
                {
                    tweenRecord.tweenPoints.erase(tweenRecord.tweenPoints.begin() + i);
                    tweenRecord.tweenDelays.erase(tweenRecord.tweenDelays.begin() + i);
                    tweenRecord.easingTypes.erase(tweenRecord.easingTypes.begin() + i);
                    break;
                }
                ImGui::Separator();
            }

        ImGui::Spacing();
        //generates new points for tween
        if (ImGui::Button("Add new point"))
        {
            tweenRecord.tweenPoints.push_back(vec3::zero);
            if (tweenRecord.moveTween)
                if (tweenRecord.worldVec3)
                    tweenRecord.tweenPoints.back() = _node->getWorldTranslation();
                else
                    tweenRecord.tweenPoints.back() = _node->getLocalTranslation();
            else
                if (tweenRecord.worldVec3)
                    tweenRecord.tweenPoints.back() = _node->getWorldScale();
                else
                    tweenRecord.tweenPoints.back() = _node->getLocalScale();

            if (tweenRecord.tweenDelays.empty())
                tweenRecord.tweenDelays.push_back(0.f);
            else
                tweenRecord.tweenDelays.push_back(tweenRecord.tweenDelays.back());
            if (tweenRecord.easingTypes.empty())
                tweenRecord.easingTypes.push_back(Easing::Linear);
            else
                tweenRecord.easingTypes.push_back(tweenRecord.easingTypes.back());
        }
        if (ImGui::Button("Play tween"))
        {

            if (!tweenRecord.loopedTween)
                tweenRecord.tween = generateTween(tweenRecord, _node);
            else
                tweenRecord.tweenEndless = generateEndlessTween(tweenRecord, _node);
        }

        if (ImGui::Button("Stop tween"))
        {
            tweenRecord.tween = nullptr;
            tweenRecord.tweenEndless = nullptr;
        }
    }

    //reusable tween builder menu
    void ImGuiTweens(sptr<Node> _node)
    {
        if (!_node)
            return;

        ImGui::Separator();
        if (ImGui::TreeNode("Tween builder"))
        {
            //creating records if this is the first time opening the branch
            if (debugTweensMove.find(_node) == debugTweensMove.end())
            {
                debugTweensMove.insert({ _node, tweenDebugRecord() });
                debugTweensScale.insert({ _node, tweenDebugRecord() });

                debugTweensMove[_node].moveTween = true;
                if (debugTweensMove[_node].worldVec3)
                    debugTweensMove[_node].tweenStart = _node->getWorldTranslation();
                else
                    debugTweensMove[_node].tweenStart = _node->getLocalTranslation();

                debugTweensScale[_node].moveTween = false;
                if (debugTweensScale[_node].worldVec3)
                    debugTweensScale[_node].tweenStart = _node->getWorldScale();
                else
                    debugTweensScale[_node].tweenStart = _node->getLocalScale();
            }

            tweenDebugRecord& tweenRecordMove = debugTweensMove[_node];
            tweenDebugRecord& tweenRecordScale = debugTweensScale[_node];
            //Movement tween branch
            if (ImGui::TreeNode("Movement tween"))
            {
                ImGuiTweensBody(_node, tweenRecordMove);

                ImGui::TreePop();
            }
            //Scale tween branch
            if (ImGui::TreeNode("Scale tween"))
            {
                ImGuiTweensBody(_node, tweenRecordScale);

                ImGui::TreePop();
            }

            //buttons for simultaneous launch/stop
            if (ImGui::Button("Play all tweens"))
            {

                if (!tweenRecordMove.loopedTween)
                    tweenRecordMove.tween = generateTween(tweenRecordMove, _node);
                else
                    tweenRecordMove.tweenEndless = generateEndlessTween(tweenRecordMove, _node);
                if (!tweenRecordScale.loopedTween)
                    tweenRecordScale.tween = generateTween(tweenRecordScale, _node);
                else
                    tweenRecordScale.tweenEndless = generateEndlessTween(tweenRecordScale, _node);
            }
            if (ImGui::Button("Stop all tweens"))
            {
                tweenRecordMove.tween = nullptr;
                tweenRecordMove.tweenEndless = nullptr;
                tweenRecordScale.tween = nullptr;
                tweenRecordScale.tweenEndless = nullptr;
            }

            ImGui::TreePop();
        }
    }
}

#endif