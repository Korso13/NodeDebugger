#pragma once
#include "W4Framework.h"
#include "./InstanceInitiation.h"
#include "./SettingsContainer.h"
#include "../common/Defines.h"

W4_USE_UNSTRICT_INTERFACE

#define ND_ACCESSENABLED

class Settings : public InstanceInitiation
{

public:
	static Settings& instance()
	{
		static Settings instance;
		return instance;
	}

	Settings();
	virtual ~Settings();

	virtual void loadConfig() override;

	bool getBool(const std::string& key)
	{
		initCheck();
		return m_boolData[key];
	}
	int getInt(const std::string& key)
	{
		initCheck();
		return m_intData[key];
	}
	float getFloat(const std::string& key)
	{
		initCheck();
		return m_floatData[key];
	}
	std::string getString(const std::string& key)
	{
		initCheck();
		return m_stringData[key];
	}
	vec2 getVec2(const std::string& key)
	{
		initCheck();
		return m_vec2Data[key];
	}
	vec3 getVec3(const std::string& key)
	{
		initCheck();
		return m_vec3Data[key];
	}
	vec4 getVec4(const std::string& key)
	{
		initCheck();
		return m_vec4Data[key];
	}

	bool getBoolForsed(const std::string& key, bool defValue = false);
	int getIntForsed(const std::string& key, int defValue = 1);
	float getFloatForsed(const std::string& key, float defValue = 1.f);
	std::string getStringForsed(const std::string& key, std::string defValue = "");
	vec2 getVec2Forsed(const std::string& key, vec2 defValue = vec2::one);
	vec3 getVec3Forsed(const std::string& key, vec3 defValue = vec3::one);
	vec4 getVec4Forsed(const std::string& key, vec4 defValue = vec4::one);

	void setPlayableName(const std::string& playable) { m_playable = playable; }
	void setNetworkName(const std::string& network) { m_network = network; }

	const std::string& getPlayableName() { return m_playable; }
	const std::string& getNetworkName() { return m_network; }

	template<typename T>
	std::unordered_map<std::string, T>& getContainerRef();

private:
	SettingsContainer<bool> m_boolData;
	SettingsContainer<int> m_intData;
	SettingsContainer<float> m_floatData;
	SettingsContainer<std::string> m_stringData;
	SettingsContainer<vec2> m_vec2Data;
	SettingsContainer<vec3> m_vec3Data;
	SettingsContainer<vec4> m_vec4Data;

	bool m_isInit = false;
	std::string m_fileExtension = ".json";
	std::string m_configName = "settings";

	std::string m_playable = "none";
	std::string m_network = "none";
};

	template<>
	inline std::unordered_map<std::string, bool>& Settings::getContainerRef<bool>()
	{
		initCheck();
		return m_boolData.getDataStoreRef();
	};
	template<>
	inline std::unordered_map<std::string, int>& Settings::getContainerRef<int>()
	{
		initCheck();
		return m_intData.getDataStoreRef();
	};
	template<>
	inline std::unordered_map<std::string, float>& Settings::getContainerRef<float>()
	{
		initCheck();
		return m_floatData.getDataStoreRef();
	};
	template<>
	inline std::unordered_map<std::string, std::string>& Settings::getContainerRef<std::string>()
	{
		initCheck();
		return m_stringData.getDataStoreRef();
	};
	template<>
	inline std::unordered_map<std::string, vec2>& Settings::getContainerRef<vec2>()
	{
		initCheck();
		return m_vec2Data.getDataStoreRef();
	};
	template<>
	inline std::unordered_map<std::string, vec3>& Settings::getContainerRef<vec3>()
	{
		initCheck();
		return m_vec3Data.getDataStoreRef();
	};
	template<>
	inline std::unordered_map<std::string, vec4>& Settings::getContainerRef<vec4>()
	{
		initCheck();
		return m_vec4Data.getDataStoreRef();
	};