#include "Spawnable.h"

//Template methods impls. setValue is used when adding Spawnable Settings for rendering in Spawned Objects submenu
template<typename T>
inline void Spawnable::anyVar::setValue(T val)
{
	m_value = val;
#ifdef DEBUG
	onGuiCb =
		[this](const std::string& name, anyVar& var) -> bool
		{
			return ImGui::Auto(name.c_str(), var.getRef<T>());
		};
#endif
}

template<typename T>
inline void Spawnable::anyVar::setValueByRef(T& val)
{
	m_value = val;
	refCb = [this, val](anyVar& newValue)
		{
			val = newValue.getRef<T>();
		};
#ifdef DEBUG
	onGuiCb =
		[this](const std::string& name, anyVar& var) -> bool
		{
			bool changed = ImGui::Auto(name.c_str(), var.getRef<T>());
			if (changed && refCb) refCb(var);
			return changed;
		};
#endif
}

//specialisation for sptr<Node> because the engine one is shit
template<>
inline void Spawnable::anyVar::setValue(sptr<Node> val)
{
	m_value = val;
#ifdef DEBUG
	onGuiCb =
		[this](const std::string& name, anyVar& var) -> bool
		{
			bool changed = false;
			if (ImGui::TreeNode(name.c_str()))
			{
				sptr<Node> node = var.get<sptr<Node>>();

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
}

//Template methods impls. Used to add Spawnable Settings (object's exposed variables)
template<typename T>
inline Spawnable::SpawnSetting::SpawnSetting(const std::string& name, T& value, std::function<void(std::decay_t<T>&)>&& onChangeCB)
{
	anyVar variable;
	variable.setValue<T>(value);
	settingName = name;
	settingValue = variable;
	onSettingChangeCB =
		[onChangeCB, &value](anyVar var)
		{
			if (onChangeCB && var.hasValue())
			{
				value = var.getRef<T>();
				onChangeCB(var.getRef<T>());
			}
		};
}

template<typename T>
inline Spawnable::SpawnSetting::SpawnSetting(const std::string& name, T& value)
{
	anyVar variable;
	variable.setValue<T>(value);
	settingName = name;
	settingValue = variable;
	onSettingChangeCB = [&value](anyVar var)
		{
			if(var.hasValue())
				value = var.getRef<T>(); 
		};
}

template<typename T>
inline Spawnable::SpawnSetting::SpawnSetting(const std::string& name, T&& value, std::function<void(std::decay_t<T>&)>&& onChangeCB)
{
	anyVar variable;
	variable.setValue<T>(value);
	settingName = name;
	settingValue = variable;
	onSettingChangeCB =
		[onChangeCB, &value](anyVar var)
		{
			if (onChangeCB && var.hasValue())
			{
				onChangeCB(var.getRef<T>());
			}
		};
}

template<typename T>
inline Spawnable::SpawnSetting Spawnable::SpawnSetting::makeSpawnSetting(const std::string& name, std::decay_t<T> value, std::function<void(T&)>&& onChangeCB)
{
	SpawnSetting setting(name);
	anyVar variable;
	variable.setValue<T>(value);
	setting.settingValue = variable;
	setting.onSettingChangeCB =
		[onChangeCB](anyVar var)
		{
			if (onChangeCB && var.hasValue())
				onChangeCB(var.getRef<T>());
		};
	return setting;
}