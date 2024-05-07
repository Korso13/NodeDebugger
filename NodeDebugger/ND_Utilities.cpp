#include "ND_Utilities.h"

#ifndef FORCENODEDEBUG_OFF
#include <sources/common/NodeDebugger/ND_JsonTools.h>
#include "external/imgui_stdlib.h"
#include <regex>
#include "TweenFactory.h"
#include <utils/EditorCamera.h>
#include "external/addonPlot/implot.h"

#define TWEENCONTAINER TweenContainer::getTweenContainer()

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
                TweenContainer::nodeValueSetter(newValue, node, tweenRec);
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
                TweenContainer::nodeValueSetter(newValue, node, tweenRec);
            }
        }
        for (auto& [node, tweenRec] : debugTweensRotate)
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
                TweenContainer::nodeValueSetter(newValue, node, tweenRec);
            }
        }
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
        if (ImGui::Button("Center on camera"))
        {
            vec3 target = Render::getScreenCamera()->getWorldTranslation();
            target.z = 0.f;
            _node->setWorldTranslation(target);
        }

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

        if (!_node->is<Camera>() && !_node->is<EditorCamera>())
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
        if (ImGui::Button("100%"))
        {
            Game::getInstance()->setTimeScale(1.f);
        }
        ImGui::SameLine();
        timescale = Game::getInstance()->getTimeScale()*100.f;
        if (ImGui::SliderInt("% Game timescale", &timescale, 0, 500))
        {
            Game::getInstance()->setTimeScale(timescale/100.f);
        }
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    bool ImGuiSearchBar(std::string& lastSearchVal)
    {
        ImGui::PushID(repeatElemId++);

        ImGui::Text("%s", "Filter: ");
        ImGui::SameLine();
        bool isSearchChanged = ImGui::InputText(" ", &lastSearchVal);
        ImGui::Separator();

        ImGui::PopID();

        return isSearchChanged;
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

    void resetPlotterWindowFlag()
    {
        needDrawPlotterWindow = false;
    }

    ImVec2 getSizeForChild(float windowHeightPercent)
    {
        ImVec2 size = ImGui::GetWindowSize();
        size.y *= windowHeightPercent;
        size.x *= 0.95;

        return size;
    }

    void ImGuiNodeSettings()
    {
        ImGui::DragFloat("Sliders sensivity", &sliderSensivity, 0.001, 0.001f, 10.0f, "%.4f");
        ImGui::Checkbox("Rotation in degrees", &degreesOutput);
        ImGui::Checkbox("Single scale value", &bindXYZScaling);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    //reusable ImGui form for different types of vec3 tweens (used in ImGuiTweens)
    void ImGuiTweensBody(sptr<Node> _node, tweenDebugRecord& tweenRecord)
    {
        //tween sample (settings container item) loader
        if (!TWEENCONTAINER.empty())
        {
            ImGui::Columns(2);
            const char* selectedSample = "Select a sample";
            if (lastSelectedTweenSample[tweenRecord.transformType] != -1)
                selectedSample = TWEENCONTAINER[lastSelectedTweenSample[tweenRecord.transformType]].tweenName.c_str();

            if (ImGui::BeginCombo("Samples", selectedSample))
            {
                for (size_t i = 0; i < TWEENCONTAINER.size(); i++)
                {
                    if (tweenRecord.transformType == TWEENCONTAINER[i].transformType)
                    {
                        bool bSelected = i == lastSelectedTweenSample[tweenRecord.transformType];
                        if (ImGui::Selectable(TWEENCONTAINER[i].tweenName.c_str(), &bSelected))
                        {
                            lastSelectedTweenSample[tweenRecord.transformType] = i;
                        }
                    }
                }
                
                ImGui::EndCombo();
            }

            ImGui::NextColumn();
            if (lastSelectedTweenSample[tweenRecord.transformType] != -1)
            {
                std::string buttonCaption = "Load ";
                buttonCaption += TWEENCONTAINER[lastSelectedTweenSample[tweenRecord.transformType]].tweenName;
                if (ImGui::Button(buttonCaption.c_str()))
                {
                    tweenRecord = TWEENCONTAINER[lastSelectedTweenSample[tweenRecord.transformType]];
                }
            }
            ImGui::Separator();
            ImGui::Columns(1);
        }
        
        //save+export buttons
        ImGui::Columns(2);
        if (ImGui::Button("Save tween to Samples"))
        {
            TweenContainer::addNewTweenSample(tweenRecord);
        }
        ImGui::NextColumn();
        if (ImGui::Button("Export samples to JSON"))
        {
            TweenContainer::exportTweenContainerToJson();
        }
        ImGui::Columns(1);

        //Tween sample settings
        ImGui::InputText("Tween name", &tweenRecord.tweenName);

        if (ImGui::Checkbox("Looped tween", &tweenRecord.isLoopedTween))
        {
            tweenRecord.tweenEndless = nullptr;
            tweenRecord.tween = nullptr;
        }
        ImGui::Checkbox("World vec3 values?", &tweenRecord.isWorldVec3);
        ImGui::Columns(2);
        ImGui::Checkbox("Relative values", &tweenRecord.isRelativeValues);
        ImGui::NextColumn();
        if (tweenRecord.isRelativeValues)
        {
            ImGui::Checkbox("Relative values in %% ?", &tweenRecord.isPercentRelativeValues);
        }
        ImGui::Columns(1);



        //start point setter
        ImGui::Text("Tween start point");
        ImGui::Columns(2);
        if (ImGui::Button("Set start to current value"))
        {
            tweenRecord.tweenStart = TweenContainer::nodeValueGetter(_node, tweenRecord);
        }
        ImGui::NextColumn();
        if (ImGui::Button("Insert from clipboard(JSON string)")) //Due to ImGui bug it copies correct CB data only on second push :(
        {
            std::string cbText = ImGui::GetClipboardText();
            //info("Extracted string from CB: %s", cbText.c_str());
            tweenRecord.tweenStart = vec3FromJSString(cbText);
        }
        ImGui::Columns(1);

        //tween plotter
        separatePlotterWindow_node = _node;
        separatePlotterWindow_recordType = tweenRecord.transformType;
        if (!callSeparatePlotterWindow)
            ImGuiTweenPlotter(_node, tweenRecord);
        else if(!tweenRecord.tweenPoints.empty())
            needDrawPlotterWindow = true;

        //tween points scroll zone
        float pointsScrollzoneSize = 0.1;
        if (tweenRecord.tweenPoints.size() == 1)
            pointsScrollzoneSize = 0.27;
        else if(tweenRecord.tweenPoints.size() > 1)
            pointsScrollzoneSize = 0.4;
        if (ImGui::BeginChild("Tween points scroll zone", getSizeForChild(pointsScrollzoneSize)))
        {
            ImGui::DragVec3("Start point", tweenRecord.tweenStart, -500.f, 500.f, sliderSensivity);

            //tween points settings UI render
            if (!tweenRecord.tweenPoints.empty())
            {
                for (size_t i = 0; i < tweenRecord.tweenPoints.size(); i++)
                {
                    ImGui::Text("Point %d", CastI(i + 1));

                    //Point value setters
                    ImGui::Columns(2);
                    if (ImGui::Button(CSTR("Set point " + STR(i + 1) + " to current value")))
                    {
                        tweenRecord.tweenPoints[i] = TweenContainer::nodeValueGetter(_node, tweenRecord);
                    }
                    ImGui::NextColumn();
                    if (ImGui::Button(CSTR("Set point " + STR(i + 1) + " to clipboard value(JSON string)"))) //Due to ImGui bug it copies correct CB data only on second push :(
                    {
                        std::string cbText = ImGui::GetClipboardText();
                        //info("Extracted string from CB: %s", cbText.c_str());
                        tweenRecord.tweenPoints[i] = vec3FromJSString(cbText);
                    }
                    ImGui::Columns(1);

                    //vec, time selector
                    ImGui::DragVec3(CSTR("Vec " + STR(i + 1)), tweenRecord.tweenPoints[i], -500.f, 500.f, sliderSensivity);
                    ImGui::InputFloat(CSTR("Time " + STR(i + 1)), &tweenRecord.tweenDelays[i], 0.01, 0.1, "%.2f");

                    //Easings selector
                    int intEasingType = CastI(tweenRecord.easingTypes[i]); //selected easing type
                    //one-time script to load easings' names
                    if (!loadedEasings)
                    {
                        loadedEasings = true;
                        int easingsCount;
                        const auto* easings = nlohmann::adl_serializer<Easing>::getValues(easingsCount);
                        for (size_t i = 0; i < easingsCount; i++)
                        {
                            easingNames[i] = easings[i].second;
                        }
                    }
                    //easings drop-down menu
                    if (ImGui::Combo(CSTR("Easing " + STR(i + 1)), &intEasingType, easingNames, 31))
                    {
                        tweenRecord.easingTypes[i] = static_cast<Easing>(intEasingType);
                    }
                    ImGui::Spacing();

                    if (ImGui::Button(CSTR("Insert point before point " + STR(i + 1))))
                    {
                        tweenRecord.tweenPoints.insert(tweenRecord.tweenPoints.begin() + i, tweenRecord.tweenPoints[i]);
                        tweenRecord.tweenDelays.insert(tweenRecord.tweenDelays.begin() + i, tweenRecord.tweenDelays[i]);
                        tweenRecord.easingTypes.insert(tweenRecord.easingTypes.begin() + i, tweenRecord.easingTypes[i]);
                        break;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(CSTR("Delete point" + STR(i + 1))))
                    {
                        tweenRecord.tweenPoints.erase(tweenRecord.tweenPoints.begin() + i);
                        tweenRecord.tweenDelays.erase(tweenRecord.tweenDelays.begin() + i);
                        tweenRecord.easingTypes.erase(tweenRecord.easingTypes.begin() + i);
                        break;
                    }
                    ImGui::Spacing();
                    ImGui::Separator();
                }
            }
        }
        ImGui::EndChild();

        ImGui::Spacing();
        //generates new points for tween
        if (ImGui::Button("Add new point"))
        {
            tweenRecord.tweenPoints.push_back(vec3::zero);
            tweenRecord.tweenPoints.back() = TweenContainer::nodeValueGetter(_node, tweenRecord);

            if (tweenRecord.tweenDelays.empty())
                tweenRecord.tweenDelays.push_back(0.f);
            else
                tweenRecord.tweenDelays.push_back(tweenRecord.tweenDelays.back());
            if (tweenRecord.easingTypes.empty())
                tweenRecord.easingTypes.push_back(Easing::Linear);
            else
                tweenRecord.easingTypes.push_back(tweenRecord.easingTypes.back());
        }
        ImGui::Columns(2);
        if (ImGui::Button("Play tween"))
        {

            if (!tweenRecord.isLoopedTween)
                tweenRecord.tween = TweenContainer::generateTween(tweenRecord, _node, nullptr, nullptr, true);
            else
                tweenRecord.tweenEndless = TweenContainer::generateEndlessTween(tweenRecord, _node, true);
        }
        ImGui::NextColumn();
        if (ImGui::Button("Stop tween"))
        {
            tweenRecord.tween = nullptr;
            tweenRecord.tweenEndless = nullptr;
        }
        ImGui::Columns(1);
    }

    struct vec3D
    {
        vec3D(vec3 inVec) { x = static_cast<double>(inVec.x); y = static_cast<double>(inVec.y); z = static_cast<double>(inVec.z); };
        vec3 getVec3() const { return vec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)); }
        double x;
        double y;
        double z;
    };
    static bool tweenPlotterInputBlocked = false;
