#include "external/boost/preprocessor/arithmetic/inc.hpp"
#include "external/boost/preprocessor/comparison/not_equal.hpp"
#include "external/boost/preprocessor/repetition/for.hpp"
#include "external/boost/preprocessor/tuple/elem.hpp"
#include "external/boost/preprocessor/tuple/eat.hpp"

//Helper methods for generating variants of registerSpawnableClass() for different amount of constructor arguements used in the spawnable entity's constructor
//Some reference info for possible refactoring: 
// BOOST_PP_CAT - concatenation macro (we can't use ## here!)
// BOOST_PP_TUPLE_ELEM - unpacks a tuple (passed in () to BOOST_PP_FOR) arguement. 1st arg - tuple size, 2nd arg - tuple element index, 3rd arguement - tuple for unpacking
// ADD_ - code added in each iteration of BOOST_PP_FOR
// ADD_..._MACRO - predicate via which BOOST_PP_FOR calls ADD_ macros each iteration and passes them new value
// PRED and OP - helper macros needed for BOOST_PP_FOR work
// BOOST_PP_DEC - decreases value by 1. Needed to resolve concatenation errors in BOOST macros when passing calculation result instead of value (e.g. 12-1 instead of 11)
//BOOST_PP_NOT_EQUAL(f, s) - returns "true" if "f" and "s" are not equal
// (unused for now)BOOST_PP_IF(pred, t, f) - if predicate returns "true", "t" is used, else "f" is used

//Adds template param for registerSpawnableClass() specialization in the format of "T#", where # - is 1-based number of template parameter
#define ADD_TEMPLATE_PARAM(num) typename BOOST_PP_CAT(T, num),
//Registeres constructor param for spawnable entity and exposes it to ImGui menu
#define REG_CONSTRUCTOR_PARAM(num) m_classesRegistry[classID].addConstructorParam<BOOST_PP_CAT(T, num)>(BOOST_PP_CAT(param, num).first, BOOST_PP_CAT(param, num).second);
//Adds function param for registerSpawnableClass() overload in the format of "ParamPair<T#> param#", where # - is 1-based number of template/function parameter
#define ADD_FUNC_PARAM(num) ParamPair<BOOST_PP_CAT(T, num)> BOOST_PP_CAT(param, num),
//Used for iterative addition of arguements to spawnObject() call. Refers to template parameter types to deduce std::variable type in get<>() function. Arguements are stored in zero-based vector (hence num-1)
#define ADD_SPAWNER_ARG(num) args[num-1].paramValue.get<BOOST_PP_CAT(T, num)>(),
//Used for adding closing arguement to spawnObject() call (needed because ADD_SPAWNER_ARG ends with comma(,))
#define ADD_SPAWNER_ARG_FINAL(num) args[num-1].paramValue.get<BOOST_PP_CAT(T, num)>()
//placeholder macros for closing arguement\parameter in template and function arguements list (needed because ADD_TEMPLATE_PARAM and ADD_FUNC_PARAM end with comma(,))
#define TEMPLATE_PARAMS_END typename PHT = bool
#define FUNC_PARAMS_END bool phArg = true

//Helper macros for iterative BOOST_PP_FOR macro
#define PRED(r, state) \
   BOOST_PP_NOT_EQUAL( \
      BOOST_PP_TUPLE_ELEM(2, 0, state), \
      BOOST_PP_INC(BOOST_PP_TUPLE_ELEM(2, 1, state)) \
   ) \

#define OP(r, state) \
   ( \
      BOOST_PP_INC(BOOST_PP_TUPLE_ELEM(2, 0, state)), \
      BOOST_PP_TUPLE_ELEM(2, 1, state) \
   ) \
//===

//predicates to be called each iteration from BOOST_PP_FOR
#define ADD_TEMPLATE_PARAM_MACRO(r, state) ADD_TEMPLATE_PARAM(BOOST_PP_TUPLE_ELEM(2, 0, state))
#define REG_PARAM_MACRO(r, state) REG_CONSTRUCTOR_PARAM(BOOST_PP_TUPLE_ELEM(2, 0, state))
#define ADD_FUNC_PARAM_MACRO(r, state) ADD_FUNC_PARAM(BOOST_PP_TUPLE_ELEM(2, 0, state))
#define ADD_SPAWNER_ARG_MACRO(r, state) ADD_SPAWNER_ARG(BOOST_PP_TUPLE_ELEM(2, 0, state))
//===

//Macro for generating registerSpawnableClass() variant with arg_count of template/function parameters (number of arguements for spawnable class constructor)
//Generated function will have arg_count template parameters (see TEMPLATE_PARAMS_END) and arg_count+2 function parameters (1st param always const std::string& entityName. For the last one see FUNC_PARAMS_END)
// registerSpawnableClass() checks if the class has bee registered. If not, it creates a new SpawnerRecord, 
// exposes arg_count constructor arguements for the class' constructor to ImGui via addConstructorParam() method (see REG_PARAM_MACRO/REG_CONSTRUCTOR_PARAM)
// and then generates a spawner lambda (m_spawnerFunc field in SpawnerRecord) that takes a vector of arguements (in form of ParamPack), 
// which are then passed to class constructor (see ADD_SPAWNER_ARG_MACRO/ADD_SPAWNER_ARG)
#define ADD_REGISTER_SPAWNABLE_FUNC_MACRO(arg_count) \
	template<typename className, BOOST_PP_FOR((1, arg_count), PRED, OP, ADD_TEMPLATE_PARAM_MACRO) TEMPLATE_PARAMS_END> \
	SpawnerRecord& registerSpawnableClass(const std::string& entityName, BOOST_PP_FOR((1, arg_count), PRED, OP, ADD_FUNC_PARAM_MACRO) FUNC_PARAMS_END) \
	{ \
		std::string classID = entityName; \
		if (!m_classesRegistry.contains(classID)) \
		{ \
			m_classesRegistry.emplace(classID, SpawnerRecord(classID));\
			BOOST_PP_FOR((1, arg_count), PRED, OP, REG_PARAM_MACRO) \
			m_classesRegistry[classID].m_spawnerFunc = [this](std::vector<SpawnerRecord::ParamPack> args) \
			{ \
				return cast_sptr<Spawnable>(std::shared_ptr<className>(new className( \
							BOOST_PP_FOR((1, BOOST_PP_DEC(arg_count)), PRED, OP, ADD_SPAWNER_ARG_MACRO) \
							ADD_SPAWNER_ARG_FINAL(arg_count)\
						) \
					) \
				); \
			}; \
		} \
		return m_classesRegistry[classID]; \
}