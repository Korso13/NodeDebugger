//Various defines, global variables, settings and utility macros for NodeDebugger components
#pragma once

#include "W4Framework.h"
#include "common/Defines.h"
#include "common/Templates.h"
#include "Concepts.h"

W4_USE_UNSTRICT_INTERFACE

//conditional compilation - debugger switched off in shipping version. Comment out if you need debugger in shipping
#if W4_LOG_MIN_LEVEL >= W4_LOG_LEVEL_ERROR
#define FORCENODEDEBUG_OFF
#define NOCOMP {};
#define NOCOMP_B {return false;};
#define NOCOMP_F {return 1.f;};
#define NOCOMP_PTR {return nullptr;}
class ImGuiNode;
//struct ImVec2 { ImVec2(float, float) {}; };
#endif

#ifndef FORCENODEDEBUG_OFF
#define NOCOMP ; //used for conditional compilation
#define NOCOMP_B ;//used for conditional compilation
#define NOCOMP_F ;
#define NOCOMP_PTR ;//used for conditional compilation
#include "ImGuiNode.h"
#include "ImGuiHelpers.h"
#include "internal/ImGuiFontRoboto.h"
#include "internal/ImGuiDrawerGizmo.h"
#include "internal/ImGuiAuto.h"
#endif

#define jsprint(vec) ND_Util::jsstring(vec).c_str()
#define SAFE_IMG_NAME(name) ((name.empty()) ? ("default_name"+std::to_string(w4::utils::UIDGenerator::get())).c_str() : name.c_str())

//Switches for custom ImGui tabs
#define NDSPINES
#define NDVARIABLES
#define NDBORDERS
#define NDPATHDRAWER
//#define NDSPOTLIGHTS
//#define NDCONSOLE

//Other settings
#define JSON_FLOAT_ACCURACY 5

//=========================================================================================
// Node Debugger SETTINGS and Global Variables
//=========================================================================================

//Common global variables
static bool isDebuggerOn = true;
static bool camControlOn = true;
static bool editorCamOn = false;
static bool degreesOutput = true;
static int  floatAccuracy = SIF("debugNode_floatAccuracy", 2); //number of digits after . 
static int	minLoggingLevel = 1; //what is written in the console log

static std::string keyMappings = { "\nC - camera debug mode\nL - directional light debug mode\nR - switch rotation output mode (radians|degrees)\nP - switchon/off click placement mode\n\nFor camera mode:\nA-D - move camera by X\nW-S - move camera by Y\nQ-E - move camera by Z\nFor light mode:\nA-D - change light direction vector by X\nW-S - change light direction vector by Y\nQ-E - change light direction vector by Z\n\nNode transformations:\n1-9 digits - select debugged node.\nalso\n1 - Gizmo Translate Mode\n2 - Gizmo Rotate Mode\n3 - Gizmo Scale Mode\nArrow Left/Right - move by x\nArrow Up/Down - move by y\nPage Up and Down	 - move by z\nNumPad1 - NumPad3 - rotation along y axis\nNumPad2 - NumPad5 - rotation along x axis\nNumPad4 - NumPad6 - rotation along z axis\nNumpad+ and Numpad-  - scale in all 3 dimentions\nHome - End - scale by x\nInsert - Delete - scale by y\n[  -  ] - scale by z\nMinus - Equal - Decrease / increase translation step\nNumpad7 - Numpad9 - Decrease / increase rotation step\nNumpadDivide(/) - NumpadMultiply(*) - Decrease / increase scaling percent\nAny other key - print selected node's translation\n\n N - print all conected nodes\n" };

//ImGui global variables
static bool		bindXYZScaling = true;
static int		repeatElemId = 0;
static int		timescale = 100;
static float	sliderSensivity = 0.01;

//ImGui settings:
#define IMG_STARTSIZE ImVec2(550, 470)	//ImGui window start size
#define IMG_FONTSIZE 28.f				//font size for all ImGui text
#define IMG_FLOATFORMAT "%.5f"			//float precision format for variables tab
#define IMG_VECFORMAT "%.4f"			//vec2 and vec3 precision format for variables tab
#define IMG_LISTBOX_ENTRIESINFRAME 5	//how many items fit ImGuiListBox height

//console messages colors:
#define IMG_RED		ImVec4(1.f, 0.f, 0.f, 1.f)
#define IMG_ORANGE  ImVec4(1.f, 0.2, 0.f, 1.f)
#define IMG_BLUE	ImVec4(0.f, 0.2, 1.f, 1.f)
#define IMG_VIOLET  ImVec4(0.4, 0.1, 0.5, 1.f)
#define IMG_WHITE	ImVec4(1.f, 1.f, 1.f, 1.f)

//Slider settings (limits):
#define IMG_POSRANGE 900.f
#define IMG_ROTRANGE_DEG 360.f
#define IMG_ROTRANGE_RAD TWO_PI
#define IMG_SCALERANGE 50.f

#define IMG_PLOTTERSTIMESTEP 0.01
//=========================================================================================

//pair of parameter name and its default value
template<typename T>
using ParamPair = std::pair<std::string, T>;

struct ImG_Node
{
	wptr<Node> node;
	std::string name;
	bool markedForDeletion = false;
};

enum class LogLevel
{
	All = 0,
	Debug = 1,
	Info = 2,
	Warning = 3,
	Error = 4
};

enum class tweenTransformType
{
	MOVE,
	SCALE,
	ROTATE
};

