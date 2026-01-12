#pragma once

#include "W4Framework.h"
#include "ND_Defines.h"
#include "common/ParabolaFactory.h"

namespace TweenContainer
{
	//loads tween settins container
    void preloadTweenContainer();

    //NodeDebugger tween tool methods:
	void addNewTweenSample(tweenDebugRecord tweenRecord);
	void exportTweenContainerToJson();
	const std::vector<tweenDebugRecord>& getTweenContainer();
    void deleteTweenSample(const std::string& tweenSampleName, tweenTransformType type);
    vec3 getPercentModifyer(const vec3& startValue, const vec3& targetValue);

    //Pulls nessesary node transforms (translate, rotate, scale; world/local) based on tweenRecord
    vec3 nodeValueGetter(sptr<Node> _node, const tweenDebugRecord& tweenRecord);
    //Sets nessesary node transforms (translate, rotate, scale; world/local) based on tweenRecord
    void nodeValueSetter(vec3 value, sptr<Node> _node, const tweenDebugRecord& tweenRecord);

    //Non-looped tween generator
	static sptr<CustomTweens::Tween<vec3>> generateTween(const tweenDebugRecord& tweenRecord, sptr<Node> _node, std::function<void(vec3&)>&& changeValueOnUpdateCB = nullptr, std::function<void()>&& onFinishCB = nullptr, bool bSetNodeToStartPoint = false)
	{
        if (tweenRecord.tweenDelays.empty() || tweenRecord.tweenPoints.empty())
            return nullptr;

        vec3 offset, tweenStart;

        if (!bSetNodeToStartPoint || tweenRecord.isRelativeValues) //default behavior
        {
            tweenStart = nodeValueGetter(_node, tweenRecord);
        }
        else //used by ImGui TweenEditor
        {
            tweenStart = tweenRecord.tweenStart;
            nodeValueSetter(tweenStart, _node, tweenRecord);
        }

        if (!tweenRecord.isRelativeValues || (tweenRecord.isRelativeValues && tweenRecord.isPercentRelativeValues) )
            offset = vec3::zero;
        else
            offset = tweenStart - tweenRecord.tweenStart;

        //for curves
        using curveFuncsHandler = std::map<int, std::pair<bool, std::function<float(float)>>>; //key - point id, pair - is curveOn and curve function itself 
        sptr<curveFuncsHandler> curveFuncs = make::sptr<curveFuncsHandler>(); //use sptr to share container between tween callbacks! Yep, it's that fucked up! -_|*_*|_-
        if(tweenRecord.transformType == tweenTransformType::MOVE && curveFuncs)
        {
            for (int i = 0; i < tweenRecord.tweenPoints.size(); i++)
            {
                if (!tweenRecord.curveApogeePoints.contains(i) || !tweenRecord.curveApogeePoints.at(i).first)
                    continue;

                vec3 end = tweenRecord.tweenPoints[i];
                vec3 begin = (i == 0) ? tweenRecord.tweenStart : tweenRecord.tweenPoints[i - 1];
                auto curve = getParabolicFunction(begin, end, tweenRecord.curveApogeePoints.at(i).second);
                if (i == 0)
                    curveFuncs->insert({ 0, { true, curve } });
                else
                    curveFuncs->insert({ i - 1, { true, curve } });

            }
        }
        //============

        auto tween = make::sptr<CustomTweens::Tween<vec3>>(
            tweenStart,
            [tweenRecord, _node, changeValueOnUpdateCB, curveFuncs](vec3 newValue)
            {
                if (!_node)
                    return;

                //applying curve functions (if any). curveFunc.first - bool indicating if curve func is active (activated in point callbacks)
                if (curveFuncs)
                {                
                    for (auto& [point, curveFunc] : *curveFuncs)
                        if (curveFunc.first && curveFunc.second)
                            newValue.y = curveFunc.second(newValue.x);
                }

                if (changeValueOnUpdateCB)
                    changeValueOnUpdateCB(newValue);

                //setting final transform value to the node
                nodeValueSetter(newValue, _node, tweenRecord);
            },
            tweenRecord.tweenName
        );

        for (size_t i = 0; i < tweenRecord.tweenPoints.size(); i++)
        {
            vec3 nextValue = vec3::zero;
            if (tweenRecord.isRelativeValues && tweenRecord.isPercentRelativeValues)
            {
                nextValue = tweenStart * getPercentModifyer(tweenRecord.tweenStart, tweenRecord.tweenPoints[i]);
            }
            else
            {
                nextValue = tweenRecord.tweenPoints[i] + offset;
            }
            tween->addMore(
                nextValue,
                tweenRecord.tweenDelays[i],
                tweenRecord.easingTypes[i],
                (i + 1 == tweenRecord.tweenPoints.size())
                ? onFinishCB //last point's callback
                : //other points' callbacks
                [curveFuncs, i]() 
                {
                    //toggles on/off curveFuncs based on passed tween point
                    if (!curveFuncs)
                        return;
                    for (auto& [point, curveFunc] : *curveFuncs)
                        curveFunc.first = point == i;
                }
            );
        }

        return tween;
	}

