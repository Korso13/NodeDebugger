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
	
	SpawnerTab();

	void onGuiCall() override NOCOMP;
	bool isEmpty() override NOCOMP_B;

	//Unused right now. Consider depricated for this tab
	void addItem(std::any item, const std::string& altName = "default") override NOCOMP;
	
	void purgeValues() override NOCOMP;
	void markActive(bool isActive) override ;

	void initClickColliders() NOCOMP;

protected:

	void onUpdate(float dt) override NOCOMP;

private:

	//onGui sub-calls
	void onGuiClassPalette() NOCOMP;
	void onGuiMeshPalette() NOCOMP;
	void onGuiSpawnedList() NOCOMP;
	void onGuiSpawnedMeshesList() NOCOMP;
	void onGuiMapMeshData(MapMeshData& data) NOCOMP;
	void onGuiExportBtn() NOCOMP;

	void onKeyPressed(const event::Keyboard::Down& evt) NOCOMP;
	void onKeyReleased(const event::Keyboard::Up& evt) NOCOMP;
	void onTouchBegin(const Touch::Begin& evt) NOCOMP;
	void setupClickCollider(const std::string& name, sptr<Spawnable> spawnedObj) NOCOMP; //for Spawnables
	void setupClickCollider(const std::string& name, sptr<Mesh> spawnedMesh) NOCOMP; //for meshes

private:

	bool m_needUpdateEntitiesFilter = true; 
	bool m_clickAddModeON = false;
	bool m_clickFiltering = false;
	std::string m_entitiesFilter = "";
	std::string m_meshesFilter = "";
	std::string m_lastSelectedSpawnable = "";
	std::string m_lastSelectedMesh = "";
	std::set<std::string> m_lastSpawnablesFilterList;
	std::set<std::string> m_lastMeshesFilterList;

	enum Subtab
	{
		ClassPalette,
		MeshPalette,
		SpawnedObjects,
		SpawnedMeshes
	} m_activeSubtab = Subtab::ClassPalette;

	MapMeshData m_currentMeshSpawnerSetting;
};