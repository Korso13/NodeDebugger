#pragma once
#include "W4Framework.h"
#include "ND_Defines.h"

W4_USE_UNSTRICT_INTERFACE

#define ND_PartEmitter ND_Util::particleEmitter_onGui


namespace ND_Util
{
	enum BtnType
	{
		STOP,
		PAUSE,
		RANGE,
		RANGE_OFF,
		INFIN,
		INFIN_OFF
	};

	const sptr<Texture>& getBtnTexture(ND_Util::BtnType type);
	void imGuiEmitterEditor(const sptr<ParticlesEmitter>& _emitter) NOCOMP;
	void emitterSettingsDraw(const sptr<ParticlesEmitter>& _emitter, ParticlesEmitterParameters& params) NOCOMP;
	void particleSettingsDraw(ParticlesEmitterParameters& params) NOCOMP;
	void colorSettingsDraw(ParticlesEmitterParameters& params) NOCOMP;

	void ImGuiOptionalRangeFloat(const char* label, bool rangedValue, float* value, float* variance, float minValue, float maxValue, bool useFloatInputForNonRange = true) NOCOMP;

	static std::map<std::string, int> rangeFloatSliderIds;
}