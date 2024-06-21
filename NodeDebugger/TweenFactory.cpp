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
        if (tweenSample.tweenName == tweenRecord.tweenName && tweenSample.transformType == tweenRecord.transformType)
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

void TweenContainer::deleteTweenSample(const std::string& tweenSampleName, tweenTransformType type)
{
    std::erase_if(tweenSettingsContainer, [tweenSampleName, type](tweenDebugRecord item) {
        return item.tweenName == tweenSampleName && item.transformType == type;
        });
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

vec3 TweenContainer::nodeValueGetter(sptr<Node> _node, const tweenDebugRecord& tweenRecord)
{
    vec3 returnValue;
    if (tweenRecord.transformType == tweenTransformType::MOVE)
    {
        if (tweenRecord.isWorldVec3)
            returnValue = _node->getWorldTranslation();
        else
            returnValue = _node->getLocalTranslation();
    }
    else if (tweenRecord.transformType == tweenTransformType::SCALE)
    {
        if (tweenRecord.isWorldVec3)
            returnValue = _node->getWorldScale();
        else
            returnValue = _node->getLocalScale();
    }
    else if (tweenRecord.transformType == tweenTransformType::ROTATE)
    {
        if (tweenRecord.isWorldVec3)
            returnValue = _node->getWorldRotation().euler() * RAD2DEG;
        else
            returnValue = _node->getLocalRotation().euler() * RAD2DEG;
    }

    return returnValue;
}

void TweenContainer::nodeValueSetter(vec3 value, sptr<Node> _node, const tweenDebugRecord& tweenRecord)
{
    if (tweenRecord.transformType == tweenTransformType::MOVE)
    {
        if (tweenRecord.isWorldVec3)
            _node->setWorldTranslation(value);
        else
            _node->setLocalTranslation(value);
    }
    else if (tweenRecord.transformType == tweenTransformType::SCALE)
    {
        if (tweenRecord.isWorldVec3)
            _node->setWorldScale(value);
        else
            _node->setLocalScale(value);
    }
    else if (tweenRecord.transformType == tweenTransformType::ROTATE)
    {
        if (!tweenRecord.isLoopedTween)
        {
            if (tweenRecord.isWorldVec3)
                _node->setWorldRotation(Rotator(value * DEG2RAD));
            else
                _node->setLocalRotation(Rotator(value * DEG2RAD));
        }
        else
        {
            if (tweenRecord.isWorldVec3)
                _node->setWorldRotation(Rotator(value));
            else
                _node->setLocalRotation(Rotator(value));
        }
        
    }
}
