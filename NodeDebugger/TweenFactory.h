#pragma once

#include "W4Framework.h"
#include "ND_Defines.h"

namespace TweenContainer
{
	//loads tween settins container
    void preloadTweenContainer();

    //NodeDebugger tween tool methods:
	void addNewTweenSample(tweenDebugRecord tweenRecord);
	void exportTweenContainerToJson();
	const std::vector<tweenDebugRecord>& getTweenContainer();
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
            offset = tweenStart-tweenRecord.tweenStart;

        auto tween = make::sptr<CustomTweens::Tween<vec3>>(
            tweenStart,
            [tweenRecord, _node, changeValueOnUpdateCB](vec3 newValue)
            {
                if (!_node)
                    return;

                if (changeValueOnUpdateCB)
                    changeValueOnUpdateCB(newValue);

                nodeValueSetter(newValue, _node, tweenRecord);
            });

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
                ? onFinishCB
                : []() {}
            );
        }

        return tween;
	}

    //Looped tween generator
    static sptr<CustomTweens::EndlessTween<vec3>> generateEndlessTween(const tweenDebugRecord& tweenRecord, sptr<Node> _node, bool bSetNodeToStartPoint = false)
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

        if (!tweenRecord.isRelativeValues || (tweenRecord.isRelativeValues && tweenRecord.isPercentRelativeValues))
            offset = vec3::zero;
        else
            offset = tweenStart - tweenRecord.tweenStart;

        sptr<CustomTweens::EndlessTween<vec3>> tween = make::sptr<CustomTweens::EndlessTween<vec3>>(tweenStart, "new_tween");
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
            nowTime += tweenRecord.tweenDelays[i];
            tween->add(nextValue, nowTime, tweenRecord.easingTypes[i]);
        }

        return tween;
    }

    /*
    Non - looped tween getter from JSON
    tweenSetting - name of the record from tweens.json;
    node - node/mesh/spine affected by the tween
    changeValueOnUpdateCB - lambda that alters generated tween values on update (e.g. parabola function to add curve to the tween)
    onFinishCB - lambda to be called at the tween's end
    */
    static sptr<CustomTweens::Tween<vec3>> getTween(
        const std::string& tweenSetting, 
        sptr<Node> node, 
        std::function<void(vec3&)>&& changeValueOnUpdateCB = nullptr, 
        std::function<void()>&& onFinishCB = nullptr)
    {
        if(getTweenContainer().empty())
            preloadTweenContainer();
        
        for (auto& tweenRecord : getTweenContainer())
        {
            if (tweenRecord.tweenName == tweenSetting)
            {
                return generateTween(tweenRecord, node, std::move(changeValueOnUpdateCB), std::move(onFinishCB));
            }
        }

        warn("Can't find %s tween setting! Returning null!", tweenSetting.c_str());
        return nullptr;
    }

    /*
    Looped tween getter from JSON
    tweenSetting - name of the record from tweens.json;
    node - node / mesh / spine affected by the tween
    */
    static sptr<CustomTweens::EndlessTween<vec3>> getLoopedTween(const std::string& tweenSetting, sptr<Node> node)
    {
        if (getTweenContainer().empty())
            preloadTweenContainer();

        for (auto& tweenRecord : getTweenContainer())
        {
            if (tweenRecord.tweenName == tweenSetting)
            {
                return generateEndlessTween(tweenRecord, node);
            }
        }

        warn("Can't find %s tween setting! Returning null!", tweenSetting.c_str());
        return nullptr;
    }

    //Tween settings container
	static std::vector<tweenDebugRecord> tweenSettingsContainer;
}