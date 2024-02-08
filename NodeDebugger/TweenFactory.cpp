#include "TweenFactory.h"
#include "ND_JsonTools.h"

void CustomTweens::preloadTweenContainer()
{
    if (!tweenSettingsContainer.empty())
        return;

    tweenSettingsContainer = nlohmann::loadTweens().tweenRecordsData;
}

void CustomTweens::addNewTweenSample(tweenDebugRecord tweenRecord)
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

void CustomTweens::exportTweenContainerToJson()
{
    if (tweenSettingsContainer.empty())
        return;

    tweenRecordsContainer tweenExportContainer;
    tweenExportContainer.tweenRecordsData = tweenSettingsContainer;

    nlohmann::exportTweens(tweenExportContainer);
}

const std::vector<tweenDebugRecord>& CustomTweens::getTweenContainer()
{
    return tweenSettingsContainer;
}