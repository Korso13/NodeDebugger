#include "TweenFactory.h"
#include "ND_JsonTools.h"

void TweenContainer::preloadTweenContainer()
{
    if (!tweenSettingsContainer.empty())
        return;

    tweenSettingsContainer = nlohmann::loadTweens().tweenRecordsData;
}

void TweenContainer::addNewTweenSample(tweenDebugRecord tweenRecord)
{
    //replace mechanic
    for (auto& tweenSample : tweenSettingsContainer)
    {
        if (tweenSample.tweenName == tweenRecord.tweenName)
        {
            tweenSample = tweenRecord;
            return;
        }
    }

    //in case it's a new sample - add to the list
    tweenSettingsContainer.push_back(tweenRecord);
}

void TweenContainer::exportTweenContainerToJson()
{
    if (tweenSettingsContainer.empty())
        return;

    tweenRecordsContainer tweenExportContainer;
    tweenExportContainer.tweenRecordsData = tweenSettingsContainer;

    nlohmann::exportTweens(tweenExportContainer);
}

const std::vector<tweenDebugRecord>& TweenContainer::getTweenContainer()
{
    return tweenSettingsContainer;
}

vec3 TweenContainer::getPercentModifyer(const vec3& startValue, const vec3& targetValue)
{
    vec3 result;
    if (startValue.x != 0.f)
        result.x = targetValue.x / startValue.x;
    else
        result.x = 1.f;
    if (startValue.y != 0.f)
        result.y = targetValue.y / startValue.y;
    else
        result.y = 1.f;
    if (startValue.z != 0.f)
        result.z = targetValue.z / startValue.z;
    else
        result.z = 1.f;
    return result;
}