#define PLOTTER_INPUTBLOCKCHECK ((tweenPlotterInputBlocked) ? ImPlotDragToolFlags_NoInputs : 0)

    void ImGuiTweenPlotter(sptr<Node> _node, tweenDebugRecord& tweenRecord)
    {
        if (tweenRecord.tweenPoints.empty())
            return;

        if(ImPlot::GetCurrentContext() == NULL)
            ImPlot::CreateContext();

        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Checkbox("Separate Plotter window", &callSeparatePlotterWindow);
        ImGui::Checkbox("Block points drag", &tweenPlotterInputBlocked);
        ImGui::PushID(repeatElemId++);

        if (ImPlot::BeginPlot("##tweenlines", !callSeparatePlotterWindow ? getSizeForChild(0.4) : ImVec2(-1, -1)))
        {
            double minValXPositions = 0.f;
            double minValYPositions = 0.f;
            double maxValXPositions = -1000.f;            
            double maxValYPositions = -1000.f;

            vec3D startPoint(tweenRecord.tweenStart);
            float tweenDuration = std::accumulate(tweenRecord.tweenDelays.begin(), tweenRecord.tweenDelays.end(), 0.f);

            //min-max values need to be calculated before any points on teh graph
            if (startPoint.x < minValXPositions)
                minValXPositions = startPoint.x;
            if (startPoint.y < minValYPositions)
                minValYPositions = startPoint.y;
            if (startPoint.x > maxValXPositions)
                maxValXPositions = startPoint.x;
            if (startPoint.y > maxValYPositions)
                maxValYPositions = startPoint.y;

            if (tweenDuration > maxValYPositions)
                maxValYPositions = tweenDuration;

            for (size_t i = 0; i < tweenRecord.tweenPoints.size(); i++)
            {
                vec3D nextPoint(tweenRecord.tweenPoints[i]);
                if (nextPoint.x < minValXPositions)
                    minValXPositions = nextPoint.x;
                if (nextPoint.y < minValYPositions)
                    minValYPositions = nextPoint.y;
                if (nextPoint.x > maxValXPositions)
                    maxValXPositions = nextPoint.x;
                if (nextPoint.y > maxValYPositions)
                    maxValYPositions = nextPoint.y;
            }
            ImPlot::SetupAxes("X / Now Time", "Y");
            ImPlot::SetupAxesLimits(minValXPositions, maxValXPositions, minValYPositions, maxValYPositions);
            ImPlot::SetupLegend(ImPlotLocation_North, ImPlotLegendFlags_Horizontal);

            if (_node)
            {
                vec3D nodePoint(TweenContainer::nodeValueGetter(_node, tweenRecord));
                if (ImPlot::DragPoint(-1, &nodePoint.x, &nodePoint.y, IMG_RED, 4, PLOTTER_INPUTBLOCKCHECK))
                {
                    TweenContainer::nodeValueSetter(nodePoint.getVec3(), _node, tweenRecord);
                }
            }
            
            //tween to be used in plotting the line
            auto tween = make::sptr<Tween<vec3>>(tweenRecord.tweenStart);
            float nowTime = 0.f;

            if (ImPlot::DragPoint(0, &startPoint.x, &startPoint.y, IMG_VIOLET, 4, PLOTTER_INPUTBLOCKCHECK))
            {
                tweenRecord.tweenStart = startPoint.getVec3();
            }
            ImPlot::Annotation(startPoint.x, startPoint.y, IMG_VIOLET, ImVec2(15, -15), false, "%s", "Start Point");

            for (size_t i = 0; i < tweenRecord.tweenPoints.size(); i++)
            {
                float nowTimeStamp = nowTime; //to be used for timeline points altering
                nowTime += tweenRecord.tweenDelays[i];
                tween->add(tweenRecord.tweenPoints[i], nowTime, tweenRecord.easingTypes[i]);
                
                vec3D nextPoint(tweenRecord.tweenPoints[i]);
                double xTime = static_cast<double>(nowTime);
                double yTime = 1;
                //vec3 points
                if (ImPlot::DragPoint(i+1, &nextPoint.x, &nextPoint.y, IMG_VIOLET, 4, PLOTTER_INPUTBLOCKCHECK))
                {
                    tweenRecord.tweenPoints[i] = nextPoint.getVec3();
                }
                ImPlot::Annotation(nextPoint.x, nextPoint.y, IMG_VIOLET, ImVec2(15, -15), false, "%s%i", "Point ", i);

                //timeline points
                if (ImPlot::DragPoint(i+1+tweenRecord.tweenPoints.size(), &xTime, &yTime, IMG_ORANGE, 4, PLOTTER_INPUTBLOCKCHECK))
                {
                    //float nowTimeStamp = std::accumulate(tweenRecord.tweenDelays.begin(), tweenRecord.tweenDelays.begin() + i, 0.f);

                    if (float delta = (static_cast<float>(xTime) - nowTimeStamp); delta > 0.f)
                        tweenRecord.tweenDelays[i] = delta;
                }
                ImPlot::Annotation(xTime, yTime, IMG_ORANGE, ImVec2(-10, -20), false, "%s%i", "Point ", i);
            }
            
            //tween vec3 values line
            int linePoints = static_cast<int>(std::floor(tweenDuration / IMG_PLOTTERSTIMESTEP));
            if (linePoints > 2 && tween)
            { 
                linePoints++;
                double xs[linePoints], ys[linePoints];
                nowTime = 0.f; //resetting, reusing the variable
                xs[0] = startPoint.x;
                ys[0] = startPoint.y;
                for (int i = 1; i < linePoints; ++i) 
                {
                    nowTime += IMG_PLOTTERSTIMESTEP;
                    if (nowTime > tweenDuration)
                        nowTime = tweenDuration;
                    vec3 nextValue = tween->getValue(nowTime);
                    xs[i] = static_cast<double>(nextValue.x);
                    ys[i] = static_cast<double>(nextValue.y);
                }

                ImPlot::PlotLine("Tween values line", xs, ys, linePoints);
            }
            
            //Timeline
            if (!tweenRecord.tweenDelays.empty())
            {
                double xTimes[tweenRecord.tweenDelays.size() + 1], yTimes[tweenRecord.tweenDelays.size() + 1];
                xTimes[0] = 0;
                yTimes[0] = 1;
                nowTime = 0.f;
                for (size_t i = 0; i < tweenRecord.tweenDelays.size(); i++)
                {
                    nowTime += tweenRecord.tweenDelays[i];
                    xTimes[i + 1] = nowTime;
                    yTimes[i + 1] = 1;
                }
                ImPlot::PlotLine("Tween timeline", xTimes, yTimes, tweenRecord.tweenDelays.size() + 1);
            }

            ImPlot::EndPlot();
        }
        ImGui::PopID();

        ImGui::Separator();
        ImGui::Spacing();
    }