    //Looped tween generator
    static sptr<CustomTweens::EndlessTween<vec3>> generateEndlessTween(const tweenDebugRecord& tweenRecord, sptr<Node> _node, std::function<void(vec3)> onUpdateCb = nullptr, bool bSetNodeToStartPoint = false)
    {
        if (tweenRecord.tweenDelays.empty() || tweenRecord.tweenPoints.empty())
            return nullptr;

        vec3 offset, tweenStart;

        if (!bSetNodeToStartPoint || tweenRecord.isRelativeValues) //default behavior
        {
            tweenStart = nodeValueGetter(_node, tweenRecord);
        }
        else //used by ImGui TweenEditor
        {
            tweenStart = tweenRecord.tweenStart;
            if (tweenRecord.transformType == tweenTransformType::ROTATE)
                tweenStart *= DEG2RAD; //hardcoded to degree, because no one is using radians in NodeDebugger
            nodeValueSetter(tweenStart, _node, tweenRecord);
        }

        if (!tweenRecord.isRelativeValues || (tweenRecord.isRelativeValues && tweenRecord.isPercentRelativeValues))
            offset = vec3::zero;
        else
            offset = tweenStart - tweenRecord.tweenStart;

        sptr<CustomTweens::EndlessTween<vec3>> tween = make::sptr<CustomTweens::EndlessTween<vec3>>(tweenStart, onUpdateCb, tweenRecord.tweenName);
        float nowTime = 0.f;
        for (size_t i = 0; i < tweenRecord.tweenPoints.size(); i++)
        {
            vec3 nextValue = vec3::zero;
            if (tweenRecord.isRelativeValues && tweenRecord.isPercentRelativeValues)
            {
                nextValue = tweenStart * getPercentModifyer(tweenRecord.tweenStart, tweenRecord.tweenPoints[i]);
            }
            else
            {
                nextValue = tweenRecord.tweenPoints[i] + offset;
            }
            if (tweenRecord.transformType == tweenTransformType::ROTATE)
                nextValue *= DEG2RAD;
            nowTime += tweenRecord.tweenDelays[i];
            tween->add(nextValue, nowTime, tweenRecord.easingTypes[i]);
        }

        return tween;
    }
    //Legacy compatibility call
    static sptr<CustomTweens::EndlessTween<vec3>> generateEndlessTween(const tweenDebugRecord& tweenRecord, sptr<Node> _node, bool bSetNodeToStartPoint = false)
    {
        return generateEndlessTween(tweenRecord, _node, nullptr, bSetNodeToStartPoint);
    }

    /*
    Non - looped tween getter from JSON
    tweenSetting - name of the record from tweens.json;
    node - node/mesh/spine affected by the tween
    changeValueOnUpdateCB - lambda that alters generated tween values on update (e.g. parabola function to add curve to the tween)
    onFinishCB - lambda to be called at the tween's end
    useTweenSettingStartPoint - if true will set relevant transform to startPoint written in the tween sample
    transformTypeSpecifyier - use it in case you have 2+ tweens with same names but different transforms (can leave default(MOVE) if all Samples have unique names)
    */
    static sptr<CustomTweens::Tween<vec3>> getTween(
        const std::string& tweenSetting, 
        sptr<Node> node, 
        std::function<void(vec3&)>&& changeValueOnUpdateCB = nullptr, 
        std::function<void()>&& onFinishCB = nullptr,
        bool useTweenSettingStartPoint = false,
        tweenTransformType transformTypeSpecifyier = tweenTransformType::MOVE)
    {
        if(getTweenContainer().empty())
            preloadTweenContainer();
        
        tweenDebugRecord defaultSearchResult;
        for (auto& tweenRecord : getTweenContainer())
        {
            if (tweenRecord.tweenName == tweenSetting && tweenRecord.transformType == transformTypeSpecifyier)
            {
                return generateTween(tweenRecord, node, std::move(changeValueOnUpdateCB), std::move(onFinishCB), useTweenSettingStartPoint);
            }
            //in case transformType was not specified
            else if (tweenRecord.tweenName == tweenSetting && transformTypeSpecifyier == tweenTransformType::MOVE)
            {
                defaultSearchResult = tweenRecord;
            }
        }

        //in case transformType was not specified
        if (!defaultSearchResult.tweenName.empty())
        {
            return generateTween(defaultSearchResult, node, std::move(changeValueOnUpdateCB), std::move(onFinishCB), useTweenSettingStartPoint);
        }

        warn("Can't find %s tween setting! Returning null!", tweenSetting.c_str());
        return nullptr;
    }

    /*
    Looped tween getter from JSON
    tweenSetting - name of the record from tweens.json;
    node - node / mesh / spine affected by the tween
    useTweenSettingStartPoint - if true will set relevant transform to startPoint written in the tween sample
    transformTypeSpecifyier - use it in case you have 2+ tweens with same names but different transforms (can leave default(MOVE) if all Samples have unique names)
    */
    static sptr<CustomTweens::EndlessTween<vec3>> getLoopedTween(const std::string& tweenSetting, sptr<Node> node, bool useTweenSettingStartPoint = false, std::function<void(vec3)> onUpdateCb = nullptr, tweenTransformType transformTypeSpecifyier = tweenTransformType::MOVE)
    {
        if (getTweenContainer().empty())
            preloadTweenContainer();

        tweenDebugRecord defaultSearchResult;
        for (auto& tweenRecord : getTweenContainer())
        {
            if (tweenRecord.tweenName == tweenSetting && tweenRecord.transformType == transformTypeSpecifyier)
            {
                return generateEndlessTween(tweenRecord, node, onUpdateCb, useTweenSettingStartPoint);
            }
            //in case transformType was not specified
            else if (tweenRecord.tweenName == tweenSetting && transformTypeSpecifyier == tweenTransformType::MOVE)
            {
                defaultSearchResult = tweenRecord;
            }
        }

        //in case transformType was not specified
        if (!defaultSearchResult.tweenName.empty())
        {
            return generateEndlessTween(defaultSearchResult, node, onUpdateCb, useTweenSettingStartPoint);
        }

        warn("Can't find %s tween setting! Returning null!", tweenSetting.c_str());
        return nullptr;
    }

    //Tween settings container
	static std::vector<tweenDebugRecord> tweenSettingsContainer;
}