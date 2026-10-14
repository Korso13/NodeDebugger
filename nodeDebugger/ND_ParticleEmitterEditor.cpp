#include "ND_ParticleEmitterEditor.h"

#ifndef FORCENODEDEBUG_OFF
#include "math/ne_math.h"
#include "ND_Utilities.h"

static bool bEmitterEdited = false;

ImVec2 btnSize = ImVec2(27, 27);
std::string base64Appender = "data:image/jpeg;base64,";
static sptr<Texture> fallbackBtnTxt;
static sptr<Texture> stopBtnTxt;
static sptr<Texture> pauseBtnTxt;
static sptr<Texture> rangeBtnTxt;
static sptr<Texture> rangeOffBtnTxt;
static sptr<Texture> infinityBtnTxt;
static sptr<Texture> infinityOffBtnTxt;

const char* const emitterTransformTypes[2] = { "EMITTER", "PARTICLE" };
const char* const emitterTypes[2] = { "GRAVITY", "RADIUS" };

//Button texture getter
const sptr<Texture>& ND_Util::getBtnTexture(ND_Util::BtnType type)
{
	switch (type)
	{
	case ND_Util::BtnType::STOP:
		if (!stopBtnTxt)
		{
			std::string stopImgB64 = base64Appender + "/9j/4AAQSkZJRgABAQEAYABgAAD/4QBoRXhpZgAATU0AKgAAAAgABAEaAAUAAAABAAAAPgEbAAUAAAABAAAARgEoAAMAAAABAAIAAAExAAIAAAARAAAATgAAAAAAAABgAAAAAQAAAGAAAAABcGFpbnQubmV0IDUuMC4xMwAA/9sAQwAGBAUGBQQGBgUGBwcGCAoQCgoJCQoUDg8MEBcUGBgXFBYWGh0lHxobIxwWFiAsICMmJykqKRkfLTAtKDAlKCko/9sAQwEHBwcKCAoTCgoTKBoWGigoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgo/8AAEQgAIAAgAwESAAIRAQMRAf/EAB8AAAEFAQEBAQEBAAAAAAAAAAABAgMEBQYHCAkKC//EALUQAAIBAwMCBAMFBQQEAAABfQECAwAEEQUSITFBBhNRYQcicRQygZGhCCNCscEVUtHwJDNicoIJChYXGBkaJSYnKCkqNDU2Nzg5OkNERUZHSElKU1RVVldYWVpjZGVmZ2hpanN0dXZ3eHl6g4SFhoeIiYqSk5SVlpeYmZqio6Slpqeoqaqys7S1tre4ubrCw8TFxsfIycrS09TV1tfY2drh4uPk5ebn6Onq8fLz9PX29/j5+v/EAB8BAAMBAQEBAQEBAQEAAAAAAAABAgMEBQYHCAkKC//EALURAAIBAgQEAwQHBQQEAAECdwABAgMRBAUhMQYSQVEHYXETIjKBCBRCkaGxwQkjM1LwFWJy0QoWJDThJfEXGBkaJicoKSo1Njc4OTpDREVGR0hJSlNUVVZXWFlaY2RlZmdoaWpzdHV2d3h5eoKDhIWGh4iJipKTlJWWl5iZmqKjpKWmp6ipqrKztLW2t7i5usLDxMXGx8jJytLT1NXW19jZ2uLj5OXm5+jp6vLz9PX29/j5+v/aAAwDAQACEQMRAD8A4tlLSlVBLFsAAZJr0z4E6JDqXiu5vrlBImnoHjBGQJGOFP4AN+OD2pgRaB8H9e1K2S4vpbfTkcZEcuWkA91HA+hOfUV2fxR+Iur+FvEken6bb2LxG3WUtOjM2SWHZhxwKAOB8U/CvXtCtJLuLydQtYxlzb53oPUqR0+ma9O+EvjnUvF9xqcWqQ2kf2ZY2Q26MudxbOcsfQUAfNl9/wAeU/8A1zb+Vdn8a9Eh0LxVqUVogjtriH7TGi9F3Agge24Nx2FAG38EvEEGjeLZrW8kEcGoIIg54AkBymfrlh9SK85k++31oA+hviR8NrnxZr8eo22oQ24ECwlJIyeQWOcg+9eYaB8UfE2jWq263EN7Cg2oLxC5Uem4EE/iTQB658MvAs3guTUZ7u/huPtCIBsQqEC7iSST71414o+IniHxHbNbXdzHBaOMPBapsV/qSSxHtnFAFb4wa9D4i8TaldWj77SKL7PC/wDeVQcn6FixHtiuSvv+PKf/AK5t/KgD/9k=";
			ResourceLoadDescr stopImgDscr(stopImgB64.c_str());
			sptr<w4::resources::Image> stopImg = w4::resources::Image::loadFromUrl(stopImgDscr, stopImgB64.c_str());
			stopBtnTxt = make::sptr<Texture>(stopImg->getDescr(), stopImg);
		}

		return stopBtnTxt;
		break;
	case ND_Util::BtnType::PAUSE:
		if (!pauseBtnTxt)
		{
			std::string pauseImgB64 = base64Appender + "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";
			ResourceLoadDescr stopImgDscr(pauseImgB64.c_str());
			sptr<w4::resources::Image> pauseImg = w4::resources::Image::loadFromUrl(stopImgDscr, pauseImgB64.c_str());
			pauseBtnTxt = make::sptr<Texture>(pauseImg->getDescr(), pauseImg);
		}

		return pauseBtnTxt;
		break;
	case ND_Util::BtnType::RANGE:
		if (!rangeBtnTxt)
		{
			std::string rangeImgB64 = base64Appender + "/9j/4AAQSkZJRgABAQEAYABgAAD/4QBoRXhpZgAATU0AKgAAAAgABAEaAAUAAAABAAAAPgEbAAUAAAABAAAARgEoAAMAAAABAAIAAAExAAIAAAARAAAATgAAAAAAAABgAAAAAQAAAGAAAAABcGFpbnQubmV0IDUuMC4xMwAA/9sAQwAGBAUGBQQGBgUGBwcGCAoQCgoJCQoUDg8MEBcUGBgXFBYWGh0lHxobIxwWFiAsICMmJykqKRkfLTAtKDAlKCko/9sAQwEHBwcKCAoTCgoTKBoWGigoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgo/8AAEQgAIwAjAwESAAIRAQMRAf/EAB8AAAEFAQEBAQEBAAAAAAAAAAABAgMEBQYHCAkKC//EALUQAAIBAwMCBAMFBQQEAAABfQECAwAEEQUSITFBBhNRYQcicRQygZGhCCNCscEVUtHwJDNicoIJChYXGBkaJSYnKCkqNDU2Nzg5OkNERUZHSElKU1RVVldYWVpjZGVmZ2hpanN0dXZ3eHl6g4SFhoeIiYqSk5SVlpeYmZqio6Slpqeoqaqys7S1tre4ubrCw8TFxsfIycrS09TV1tfY2drh4uPk5ebn6Onq8fLz9PX29/j5+v/EAB8BAAMBAQEBAQEBAQEAAAAAAAABAgMEBQYHCAkKC//EALURAAIBAgQEAwQHBQQEAAECdwABAgMRBAUhMQYSQVEHYXETIjKBCBRCkaGxwQkjM1LwFWJy0QoWJDThJfEXGBkaJicoKSo1Njc4OTpDREVGR0hJSlNUVVZXWFlaY2RlZmdoaWpzdHV2d3h5eoKDhIWGh4iJipKTlJWWl5iZmqKjpKWmp6ipqrKztLW2t7i5usLDxMXGx8jJytLT1NXW19jZ2uLj5OXm5+jp6vLz9PX29/j5+v/aAAwDAQACEQMRAD8A4mr2mWEV4rmW6WEg4APeuTEY+jh5ctRv7mdFLC1KyvD8yjVvVtPk0yRfMYPC/KOO9VhsbQxK/dSuKthqtH41YqVp6bost5CZp3+z2/Zm4JrKvmeGoS5Zy17LX8i6WDrVVzRjoZefeup17QrO00aK4hlAlReuf9ZXHhc+w+Iquik0/Q6K+V1aNP2jaOXpAcivbPNExnOFY/7oq/p2qTaerLFHG4Y5ywrixFXERlalTUl62OilCjJXqTs/QvaZp+LX7ZrMjraRDMcTHk1majqFxqMga5bheFQdBXnfUMViZc1Zqmu0d36s7PrVCjG1NOT7vb7i1rF1ealGs6ROtgPljVKi07VrrToykO14TzsboKccJWwUv9mpxlH8fvE8RTxMf302n+A2+gv0s7RrzzDb7f3eeg+tbGteJI77SY7aKL946/vCRwv0rny+tjHiJKdBJX32/HqbYunh/ZJxqtvt/WxzYORQBgYr6U8YKKACigAooAKKACigD//Z";
			ResourceLoadDescr rangeImgDscr(rangeImgB64.c_str());
			sptr<w4::resources::Image> rangeImg = w4::resources::Image::loadFromUrl(rangeImgDscr, rangeImgB64.c_str());
			rangeBtnTxt = make::sptr<Texture>(rangeImg->getDescr(), rangeImg);
		}

		return rangeBtnTxt;
		break;
	case ND_Util::BtnType::RANGE_OFF:
		if (!rangeOffBtnTxt)
		{
			std::string rangeOffImgB64 = base64Appender + "/9j/4AAQSkZJRgABAQEAYABgAAD/4QBoRXhpZgAATU0AKgAAAAgABAEaAAUAAAABAAAAPgEbAAUAAAABAAAARgEoAAMAAAABAAIAAAExAAIAAAARAAAATgAAAAAAAABgAAAAAQAAAGAAAAABcGFpbnQubmV0IDUuMC4xMwAA/9sAQwAGBAUGBQQGBgUGBwcGCAoQCgoJCQoUDg8MEBcUGBgXFBYWGh0lHxobIxwWFiAsICMmJykqKRkfLTAtKDAlKCko/9sAQwEHBwcKCAoTCgoTKBoWGigoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgo/8AAEQgAIwAjAwESAAIRAQMRAf/EAB8AAAEFAQEBAQEBAAAAAAAAAAABAgMEBQYHCAkKC//EALUQAAIBAwMCBAMFBQQEAAABfQECAwAEEQUSITFBBhNRYQcicRQygZGhCCNCscEVUtHwJDNicoIJChYXGBkaJSYnKCkqNDU2Nzg5OkNERUZHSElKU1RVVldYWVpjZGVmZ2hpanN0dXZ3eHl6g4SFhoeIiYqSk5SVlpeYmZqio6Slpqeoqaqys7S1tre4ubrCw8TFxsfIycrS09TV1tfY2drh4uPk5ebn6Onq8fLz9PX29/j5+v/EAB8BAAMBAQEBAQEBAQEAAAAAAAABAgMEBQYHCAkKC//EALURAAIBAgQEAwQHBQQEAAECdwABAgMRBAUhMQYSQVEHYXETIjKBCBRCkaGxwQkjM1LwFWJy0QoWJDThJfEXGBkaJicoKSo1Njc4OTpDREVGR0hJSlNUVVZXWFlaY2RlZmdoaWpzdHV2d3h5eoKDhIWGh4iJipKTlJWWl5iZmqKjpKWmp6ipqrKztLW2t7i5usLDxMXGx8jJytLT1NXW19jZ2uLj5OXm5+jp6vLz9PX29/j5+v/aAAwDAQACEQMRAD8A4mul8K6FperQSvqOsR2MithUYdR60wOZzXSeMvC8nh2SB47qK8s7gZiljPX2xQBzea7rw/4Aa609b7Xr6PTLaQgRrJwz0AcLmvWvih4P0HSvDtle2FzHFcrEqhQf9ePWgDycUg6UAXbDR9Q1JWawsprhVOCUXIBqzo3iTV9EieLS76S3jdtzKoHJoA7jRtFtPCelJqvjCVpLhRm0sGOcHtxXneq6ne6tdG51K5kuZsY3OegoA7HxFZeJPGVrDrSJ59o52RW8J/1Q7cVzOh+ItW0Ld/Zd7LAjHLIDwaANzxt4T17SbCwvNSkkubbyVAJ/5Y8fdIqbxl8RdR8R6Xb2G0wwLGBMepkbufpQBxA6UDpQBEXOetFACbz60UAG8+tFABvPrRQAoc+tFAH/2Q==";
			ResourceLoadDescr rangeOffImgDscr(rangeOffImgB64.c_str());
			sptr<w4::resources::Image> rangeOffImg = w4::resources::Image::loadFromUrl(rangeOffImgDscr, rangeOffImgB64.c_str());
			rangeOffBtnTxt = make::sptr<Texture>(rangeOffImg->getDescr(), rangeOffImg);
		}

		return rangeOffBtnTxt;
		break;
	case ND_Util::BtnType::INFIN:
		if (!infinityBtnTxt)
		{
			std::string infinImgB64 = base64Appender + "/9j/4AAQSkZJRgABAQEAYABgAAD/4QBoRXhpZgAATU0AKgAAAAgABAEaAAUAAAABAAAAPgEbAAUAAAABAAAARgEoAAMAAAABAAIAAAExAAIAAAARAAAATgAAAAAAAABgAAAAAQAAAGAAAAABcGFpbnQubmV0IDUuMC4xMwAA/9sAQwAGBAUGBQQGBgUGBwcGCAoQCgoJCQoUDg8MEBcUGBgXFBYWGh0lHxobIxwWFiAsICMmJykqKRkfLTAtKDAlKCko/9sAQwEHBwcKCAoTCgoTKBoWGigoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgo/8AAEQgAIwAjAwESAAIRAQMRAf/EAB8AAAEFAQEBAQEBAAAAAAAAAAABAgMEBQYHCAkKC//EALUQAAIBAwMCBAMFBQQEAAABfQECAwAEEQUSITFBBhNRYQcicRQygZGhCCNCscEVUtHwJDNicoIJChYXGBkaJSYnKCkqNDU2Nzg5OkNERUZHSElKU1RVVldYWVpjZGVmZ2hpanN0dXZ3eHl6g4SFhoeIiYqSk5SVlpeYmZqio6Slpqeoqaqys7S1tre4ubrCw8TFxsfIycrS09TV1tfY2drh4uPk5ebn6Onq8fLz9PX29/j5+v/EAB8BAAMBAQEBAQEBAQEAAAAAAAABAgMEBQYHCAkKC//EALURAAIBAgQEAwQHBQQEAAECdwABAgMRBAUhMQYSQVEHYXETIjKBCBRCkaGxwQkjM1LwFWJy0QoWJDThJfEXGBkaJicoKSo1Njc4OTpDREVGR0hJSlNUVVZXWFlaY2RlZmdoaWpzdHV2d3h5eoKDhIWGh4iJipKTlJWWl5iZmqKjpKWmp6ipqrKztLW2t7i5usLDxMXGx8jJytLT1NXW19jZ2uLj5OXm5+jp6vLz9PX29/j5+v/aAAwDAQACEQMRAD8A5yrem6dNqMj+WyxxR/fkboK48Vj8PhP40rfmdFDC1a/8NXKla9xoTNEz6fdR3JT7yKwJrmhnWDnLlcreqaNpZbiIq6V/TUyK07LRJni869lW1hzgbzgmqq5vhKT5XO78tfyFTy/ETV+Wy89DMrQ1LSpLKEXMUq3FqTgspztq8NmmGxMuSnLXs9GTWwVaiuaUdO5n0V6ByG7pC/bdBvbGB1W53bsE43DisNCySB42ZHHRlODXh47KZ1sQsVRlaa76o9PC4+NOk6FSN4vtubmhaXc2V4Lu6K20EY+bLferHuLie4XbPPJIvoTxWeIy7G42Ps8ROKj5LX8S6OLw2GfPSi2/NnQeILV9YWK50+RJ4UBBjB/Wudgllt2Jt5XjJ67T1qMPk+IwDbwk00/5l+qKq5hSxSXt4u67M3BA+l+GruO9Kq84KxxA5wSKw5pJJ3DzyNIw6Fj0q4ZRWrYmOJxU1eOyiiZY+nToujQi7PuMor6A8kKKACigAooAKKACigD/2Q==";
			ResourceLoadDescr infinImgDscr(infinImgB64.c_str());
			sptr<w4::resources::Image> infinImg = w4::resources::Image::loadFromUrl(infinImgDscr, infinImgB64.c_str());
			infinityBtnTxt = make::sptr<Texture>(infinImg->getDescr(), infinImg);
		}

		return infinityBtnTxt;
		break;
	case ND_Util::BtnType::INFIN_OFF:
		if (!infinityOffBtnTxt)
		{
			std::string infinOffImgB64 = base64Appender + "/9j/4AAQSkZJRgABAQEAYABgAAD/4QBoRXhpZgAATU0AKgAAAAgABAEaAAUAAAABAAAAPgEbAAUAAAABAAAARgEoAAMAAAABAAIAAAExAAIAAAARAAAATgAAAAAAAABgAAAAAQAAAGAAAAABcGFpbnQubmV0IDUuMC4xMwAA/9sAQwAGBAUGBQQGBgUGBwcGCAoQCgoJCQoUDg8MEBcUGBgXFBYWGh0lHxobIxwWFiAsICMmJykqKRkfLTAtKDAlKCko/9sAQwEHBwcKCAoTCgoTKBoWGigoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgo/8AAEQgAIwAjAwESAAIRAQMRAf/EAB8AAAEFAQEBAQEBAAAAAAAAAAABAgMEBQYHCAkKC//EALUQAAIBAwMCBAMFBQQEAAABfQECAwAEEQUSITFBBhNRYQcicRQygZGhCCNCscEVUtHwJDNicoIJChYXGBkaJSYnKCkqNDU2Nzg5OkNERUZHSElKU1RVVldYWVpjZGVmZ2hpanN0dXZ3eHl6g4SFhoeIiYqSk5SVlpeYmZqio6Slpqeoqaqys7S1tre4ubrCw8TFxsfIycrS09TV1tfY2drh4uPk5ebn6Onq8fLz9PX29/j5+v/EAB8BAAMBAQEBAQEBAQEAAAAAAAABAgMEBQYHCAkKC//EALURAAIBAgQEAwQHBQQEAAECdwABAgMRBAUhMQYSQVEHYXETIjKBCBRCkaGxwQkjM1LwFWJy0QoWJDThJfEXGBkaJicoKSo1Njc4OTpDREVGR0hJSlNUVVZXWFlaY2RlZmdoaWpzdHV2d3h5eoKDhIWGh4iJipKTlJWWl5iZmqKjpKWmp6ipqrKztLW2t7i5usLDxMXGx8jJytLT1NXW19jZ2uLj5OXm5+jp6vLz9PX29/j5+v/aAAwDAQACEQMRAD8A5yvTfhl8N08Q2i6prEkkdgWxFFGcNLg8knsO3HP0oA8yr37xB8H9GubFv7FeayvFBKF5C6OewbOSB7j9aAPAa908KfB6xgtBJ4kke5umHMUMhWNPxGCT+lAHhdezfED4VW9tppvvDCSB4FLS2zuW3r6qT3Hp3+vUA8ZooA+oPhvcxXvgDTUsrhGkjtxCxU5MbgdCPUV89+FfE+p+GLuSfSpgvmLtkjcbkf0yPUetAH0D4D8Man4ekvH1TW5tS8/BVGztU9zySc/SvGdZ+J3iTVbB7SSeG3R8bntkKOR6bs8fhQB7j440K88RaTHbaZqsmnOsocvGThxg8HBB9/wrwTwz8QNe8O2Js7KaKS33blWdC+wnrjngGgD3qxtJPDfgeWDVtRe7NtBI0lzLnpycdzx0r5+8U+N9b8SxxxahcBIEH+pgBRG92GefxoA5migAooAKKACigAooAKKAP//Z";
			ResourceLoadDescr infinOffImgDscr(infinOffImgB64.c_str());
			sptr<w4::resources::Image> infinOffImg = w4::resources::Image::loadFromUrl(infinOffImgDscr, infinOffImgB64.c_str());
			infinityOffBtnTxt = make::sptr<Texture>(infinOffImg->getDescr(), infinOffImg);
		}

		return infinityOffBtnTxt;
		break;
	default:
		break;
	}

	if (!fallbackBtnTxt)
		fallbackBtnTxt = Texture::createWithColor(color::White); //fallback
	return fallbackBtnTxt;
}


