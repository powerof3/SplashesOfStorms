#include "Hooks.h"
#include "Settings.h"
#include "Util.h"

namespace Ripples
{
	struct ToggleWaterSplashes
	{
		static void thunk(RE::TESWaterSystem* a_waterSystem, bool a_enabled, float a_fadeAmount)
		{
			const auto settings = Settings::Manager::GetSingleton();
			const auto rain = settings->GetRain();

			if (!rain) {
				return func(a_waterSystem, false, 0.0f);
			}

			if (!rain->ripple.enabled) {
				return func(a_waterSystem, a_enabled, a_fadeAmount);
			}

			if (a_enabled && a_fadeAmount > 0.0f) {
				Dynamic::ToggleWaterRipples(rain, a_waterSystem);
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(25638, 26179), OFFSET(0x238, 0x223) };  //Precipitation::Update
		stl::write_thunk_call<ToggleWaterSplashes>(target.address());

		logger::info("installed ripples hook");
	}
}

namespace Splashes
{
	inline float splashTimer = 0.0f;
	constexpr float splashDelay = 0.01f;

	struct UpdateShaderGeometry
	{
		static void thunk(RE::BSGeometry* a_precipGeometry, RE::NiCamera* a_camera, float a_delta, float a_cubeSize, float a_particleDensity, float a_windSpeed, float a_windAngle)
		{
			func(a_precipGeometry, a_camera, a_delta, a_cubeSize, a_particleDensity, a_windSpeed, a_windAngle);

			const auto settings = Settings::Manager::GetSingleton();

			if (!a_precipGeometry || a_particleDensity < 1.0f) {
				settings->SetRainType(Rain::TYPE::kNone);
				return;
			}

			const auto& effect = a_precipGeometry->properties[RE::BSGeometry::States::kEffect];
			const auto particleShader = netimmerse_cast<RE::BSParticleShaderProperty*>(effect.get());
			const auto particleEmitter = particleShader ? particleShader->particleEmitter : nullptr;

			if (!particleEmitter || particleEmitter->emitterType != RE::BSParticleShaderEmitter::EMITTER_TYPE::kRain) {
				settings->SetRainType(Rain::TYPE::kInvalid);
				return;
			}

			const auto rain = settings->GetRain(a_particleDensity);

			if (!rain || !rain->splash.enabled) {
				return;
			}

			splashTimer += RE::GetSecondsSinceLastFrame();

			if (splashTimer > splashDelay) {
				splashTimer = 0.0f;

				const auto player = RE::PlayerCharacter::GetSingleton();
				const auto cell = player->GetParentCell();
				if (!cell) {
					return;
				}

				const auto playerPos = player->GetPosition();

				const auto enableDebugMarker = settings->enableDebugMarkerSplash;
				const auto rayCastRadius = rain->splash.rayCastRadius;
				const auto rayCastIterations = rain->splash.rayCastIterations;

				for (std::size_t i = 0; i < rayCastIterations; i++) {
					if (const auto rayOrigin = RayCast::GenerateRandomPointAroundPlayer(rayCastRadius, playerPos, true); rayOrigin) {
						SKSE::GetTaskInterface()->AddTask([=] {
							if (const auto rayCastOutput = RayCast::GenerateRayCast(cell, { *rayOrigin }); rayCastOutput && !rayCastOutput->hitWater) {
								if (!enableDebugMarker) {
									const auto& model = rayCastOutput->hitActor ? rain->splash.nifActor : rain->splash.nif;
									const float scale = rayCastOutput->hitActor ? rain->splash.nifScaleActor : rain->splash.nifScale;
									RE::BSTempEffectParticle::Spawn(cell, 1.6f, model.c_str(), rayCastOutput->normal, rayCastOutput->hitPos, scale, 7, nullptr);
								} else {
									RE::BSTempEffectParticle::Spawn(cell, 1.6f, "MarkerX.nif", rayCastOutput->normal, rayCastOutput->hitPos, 0.5f, 7, nullptr);
								}
							}
						});
					}
				}
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(25671, 26213), OFFSET(0xC5, 0xCF) };
		stl::write_thunk_call<UpdateShaderGeometry>(target.address());

		logger::info("installed splashes hook");
	}
}

namespace Hooks
{
	void Install()
	{
		Ripples::Install();
		Splashes::Install();
	}
}
