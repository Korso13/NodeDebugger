#pragma once
#include "Spawnable.h"
#include "../Concepts.h"
#include "../CustomAutos.h"

template<typename T>
bool operator==(const wptr<T>& lhw, const wptr<T>& rhw)
{
	if (lhw.expired() && rhw.expired()) return true;
	return lhw.lock() == rhw.lock();
}

template<typename T>
bool operator==(wptr<T>& lhw, wptr<T>& rhw)
{
	if (lhw.expired() && rhw.expired()) return true;
	return lhw.lock() == rhw.lock();
}

//Template methods impls. setValue is used when adding Spawnable Settings for rendering in Spawned Objects submenu
template<typename T>
inline void Spawnable::SpawnSetting::anyVar::setValue(T* val)
{
	m_value = val;
#ifdef DEBUG
	onGuiCb =
		[this](const std::string& name, anyVar& var) -> bool
		{
			if (!var.hasValue())
				return false;
			return NDGui::Auto(name.c_str(), *var.getRef<T>());
		};
#endif
	m_isInited = true;
}

template<typename T>
inline void Spawnable::SpawnSetting::anyVar::setValueByCopy(T val)
{
	m_isCopyValue = true;
	m_copyValue = val;
#ifdef DEBUG
	onGuiCb =
		[this](const std::string& name, anyVar& var) -> bool
		{
			if (!var.hasValue())
				return false;
			return NDGui::Auto(name.c_str(), *var.getRef<T>());
		};
#endif
	m_isInited = true;
}

inline bool Spawnable::SpawnSetting::anyVar::hasValue() const
{
	if (!m_isInited) 
		return false; 
	else if (!m_isCopyValue) 
		return m_value != nullptr; 
	else 
		return m_copyValue.has_value();
}

template<typename T>
inline T Spawnable::SpawnSetting::anyVar::get()
{
	if (!m_isCopyValue)
	{
		return static_cast<std::decay_t<T>>(m_value);
	}
	else
	{
		return std::any_cast<T>(m_copyValue);
	}
}

template<typename T>
inline T* Spawnable::SpawnSetting::anyVar::getRef()
{
	if (!m_isCopyValue)
		return static_cast<std::decay_t<T>*>(m_value);
	else
		return &std::any_cast<T&>(m_copyValue);
}

//specialisations for sptr<Node> because the engine one is shit
template<>
inline void Spawnable::SpawnSetting::anyVar::setValue(sptr<Node>* val)
{
	m_value = val;
	wptr<Node> nodeW = *val;

#ifdef DEBUG
	onGuiCb =
		[this, nodeW](const std::string& name, anyVar& var) -> bool
		{
			if (nodeW.expired())
				return false;

			bool changed = false;
			if (ImGui::TreeNode(name.c_str()))
			{
				sptr<Node> node = nodeW.lock();

				Transform oldTransform = node->getWorldTransform();
				if (node->is<VisibleNode>() || node->is<Mesh>() || node->is<ParticlesEmitter>())
				{
					ND_Util::ImGuiNodeRender(node, name);
				}
				else
				{
					ND_Util::NoConflictGizmo(node, name);
					changed = ND_Util::ImGuiTransforms(node);
				}

				//submenu for reparenting node
				if (ImGui::TreeNode("Parent node selector"))
				{
					auto parentNode = node->getParent();
					if (ND_Util::ImGuiNodeHierarchy(parentNode, "Parent Node")) //returns true if new node was selected
					{
						node->setParent(parentNode);
					}

					ImGui::TreePop();
				}

				//hack because we can't detect transforms done via gizmo. 
				if (!changed) 
				{
					changed = 
						node->getWorldTransform().translation() != oldTransform.translation() ||
						node->getWorldTransform().scale() != oldTransform.scale() ||
						node->getWorldTransform().rotation().euler() != oldTransform.rotation().euler();
				}

				ImGui::TreePop();
			}
			
			return changed;
		};
#endif
	m_isInited = true;
}
template<>
inline void Spawnable::SpawnSetting::anyVar::setValueByCopy(sptr<Node> val)
{
	m_isCopyValue = true;
	m_copyValue = val;
	wptr<Node> nodeW = val;

#ifdef DEBUG
	onGuiCb =
		[this, nodeW](const std::string& name, anyVar& var) -> bool
		{
			if (nodeW.expired())
				return false;

			bool changed = false;
			if (ImGui::TreeNode(name.c_str()))
			{
				sptr<Node> node = nodeW.lock();

				Transform oldTransform = node->getWorldTransform();
				if (node->is<VisibleNode>() || node->is<Mesh>() || node->is<ParticlesEmitter>())
				{
					ND_Util::ImGuiNodeRender(node, name);
				}
				else
				{
					ND_Util::NoConflictGizmo(node, name);
					changed = ND_Util::ImGuiTransforms(node);
				}

				//submenu for reparenting node
				if (ImGui::TreeNode("Parent node selector"))
				{
					auto parentNode = node->getParent();
					if (ND_Util::ImGuiNodeHierarchy(parentNode, "Parent Node")) //returns true if new node was selected
					{
						node->setParent(parentNode);
					}

					ImGui::TreePop();
				}

				//hack because we can't detect transforms done via gizmo. 
				if (!changed) 
				{
					changed = 
						node->getWorldTransform().translation() != oldTransform.translation() ||
						node->getWorldTransform().scale() != oldTransform.scale() ||
						node->getWorldTransform().rotation().euler() != oldTransform.rotation().euler();
				}

				ImGui::TreePop();
			}
			
			return changed;
		};
#endif
	m_isInited = true;
}