void ND_Util::imGuiEmitterEditor(const sptr<ParticlesEmitter>& _emitter)
{
	if (!_emitter)
		return;

	bEmitterEdited = false;
	//Play, stop btns
	if (ImGui::ArrowButton("Play", 1))
	{
		_emitter->start();
	}
	ImGui::SameLine();
	if (ImGui::ImageButton(getBtnTexture(BtnType::STOP), btnSize))
	{
		_emitter->stop();
	}

	ParticlesEmitterParameters params = _emitter->getParameters();
	auto materialInst = _emitter->getMaterialInst(); //required in case the particle has non-standard material as setParameters resets it to default one  

	//Emitter settings
	emitterSettingsDraw(_emitter, params);
	//Particle settings
	particleSettingsDraw(params);
	//Color and blending settings
	colorSettingsDraw(params);

	if (bEmitterEdited)
	{
		_emitter->setParameters(params);
		_emitter->setMaterialInst(materialInst); 
	}
}

void ND_Util::emitterSettingsDraw(const sptr<ParticlesEmitter>& _emitter, ParticlesEmitterParameters& params)
{
	if (ImGui::TreeNode("Emitter Settings"))
	{
		//Emitter type selector
		int selectedEmitterTransformType = static_cast<int>(_emitter->getParameters().transformMode);
		if (ImGui::Combo("Emmitter transform type", &selectedEmitterTransformType, emitterTransformTypes, 2))
		{
			bEmitterEdited = true;
			if (selectedEmitterTransformType == 0)
			{
				_emitter->setTransformMode(ParticlesEmitterParameters::TransformMode::EMITTER);
				params.transformMode = ParticlesEmitterParameters::TransformMode::EMITTER;
			}
			else
			{
				_emitter->setTransformMode(ParticlesEmitterParameters::TransformMode::PARTICLE);
				params.transformMode = ParticlesEmitterParameters::TransformMode::PARTICLE;
			}
		}

		if (ImGui::ImageButton(getBtnTexture((params.duration == -1.f) ? BtnType::INFIN : BtnType::INFIN_OFF), btnSize))
		{
			bEmitterEdited = true; 
			if (params.duration == -1.f)
				params.duration = 1.f;
			else
				params.duration = -1.f;
		}
		ImGui::SameLine();
		if(ImGui::InputFloat("Duration", &params.duration)) 
			bEmitterEdited = true;

		if(ImGui::DragVec2("Source position", params.sourcePosition.value, -100.f, 100.f))
			bEmitterEdited = true;
		if(ImGui::DragVec2("Source position variance", params.sourcePosition.variance, -100.f, 100.f))
			bEmitterEdited = true;

		int particlesLimit = params.maxParticles;
		if (ImGui::InputInt("Maximum particles", &particlesLimit, 1, 100))
		{
			bEmitterEdited = true;

			if (particlesLimit < 1)
				particlesLimit = 1;

			params.maxParticles = static_cast<uint32_t>(particlesLimit);
		}

		//Emitter angle editor (not using ImGuiOptionalRangeFloat because of the setting specific setters)
		bool angleRangeOn = params.angle.variance != 0.f;
		//Angle range switch
		if (ImGui::ImageButton(getBtnTexture(angleRangeOn ? BtnType::RANGE : BtnType::RANGE_OFF), btnSize))
		{
			bEmitterEdited = true;

			if (angleRangeOn)
				params.angle.variance = 0.f;
			else
			{
				params.angle.variance = HALF_PI / 2.f;
			}
		}
		//quick angle setters
		ImGui::SameLine();
		if (ImGui::ArrowButton("Right", 1))
		{
			bEmitterEdited = true;
			params.angle.value = 0.f;
			if (angleRangeOn)
				params.angle.variance = HALF_PI/2.f;
		}
		ImGui::SameLine();
		if (ImGui::ArrowButton("Down", 3))
		{
			bEmitterEdited = true; 
			params.angle.value = HALF_PI;;
			if (angleRangeOn)
				params.angle.variance = HALF_PI / 2.f;
		}
		ImGui::SameLine();
		if (ImGui::ArrowButton("Left", 0))
		{
			bEmitterEdited = true;
			params.angle.value = PI;
			if (angleRangeOn)
				params.angle.variance = HALF_PI / 2.f;
		}
		ImGui::SameLine();
		if (ImGui::ArrowButton("Up", 2))
		{
			bEmitterEdited = true;
			params.angle.value = PI + HALF_PI;
			if (angleRangeOn)
				params.angle.variance = HALF_PI / 2.f;
		}
		ImGui::SameLine();
		ImGui::Text("Emit Angle presets");
		//Angle sliders
		if (angleRangeOn)
		{		
			float value1 = params.angle.value - params.angle.variance / 2.f;
			float value2 = params.angle.value + params.angle.variance / 2.f;
			if (ImGui::SliderRangeFloat("Emit angle", &value1, &value2, -TWO_PI, TWO_PI))
			{
				bEmitterEdited = true;
				float diff = neAbs(value2 - value1);
				params.angle.variance = diff;
				params.angle.value = ((value1 < value2) ? (value1) : (value2)) + diff / 2.f;
			}
		}
		else
		{
			if (ImGui::SliderFloat("Emit angle", &params.angle.value, 0.f, TWO_PI))
				bEmitterEdited = true;		
		}

		//Emitter type
		int selectedEmitterType = static_cast<int>(params.emitterType);
		if (ImGui::Combo("Emmitter type", &selectedEmitterType, emitterTypes, 2))
		{
			bEmitterEdited = true;

			params.emitterType = static_cast<ParticlesEmitterParameters::EmitterType>(selectedEmitterType);
		}

		//Gravity
		if (params.emitterType == ParticlesEmitterParameters::EmitterType::GRAVITY)
		{
			if(ImGui::DragVec2("Gravity", params.gravityParameters.gravity, -10000.f, 10000.f))
				bEmitterEdited = true;;

			ImGuiOptionalRangeFloat(
				"Speed",
				params.gravityParameters.startSpeed.variance != 0.f,
				&params.gravityParameters.startSpeed.value,
				&params.gravityParameters.startSpeed.variance,
				0.f, 200.f, true);	

			ImGuiOptionalRangeFloat(
				"Radial Acceleration",
				params.gravityParameters.radialAcceleration.variance != 0.f,
				&params.gravityParameters.radialAcceleration.value,
				&params.gravityParameters.radialAcceleration.variance,
				-200.f, 200.f, true);

			ImGuiOptionalRangeFloat(
				"Tangential Acceleration",
				params.gravityParameters.tangentialAcceleration.variance != 0.f,
				&params.gravityParameters.tangentialAcceleration.value,
				&params.gravityParameters.tangentialAcceleration.variance,
				-200.f, 200.f, true);

			if(ImGui::Checkbox("Rotation is Direction", &params.gravityParameters.rotationIsDir))
				bEmitterEdited = true;
		}
		//Radius
		else
		{
			ImGuiOptionalRangeFloat(
				"Start Radius",
				params.radiusParameters.minRadius.variance != 0.f,
				&params.radiusParameters.minRadius.value,
				&params.radiusParameters.minRadius.variance,
				-40.f, 40.f, true);

			ImGuiOptionalRangeFloat(
				"End Radius",
				params.radiusParameters.maxRadius.variance != 0.f,
				&params.radiusParameters.maxRadius.value,
				&params.radiusParameters.maxRadius.variance,
				-40.f, 40.f, true);

			ImGuiOptionalRangeFloat(
				"Rotate per Second",
				params.radiusParameters.rotatePerSecond.variance != 0.f,
				&params.radiusParameters.rotatePerSecond.value,
				&params.radiusParameters.rotatePerSecond.variance,
				-100.f, 100.f, true);
		}

		ImGui::TreePop();
	}
}

