#pragma once
#include "W4JSON.h"
#include "W4Framework.h"

template<typename Type>
struct SettingsContainer
{
public:
	void add(const nlohmann::json& data)
	{
		std::unordered_map<std::string, Type> tmpStore = data.get<std::unordered_map<std::string, Type>>();
		for (auto& element : tmpStore)
		{
			//			W4_LOG_DEBUG("[GAME SETTINGS] add element {%s}", element.first.c_str());
			m_dataStore.emplace(element.first, element.second);
		}
	}
	void init(const nlohmann::json& data)
	{
		m_dataStore = data.get<std::unordered_map<std::string, Type>>();
	}

	Type operator[](const std::string& key) const
	{
		auto it = m_dataStore.find(key);
		if (it == m_dataStore.end())
		{
			W4_LOG_ERROR("[RESOURCE ERROR] Unable to find resource key '%s'", key.c_str());
			return Type();
		}
		return it->second;
	}

	bool isExist(const std::string& key) const
	{
		auto it = m_dataStore.find(key);
		if (it == m_dataStore.end())
		{
			return false;
		}
		return true;
	}

	std::unordered_map<std::string, Type>& getDataStoreRef() { return m_dataStore; }

private:
	std::unordered_map<std::string, Type> m_dataStore;
};
