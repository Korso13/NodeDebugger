#pragma once
#ifdef DEBUG
#include "ImGuiHelpers.h"
#include "internal/ImGuiAuto.h"
#endif

//template concepts
template<typename T>
concept AutoTypeConcept = requires(T & _value)
{
#ifdef DEBUG
	ImGui::Auto("ConceptTest", _value);
#else
	false;
#endif
};
template<typename T>
concept onGuiImplConcept = requires(T & _value)
{
#ifdef DEBUG
	_value.onGui("ConceptTest");
#else
	false;
#endif
};
template<typename T>
concept serializedEnumConcept = requires(T & _value, int count)
{
	nlohmann::adl_serializer<T>::getValues(count);
};

template<typename T>
concept ContinuousDynamicContainer = requires(T& _value, int count)
{
	_value.reserve(count);
};