void ND_Util::particleSettingsDraw(ParticlesEmitterParameters& params)
{
	if (ImGui::TreeNode("Particle Settings"))
	{
		ImGuiOptionalRangeFloat(
			"Lifetime",
			params.particleLifeSpan.variance != 0.f,
			&params.particleLifeSpan.value,
			&params.particleLifeSpan.variance,
			0.001f, 30.f, true);

		ImGuiOptionalRangeFloat(
			"Start Size",
			params.startSize.variance != 0.f,
			&params.startSize.value,
			&params.startSize.variance,
			0.f, 20.f, true);

		ImGuiOptionalRangeFloat(
			"End Size",
			params.endSize.variance != 0.f,
			&params.endSize.value,
			&params.endSize.variance,
			0.f, 20.f, true);

		ImGuiOptionalRangeFloat(
			"Start Spin(Rotation)",
			params.startRotation.variance != 0.f,
			&params.startRotation.value,
			&params.startRotation.variance,
			0.f, 6.283, true);

		ImGuiOptionalRangeFloat(
			"End Spin(Rotation)",
			params.endRotation.variance != 0.f,
			&params.endRotation.value,
			&params.endRotation.variance,
			0.f, 6.283, true);

		ImGui::TreePop();
	}
}

void ND_Util::colorSettingsDraw(ParticlesEmitterParameters& params)
{
	if (ImGui::TreeNode("Particle Color Settings"))
	{
		if (ImGui::ColorEdit("Start Color", params.startColor.value))
			bEmitterEdited = true;
		if (ImGui::ColorEdit("Start Color Variance", params.startColor.variance))
			bEmitterEdited = true;
		if (ImGui::ColorEdit("End Color", params.endColor.value))
			bEmitterEdited = true;
		if (ImGui::ColorEdit("End Color Variance", params.endColor.variance))
			bEmitterEdited = true;
		if (ImGui::Auto("Blend Source", params.blendFunc.src))
			bEmitterEdited = true;
		if (ImGui::Auto("Blend Destination", params.blendFunc.dst))
			bEmitterEdited = true;

		ImGui::TreePop();
	}
}