#undef PLOTTER_INPUTBLOCKCHECK

    void plotterDockWindowWrapper()
    {
        if (!callSeparatePlotterWindow || !separatePlotterWindow_node || !needDrawPlotterWindow)
            return;

        if (ImGui::Begin("Tween plotter"))
        {
            switch (separatePlotterWindow_recordType)
            {
            case tweenTransformType::MOVE:
                ImGuiTweenPlotter(separatePlotterWindow_node, debugTweensMove.at(separatePlotterWindow_node));
                break;
            case tweenTransformType::SCALE:
                ImGuiTweenPlotter(separatePlotterWindow_node, debugTweensScale.at(separatePlotterWindow_node));
                break;
            case tweenTransformType::ROTATE:
                ImGuiTweenPlotter(separatePlotterWindow_node, debugTweensRotate.at(separatePlotterWindow_node));
                break;
            default:
                break;
            }

        }
        ImGui::End();
    }


    //reusable tween builder menu
    void ImGuiTweens(sptr<Node> _node)
    {
        if (!_node)
            return;

        //creating records if this is the first time opening the branch
        if (debugTweensMove.find(_node) == debugTweensMove.end())
        {
            debugTweensMove.insert({ _node, tweenDebugRecord{.transformType = tweenTransformType::MOVE} });
            debugTweensScale.insert({ _node, tweenDebugRecord{.transformType = tweenTransformType::SCALE} });
            debugTweensRotate.insert({ _node, tweenDebugRecord{.transformType = tweenTransformType::ROTATE} });
            debugTweensMove[_node].tweenStart = TweenContainer::nodeValueGetter(_node, debugTweensMove[_node]);
            debugTweensScale[_node].tweenStart = TweenContainer::nodeValueGetter(_node, debugTweensScale[_node]);
            debugTweensRotate[_node].tweenStart = TweenContainer::nodeValueGetter(_node, debugTweensRotate[_node]);
        }

        tweenDebugRecord& tweenRecordMove = debugTweensMove[_node];
        tweenDebugRecord& tweenRecordScale = debugTweensScale[_node];
        tweenDebugRecord& tweenRecordRotate = debugTweensRotate[_node];

        if (ImGui::BeginTabBar("Tween editor tabbar"))
        {
            //Movement tween branch
            if (ImGui::BeginTabItem("Move"))
            {
                ImGuiTweensBody(_node, tweenRecordMove);

                ImGui::EndTabItem();
            }
            //Scale tween branch
            if (_node != Render::getScreenCamera())
                if (ImGui::BeginTabItem("Scale"))
                {
                    ImGuiTweensBody(_node, tweenRecordScale);

                    ImGui::EndTabItem();
                }
            //Rotate tween branch
            if (ImGui::BeginTabItem("Rotate"))
            {
                ImGuiTweensBody(_node, tweenRecordRotate);

                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();

        //buttons for simultaneous launch/stop
        if (ImGui::Button("Play all tweens"))
        {

            if (!tweenRecordMove.isLoopedTween)
                tweenRecordMove.tween = TweenContainer::generateTween(tweenRecordMove, _node, nullptr, nullptr, true);
            else
                tweenRecordMove.tweenEndless = TweenContainer::generateEndlessTween(tweenRecordMove, _node, true);
                
            if (!tweenRecordScale.isLoopedTween)
                tweenRecordScale.tween = TweenContainer::generateTween(tweenRecordScale, _node, nullptr, nullptr, true);
            else
                tweenRecordScale.tweenEndless = TweenContainer::generateEndlessTween(tweenRecordScale, _node, true);
                
            if (!tweenRecordRotate.isLoopedTween)
                tweenRecordRotate.tween = TweenContainer::generateTween(tweenRecordRotate, _node, nullptr, nullptr, true);
            else
                tweenRecordRotate.tweenEndless = TweenContainer::generateEndlessTween(tweenRecordRotate, _node, true);
        }
        if (ImGui::Button("Stop all tweens"))
        {
            tweenRecordMove.tween = nullptr;
            tweenRecordMove.tweenEndless = nullptr;
            tweenRecordScale.tween = nullptr;
            tweenRecordScale.tweenEndless = nullptr;
            tweenRecordRotate.tween = nullptr;
            tweenRecordRotate.tweenEndless = nullptr;
        }
    }
}

#endif