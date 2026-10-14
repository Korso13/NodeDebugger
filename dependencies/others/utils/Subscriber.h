#pragma once

#include <any>

#include "W4Framework.h"
#include "common/Defines.h"

W4_USE_UNSTRICT_INTERFACE


//You must control correctness of sending and reading payload yourself! No tools to determine if the payload has the data you need for now!!
//TODO: consider switching to optional
struct SubscriberEventPayload
{
	bool boolPayload = false;
	int intPayload = 0;
	float floatPayload = 0.f;
	std::string stringPayload = "";
	vec2 vec2Payload = vec2::zero;
	vec3 vec3Payload = vec3::zero;
	vec4 vec4Payload = vec4::zero;
	std::any anyPayload = nullptr; //use std::any_cast<>() to cast to required type
};

typedef std::function<void(std::string, SubscriberEventPayload)> SubscriberCallback;


class Subscriber
{
	struct SubscriberInfo
	{
		SubscriberCallback cb;
		bool bOneTimeEvent = false;
		bool bNeedsDeletion = false;
	};

public:

	Subscriber() = default;

	//subscribe to events triggered by this object. Returns subscribtionId for use in unsubscribeFromEvent()
	int subscribeToEvent(const std::string& eventName, SubscriberCallback onTriggerCallback, bool bUseOnce = false)
	{
		SubscriberInfo subscriptionInfo = {
			.cb = onTriggerCallback,
			.bOneTimeEvent = bUseOnce
		};

		subscribers[eventName].insert(
			{ 
				currentSubscriptionId, 
				subscriptionInfo
			}
		);
		return currentSubscriptionId++;
	}

	//unsubscribe from event using previously returned subscribtionId
	void unsubscribeFromEvent(const std::string& eventName, int subscribtionId)
	{
		if (!subscribers.contains(eventName) || !subscribers[eventName].contains(subscribtionId))
			return;

		subscribers[eventName].erase(subscribtionId);
	}

	//use it, if you need to make sure no shared pointers were caught by this object via event subscriptions!
	void flushSubscriptions() { subscribers.clear(); };

	//Notify subscribers of eventName and optionally pass payload
	void triggerEvent(const std::string& eventName, SubscriberEventPayload payload = SubscriberEventPayload())
	{
		if (!subscribers.contains(eventName))
			return;
	
		for (auto& [subscriberId, subInfo] : subscribers[eventName])
		{
			if (subInfo.cb)
			{
				SubscriberCallback tempHolder = subInfo.cb;
				tempHolder(eventName, payload);
				if (subInfo.bOneTimeEvent)
					subInfo.bNeedsDeletion = true;
			}
		}
		std::erase_if(subscribers[eventName],
			[this](auto& pair) {return pair.second.bNeedsDeletion; });
	};
protected:


private:

	int currentSubscriptionId = 0;
	std::unordered_map<std::string, std::map<int, SubscriberInfo>> subscribers;
};