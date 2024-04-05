//Various defines, global variables, settings and utility macros for NodeDebugger components
#pragma once

#include "W4Framework.h"
#include "common/Defines.h"
#include "common/Templates.h"

W4_USE_UNSTRICT_INTERFACE

//conditional compilation - debugger switched off in shipping version. Comment out if you need debugger in shipping
#if W4_LOG_MIN_LEVEL >= W4_LOG_LEVEL_ERROR
#define FORCENODEDEBUG_OFF
#define NOCOMP {};
#define NOCOMP_B {return false;};
#define NOCOMP_F {return 1.f;};
#define NOCOMP_PTR {return nullptr;}
class ImGuiNode;
struct ImVec2 { ImVec2(float, float) {}; };
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
#endif

#define jsprint(vec) ND_Util::jsstring(vec).c_str()

//Switches for custom ImGui tabs
#define NDSPINES
#define NDVARIABLES
#define NDBORDERS
//#define NDSPOTLIGHTS
//#define NDCONSOLE

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
#define IMG_STARTSIZE ImVec2(550, 430)	//ImGui window start size
#define IMG_FONTSIZE 28.f				//font size for all ImGui text

//console messages colors:
#define IMG_RED		ImVec4(1.f, 0.f, 0.f, 1.f)
#define IMG_ORANGE  ImVec4(1.f, 0.2, 0.f, 1.f)
#define IMG_VIOLET  ImVec4(0.4, 0.1, 0.5, 1.f)
#define IMG_WHITE	ImVec4(1.f, 1.f, 1.f, 1.f)

//Slider settings (limits):
#define IMG_POSRANGE 100.f
#define IMG_ROTRANGE_DEG 360.f
#define IMG_ROTRANGE_RAD TWO_PI
#define IMG_SCALERANGE 30.f
//=========================================================================================

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

struct tweenDebugRecord
{
	std::string tweenName;
	bool isLoopedTween = false;
	bool isRelativeValues = false;
	bool isPercentRelativeValues = false;
	bool isMoveTween = true;
	bool isWorldVec3 = true;
	vec3 tweenStart = vec3::zero;
	std::vector<vec3> tweenPoints;
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

template<typename T>
struct debugVar
{
	T* var = nullptr;
	std::function<void()> onChangeCb = nullptr;
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