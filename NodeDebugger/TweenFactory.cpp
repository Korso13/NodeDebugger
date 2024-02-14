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