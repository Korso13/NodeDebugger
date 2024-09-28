//Template interface class of a tab for ND_ImGui
#pragma once

#include "W4Framework.h"
#ifndef FORCENODEDEBUG_OFF
#include "common/NodeDebugger/ND_Defines.h"
#include "ND_Utilities.h"
#include <any>
#endif

W4_USE_UNSTRICT_INTERFACE

class iGuiTab
{
public:
	iGuiTab(std::string tabName = "DefaultTabName") : m_tabName(tabName) {};

	//called from onGui() method of ND_ImGui:
	virtual void onGuiCall() = 0; //ImGui draw instructions to be called (no need to call BeginTabItem()!!!)
	virtual bool isEmpty() = 0; //tab is not rendered if this returns true

	const char* getTabName() const { return m_tabName.c_str(); }

	//used to add new elements to the tab. Use any_cast<>() to cast item to desired type
	virtual void addItem(std::any item, const std::string& altName = "default") = 0;
	virtual void removeItem(const std::string& itemName) {};

	//Not so convenient, need to cast iGuiTab to specific class to work and compile properly.
	//template<typename T>
	//void addItemTyped(T item, const std::string& altName = "default");

	//Clear all records, especially sptr and lambda's that capture sptr. Needed for restarts and NodeDebugger purge method
	virtual void purgeValues() = 0;

private:
	std::string m_tabName;
};