void ND_Util::ImGuiOptionalRangeFloat(const char* label, bool rangedValue, float* value, float* variance, float minValue, float maxValue, bool useFloatInputForNonRange)
{
	if (!value || !variance)
		return;

	if (!rangeFloatSliderIds.contains(label))
		rangeFloatSliderIds.insert({ label , w4::utils::UIDGenerator::get()});
	ImGui::PushID(rangeFloatSliderIds[label]);

	//Range switch
	auto btnTexture = getBtnTexture(rangedValue ? BtnType::RANGE : BtnType::RANGE_OFF);
	if (ImGui::ImageButton(btnTexture, btnSize))
	{
		bEmitterEdited = true;

		if (rangedValue)
		{
			*variance = 0.f;
		}
		else
		{
			*variance = minValue + maxValue/10.f;
		}
	}
	ImGui::SameLine();

	//Sliders
	if (rangedValue)
	{
		float value1 = *value - *variance / 2.f;
		float value2 = *value + *variance / 2.f;
		if (value1 < minValue)
			value1 = minValue;
		if(value2 > maxValue)
			value2 = maxValue;

		if (ImGui::SliderRangeFloat(label, &value1, &value2, minValue, maxValue))
		{
			bEmitterEdited = true;
			float diff = neAbs(value2 - value1);
			*variance = diff;
			*value = ((value1 < value2) ? (value1) : (value2)) + diff / 2.f;
		}
	}
	else if (useFloatInputForNonRange)
	{
		if (ImGui::InputFloat(label, value, maxValue / 100.f, maxValue / 10.f))
			bEmitterEdited = true;
	}
	else
	{
		if (ImGui::SliderFloat(label, value, minValue, maxValue))
			bEmitterEdited = true;
	}

	ImGui::PopID();
}

#endif //ends #ifndef FORCENODEDEBUG_OFF - conditional compilation macro. If FORCENODEDEBUG_OFF is defined, NOCOMP macro provides empty implementation and saves some weight in build size