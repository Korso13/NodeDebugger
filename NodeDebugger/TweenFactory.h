#pragma once

#include "W4Framework.h"
#include "ND_Defines.h"

namespace CustomTweens
{
	//loads tween settins container
    void preloadTweenContainer();

    //NodeDebugger tween tool methods:
	void addNewTweenSample(tweenDebugRecord tweenRecord);
	void exportTweenContainerToJson();
	const std::vector<tweenDebugRecord>& getTweenContainer();

    //Non-looped tween generator
	static sptr<Tween<vec3>> generateTween(const tweenDebugRecord& tweenRecord, sptr<Node> _node, std::function<void()>&& onFinishCB = nullptr)
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

                (i + 1 == tweenRecord.tweenPoints.size())
                ? onFinishCB
                : []() {}
            );
        }

        return tween;
	}

    //Looped tween generator
    static sptr<EndlessTween<vec3>> generateEndlessTween(const tweenDebugRecord& tweenRecord, sptr<Node> _node)
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

    //Non-looped tween getter from JSON
    static sptr<Tween<vec3>> getTween(const std::string& tweenSetting, sptr<Node> node, std::function<void()>&& onFinishCB = nullptr)
    {
        if(getTweenContainer().empty())
            preloadTweenContainer();
        
        for (auto& tweenRecord : getTweenContainer())
        {
            if (tweenRecord.tweenName == tweenSetting)
            {
                return generateTween(tweenRecord, node, std::move(onFinishCB));
            }
        }

        warn("Can't find %s tween setting! Returning null!", tweenSetting.c_str());
        return nullptr;
    }

    //Looped tween getter from JSON
    static sptr<EndlessTween<vec3>> getLoopedTween(const std::string& tweenSetting, sptr<Node> node)
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