//Template methods impls. Used to add Spawnable Settings (object's exposed variables)
template<typename T>
requires AutoTypeConcept<T> || onGuiImplConcept<T>
inline Spawnable::SpawnSetting::SpawnSetting(
	const std::string& name, 
	T& value,
	std::function<void(std::decay_t<T>&)>&& onChangeCB)
{
	anyVar variable;
	variable.setValue<std::decay_t<T>>(&value);
	settingName = name;
	settingValue = variable;
	onSettingChangeCB =
		[onChangeCB](anyVar& var)
		{
			if (onChangeCB && var.hasValue())
			{
				//value = var.getRef<T>();
				onChangeCB(*var.getRef<T>());
			}
		};
}

template<typename T>
requires AutoTypeConcept<T> || onGuiImplConcept<T>
inline Spawnable::SpawnSetting::SpawnSetting(
	const std::string& name, 
	T&& value,
	std::function<void(std::decay_t<T>)>&& onChangeCB)
{
	anyVar variable;
	variable.setValueByCopy<std::decay_t<T>>(value);
	settingName = name;
	settingValue = variable;
	onSettingChangeCB =
		[onChangeCB](anyVar& var)
		{
			if (onChangeCB && var.hasValue())
			{
				//value = var.getRef<T>();
				onChangeCB(*var.getRef<T>());
			}
		};
}

template<typename T>
requires AutoTypeConcept<T> || onGuiImplConcept<T>
inline Spawnable::SpawnSetting::SpawnSetting(
	const std::string& name,
	const std::decay_t<T> value,
	std::function<void(std::decay_t<T>)>&& onChangeCB)
{
	anyVar variable;
	T copy = value;
	variable.setValueByCopy<std::decay_t<T>>(copy);
	settingName = name;
	settingValue = variable;
	onSettingChangeCB =
		[onChangeCB](anyVar& var)
		{
			if (onChangeCB && var.hasValue())
			{
				//value = var.getRef<T>();
				onChangeCB(*var.getRef<T>());
			}
		};
}

template<typename T> requires AutoTypeConcept<T> || onGuiImplConcept<T>
Spawnable::SpawnSetting& Spawnable::addSpawnableSettingList(const std::string& name, T& value,
	std::function<std::map<std::string, std::decay_t<T>>()>&& listGetter,
	std::function<void(std::decay_t<T>&)>&& onChangeCB)
{
	SpawnSetting&& newSetting = SpawnSetting(name, value, std::move(onChangeCB));
	newSetting.getOnGuiCb() = [this, listGetter](const std::string& settingName, SpawnSetting::anyVar& settingHolder) -> bool
		{
#ifdef DEBUG
			if (!settingHolder.hasValue()) return false;
			std::map<std::string, std::decay_t<T>> mapWithValues = listGetter();
			if (mapWithValues.empty())
			{
				if (ImGui::BeginCombo(SAFE_IMG_NAME(settingName), "NONE"))
				{
					static bool s_falseFlag = false;
					ImGui::Selectable("No valid items found for this setting!\nCheck filter of children!", &s_falseFlag, ImGuiSelectableFlags_Disabled);
					ImGui::EndCombo();
				}
				return false;
			}
			std::string currentValue = "";
			for (auto& [valueName, value] : mapWithValues)
			{
				if (value == *settingHolder.getRef<T>())
				{
					currentValue = valueName;
					break;
				}
			}

			std::pair<bool, T> result = ND_Util::ImGuiComboList(settingName, mapWithValues, currentValue);
			if (result.first)
			{
				*settingHolder.getRef<T>() = result.second;
				return true;
			}
#endif
			return false;
		};

	return addSpawnableSetting(std::move(newSetting));
}

template<typename T> requires (std::is_same_v<T, bool> || std::is_same_v<T, int> || std::is_same_v<T, float> || std::
		is_same_v<T, std::string> || std::is_same_v<T, vec2> || std::is_same_v<T, vec3> || std::is_same_v<T, vec4>)
std::optional<T> Spawnable::getSetting(const std::string& settingName) const
{
	if constexpr (std::is_same_v<T, bool>)
	{
		if (!m_mapObjectData.settings.flags.contains(settingName))
			return std::nullopt;
		return m_mapObjectData.settings.flags.at(settingName);
	}
	if constexpr (std::is_same_v<T, int>)
	{
		if (!m_mapObjectData.settings.integers.contains(settingName))
			return std::nullopt;
		return m_mapObjectData.settings.integers.at(settingName);
	}
	if constexpr (std::is_same_v<T, float>)
	{
		if (!m_mapObjectData.settings.floats.contains(settingName))
			return std::nullopt;
		return m_mapObjectData.settings.floats.at(settingName);
	}
	if constexpr (std::is_same_v<T, std::string>)
	{
		if (!m_mapObjectData.settings.strs.contains(settingName))
			return std::nullopt;
		return m_mapObjectData.settings.strs.at(settingName);
	}
	if constexpr (std::is_same_v<T, vec2>)
	{
		if (!m_mapObjectData.settings.vec2s.contains(settingName))
			return std::nullopt;
		return m_mapObjectData.settings.vec2s.at(settingName);
	}
	if constexpr (std::is_same_v<T, vec3>)
	{
		if (!m_mapObjectData.settings.vec3s.contains(settingName))
			return std::nullopt;
		return m_mapObjectData.settings.vec3s.at(settingName);
	}
	if constexpr (std::is_same_v<T, vec4>)
	{
		if (!m_mapObjectData.settings.vec4s.contains(settingName))
			return std::nullopt;
		return m_mapObjectData.settings.vec4s.at(settingName);
	}
	return std::nullopt;
}