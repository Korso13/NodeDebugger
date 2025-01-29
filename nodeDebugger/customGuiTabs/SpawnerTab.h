#pragma once

#include "../ND_iGuiTab.h"
#include <typeinfo>
#include <any>
#include "common/nodeDebugger/spawner/Spawnable.h"

//TODO list:
//- Add material current settings export to JSON (if possible)
//- Add constructor for shader materials and their settings export

class SpawnerFactory;
#define CLASS2STR(class_name) #class_name

#define ND_SPAWNER cast_sptr<SpawnerTab>(NODEDEBUG->getCustomTabByName("Spawner"))
#ifndef FORCENODEDEBUG_OFF 
	//Allows adding entities spawned outside of Spawner tab to the subtab of spawned entities (they must inherit Spawnable and implement its onGui and other methods!)
	//IMPORTANT! Always cast your object to sptr<Spawnable> using cast_sptr<Spawnable>() or you'll crash!!
	#define ND_SPAWNER_ADD cast_sptr<SpawnerTab>(NODEDEBUG->getCustomTabByName("Spawner"))->addItem
#else //tab doesn't exist in release!
	#define ND_SPAWNER_ADD(...)	
#endif // !FORCENODEDEBUG_OFF

class SpawnerTab : public iGuiTab, protected OnUpdateProvider
{
	friend Spawnable;
	friend SpawnerFactory;

public:
	
	SpawnerTab() : iGuiTab("Spawner") { m_currentMeshSpawnerSetting.className = "Mesh"; };

	void onGuiCall() override NOCOMP;
	bool isEmpty() override NOCOMP_B;

	//Unused right now. Consider depricated for this tab
	void addItem(std::any item, const std::string& altName = "default") override NOCOMP;
	
	void purgeValues() override NOCOMP;

protected:

	void onUpdate(float dt) override NOCOMP;

private:

	//onGui sub-calls
	void onGuiClassPalette() NOCOMP;
	void onGuiMeshPalette() NOCOMP;
	void onGuiSpawnedList() NOCOMP;
	void onGuiMapMeshData(MapMeshData& data) NOCOMP;


private:

	bool m_needUpdateEntitiesFilter = true; //todo: add usage!
	std::string m_entitiesFilter = "";
	std::set<std::string> m_lastEntitiesFilterList;

	MapMeshData m_currentMeshSpawnerSetting;

};