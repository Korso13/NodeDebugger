#pragma once

#include "W4Framework.h"
#include "common/Defines.h"
#include <any>
#include "SpawnerStructs.h"

W4_USE_UNSTRICT_INTERFACE

//forward decl
class SpawnerTab;

//struct MapObjectsData is defined in "common/nodeDebugger/SpawnerStructs.h"! via MOD_DEFINITION macro

class Spawnable
{
	friend SpawnerTab;

protected:
	//forward decl
	struct SpawnSetting;

private:

	//helper struct
	struct anyVar
	{
		friend SpawnSetting;

		template<typename T>
		void setValue(T val);

		template<typename T>
		void setValueByRef(T& val);

		template<typename T>
		T get() { return std::any_cast<T>(m_value); };

		template<typename T>
		T& getRef() { return std::any_cast<T&>(m_value); };

		bool hasValue() const { return m_value.has_value(); }

	private:
		std::any m_value;
		std::function<bool(const std::string&, anyVar&)> onGuiCb = nullptr;
		std::function<void(anyVar&)> refCb = nullptr;
	};

protected:

	//Struct containing information about exposed setting, callback for changing it and ImGui render for it
	struct SpawnSetting
	{
		SpawnSetting() = delete;

		//pass value by reference (will be updated automatically, no need to change in callback)
		template<typename T>
		explicit SpawnSetting(const std::string& name, T& value);
		//pass value by reference (will be updated automatically, no need to change in callback)
		template<typename T>
		explicit SpawnSetting(const std::string& name, T& value, std::function<void(std::decay_t<T>&)>&& onChangeCB);
		//pass value as copy
		template<typename T>
		explicit SpawnSetting(const std::string& name, T&& value, std::function<void(std::decay_t<T>&)>&& onChangeCB);

		//Factory method(use it in case you have issues with templated constructor)
		template<typename T>
		static SpawnSetting makeSpawnSetting(const std::string& name, std::decay_t<T> value, std::function<void(T&)>&& onChangeCB);

		std::string settingName = "setting_default_name";
		anyVar settingValue;
		std::function<void(anyVar)> onSettingChangeCB;

		std::function<bool(const std::string&, anyVar&)>& getOnGuiCb() { return settingValue.onGuiCb; }
		
	private:

		SpawnSetting(const std::string& name) : settingName(name) {};
	};

	//Spawnable interface
public:

	Spawnable() = delete;
	Spawnable(MapObjectsData& spawnData) : m_mapObjectData(spawnData) {};
	virtual ~Spawnable() { m_spRoot = nullptr; }

	virtual sptr<Node> getSpawnableRoot() { return m_spRoot; };

	//overload in main class to call its respective update method (allows spawned entities to recieve updates)
	//Use only if your are not setting up updates for Spawnable via 
	virtual void spawnableSelfUpdate(float dt) { };

	//Used by Spawner Tab. Do not call youreself!
	virtual bool onGui(const char* label);

	//used to get data for exporting spawned entities
	virtual MapObjectsData& getMapObjectsData() { return m_mapObjectData; }

protected:

	//call in your class' constructor and pass its root node (it will be copied and used for initial placement)
	//You need to override it if you want to expose variables of your spawned objects to SpawnerTab
	//If you override it, always call Spawnable::initSpawnable(). Use addSpawnableSetting to expose your class fields to the SpawnerTab
	virtual void initSpawnable(sptr<Node> rootNode);

	//Exposes class variable to the Spawner tab (Enums must be first serialized via AUTO_SERIALIZE_ENUM or W4_NLOHMANN_JSON_SERIALIZE_ENUM macro!). 
	//Use SpawnSetting::makeSpawnSetting to generate a proper SpawnSetting!!
	//makeSpawnSetting arguements: 1) Variable name 2) Variable ref or value 3) Callback on value change
	void addSpawnableSetting(SpawnSetting&& newSetting)
	{
		m_spawnableSettings.insert({ newSetting.settingName, std::move(newSetting)});
	}	
	void removeSpawnableSetting(const std::string& settingName)
	{
		m_spawnableSettings.erase(settingName);
	}

	//Exposes method of the spawned object to its spawner tab
	void addSpawnableBtn(const std::string& btnName, std::function<void()>&& btnCall)
	{
		m_spawnableBtns[btnName] = std::move(btnCall);
	}
	void removeSpawnableBtn(const std::string& btnName)
	{
		m_spawnableBtns.erase(btnName);
	}

public:

	bool isSpawned = false; //set to true if the object was spawned via Spawner Tab

protected:

	MapObjectsData m_mapObjectData;
	
	bool m_isChildObj = false;
	sptr<Node> m_spRoot = nullptr; //Spawnable object's root (used in OnGui())

private:

	std::map<std::string, SpawnSetting> m_spawnableSettings; //Fields exposed to the Spawner Tab Gui
	std::map<std::string, std::function<void()>> m_spawnableBtns; //Btns exposed to the Spawner Tab Gui
	size_t m_rootNodeTransformSubsHandler = -1;
};

#include "SpawnableImpl.inl"