#pragma once
#include "SpawnerFactory.h"

template<typename T> requires(std::is_base_of_v<Spawnable, T>)
sptr<T> SpawnerFactory::getObjectByName(const std::string& spawnableName)
{
	if (m_spawnedEntities.contains(spawnableName))
		return cast_sptr<T>(m_spawnedEntities[spawnableName]);
	else
	{
		err("Failed to find Spawnable %s", spawnableName.c_str());
		return nullptr;
	}
};

template<typename T> requires(std::is_base_of_v<Spawnable, T>)
std::vector<sptr<T>> SpawnerFactory::getAllObjectsOfClass(const std::string className)
{
	std::vector<sptr<T>> found;
	for (auto& [name, Spawned] : m_spawnedEntities)
		if (Spawned && Spawned->getMapObjectsData().className == className)
			found.push_back(cast_sptr<T>(Spawned));
	return found;
}

//SpawnerRecord's addConstructorParam() specialization for sptr<Node>, which utilizes ImGuiNodeHierarchy for picking the node
template<>
inline SpawnerFactory::SpawnerRecord& SpawnerFactory::SpawnerRecord::addConstructorParam<sptr<Node>>(const std::string& paramName, sptr<Node> defaultNode)
{
	ParamPack newParam;
	newParam.paramName = paramName;
	newParam.paramValue.setValue<sptr<Node>>(defaultNode);
	newParam.node = defaultNode;
	constructorParams.push_back(newParam);
	int paramIndex = constructorParams.size() - 1;

#ifndef FORCENODEDEBUG_OFF
	m_argsOnGui.push_back([this, paramIndex, paramName]()
		{
			if (!constructorParams[paramIndex].node.expired())
			{
				auto nodeHolder = constructorParams[paramIndex].node.lock();
				if (ND_Util::ImGuiNodeHierarchy(nodeHolder, paramName))
				{
					setNewConstructNode(nodeHolder);
				}
			}
		});
#endif
	return *this;
}

//SpawnerRecord's addConstructorParam() specialization for MapObjectsData
template<>
inline SpawnerFactory::SpawnerRecord& SpawnerFactory::SpawnerRecord::addConstructorParam<MapObjectsData&>(const std::string& paramName, MapObjectsData& defaultMOD)
{
	m_defaultMod = defaultMOD;
	ParamPack newParam;
	newParam.paramName = paramName;
	newParam.paramValue.setValue<MapObjectsData&>(m_defaultMod);
	newParam.isMapData = true;
	constructorParams.push_back(newParam);
	int paramIndex = constructorParams.size() - 1;

#ifndef FORCENODEDEBUG_OFF
	m_argsOnGui.push_back([this, paramIndex, paramName]()
		{
			NDGui::Auto(paramName.c_str(), m_defaultMod);
		});
#endif
	return *this;
}

template<typename DataType>
requires AutoTypeConcept<DataType> || onGuiImplConcept<DataType>
inline SpawnerFactory::SpawnerRecord& SpawnerFactory::SpawnerRecord::exposeVariableToPaletteUi(const std::string& variableName, DataType* variable)
{
#ifndef FORCENODEDEBUG_OFF
	m_argsOnGui.emplace_back(
		[this, variableName, variable]()
		{
			if (!variable)
			{
				info("[SpawnerFactory::SpawnerRecord::exposeVariableToPaletteUi][onGui] invalid pointer to variable %s for class %s!", variableName.c_str(), m_classID.c_str());
				return;
			}
			NDGui::Auto(variableName.c_str(), *variable);
		});
#endif

	return *this;
}