struct tweenDebugRecord
{
	std::string tweenName;
	bool isLoopedTween = false;
	bool isRelativeValues = false;
	bool isPercentRelativeValues = false;
	bool isWorldVec3 = true;
	tweenTransformType transformType;
	vec3 tweenStart = vec3::zero;
	std::vector<vec3> tweenPoints;
	std::unordered_map<int, std::pair<bool, vec3>> curveApogeePoints;
	std::vector<float> tweenDelays;
	std::vector<Easing> easingTypes;

	sptr<CustomTweens::Tween<vec3>> tween;
	sptr<CustomTweens::EndlessTween<vec3>> tweenEndless;
};

struct SpineDebugging
{
	std::string spineName;
	sptr<Spine> spine;
	std::vector<std::string> animationNames;
	int lastSelected = 0;
	std::vector<float> animationLength;
	std::vector<std::string> skinNames = { "none" };
	int selectedSkin1 = 0;
	int selectedSkin2 = 0;
	int selectedSkin3 = 0;
	int selectedSkin4 = 0;
	std::map<int, std::string> slots;
};

//Helper struct for using std::visit with vector of variants
//Syntax to use: std::visit(Visitor{[](type1 var){/*type1_handler*/},[](type2 var){/*type2_handler*/}...}, std::variant<>);
template<typename ... Lambdas>
struct Visitor : Lambdas...
{
	Visitor(Lambdas... lambdas) : Lambdas(std::forward<Lambdas>(lambdas))...
	{
	}
	using Lambdas::operator()...;
};

using variable_t = std::variant<bool*, int*, float*, std::string*, vec2*, vec3*, vec4*, ivec2*>;

struct debugVariant
{
	std::string variableName = "";
	variable_t var;
	std::function<void()> onChangeCb = nullptr;
};

//forced to use overload funcs here and in maps because std::is_same<>::value doesn't work properly for some reason((
struct debugVectors
{
	//std::vector<bool>* bools;			void add(std::vector<bool>& collection) { bools = &collection; }  //for some reason - not supported
	std::vector<int>* ints = nullptr;				void add(std::vector<int>& collection) { ints = &collection; }
	std::vector<float>* floats = nullptr;			void add(std::vector<float>& collection) { floats = &collection; }
	std::vector<std::string>* strings = nullptr;	void add(std::vector<std::string>& collection) { strings = &collection; }
	std::vector<vec2>* vec2s = nullptr;				void add(std::vector<vec2>& collection) { vec2s = &collection; }
	std::vector<vec3>* vec3s = nullptr;				void add(std::vector<vec3>& collection) { vec3s = &collection; }
	std::vector<vec4>* vec4s = nullptr;				void add(std::vector<vec4>& collection) { vec4s = &collection; }
};

template<typename KT>
struct debugMaps
{
	std::unordered_map<KT, bool>* um_bools = nullptr;			void add(std::unordered_map<KT, bool>& collection) { um_bools = &collection; }
	std::unordered_map<KT, int>* um_ints = nullptr;				void add(std::unordered_map<KT, int>& collection) { um_ints = &collection; }
	std::unordered_map<KT, float>* um_floats = nullptr;			void add(std::unordered_map<KT, float>& collection) { um_floats = &collection; }
	std::unordered_map<KT, std::string>* um_strings = nullptr;	void add(std::unordered_map<KT, std::string>& collection) { um_strings = &collection; }
	std::unordered_map<KT, vec2>* um_vec2s = nullptr;			void add(std::unordered_map<KT, vec2>& collection) { um_vec2s = &collection; }
	std::unordered_map<KT, vec3>* um_vec3s = nullptr;			void add(std::unordered_map<KT, vec3>& collection) { um_vec3s = &collection; }
	std::unordered_map<KT, vec4>* um_vec4s = nullptr;			void add(std::unordered_map<KT, vec4>& collection) { um_vec4s = &collection; }
	
	std::map<KT, bool>* bools = nullptr;						void add(std::map<KT, bool>& collection) { bools = &collection; }
	std::map<KT, int>* ints = nullptr;							void add(std::map<KT, int>& collection) { ints = &collection; }
	std::map<KT, float>* floats = nullptr;						void add(std::map<KT, float>& collection) { floats = &collection; }
	std::map<KT, std::string>* strings = nullptr;				void add(std::map<KT, std::string>& collection) { strings = &collection; }
	std::map<KT, vec2>* vec2s = nullptr;						void add(std::map<KT, vec2>& collection) { vec2s = &collection; }
	std::map<KT, vec3>* vec3s = nullptr;						void add(std::map<KT, vec3>& collection) { vec3s = &collection; }
	std::map<KT, vec4>* vec4s = nullptr;						void add(std::map<KT, vec4>& collection) { vec4s = &collection; }
};

struct debugMapCollection
{
	debugMapCollection(debugMaps<int>&& maps) : int_maps(maps) {};
	debugMapCollection(debugMaps<std::string>&& maps) : str_maps(maps) {};
	debugMaps<int> int_maps; 
	debugMaps<std::string> str_maps; 
};

struct SpotLightDebugging
{
	std::string name;
	sptr<SpotLight> light;

	vec3 color = vec3::one;
	float angle = 1.f;
	float intensity = 1.f;
	float decayFactor = 0.5f;
	LightDecayRate decayRate = LightDecayRate::Quadratic;
};