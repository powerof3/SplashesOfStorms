#include "Hooks.h"
#include "Settings.h"
#include "Util.h"

namespace Ripples
{
	static inline float rippleTimer = 0.0f;

	struct ToggleWaterSplashes
	{
		static void thunk(RE::TESWaterSystem* a_waterSystem, bool a_enabled, float a_arg3)
		{
			const auto settings = Settings::GetSingleton();
			const auto rain = settings->GetRainType();

			if (!rain || !rain->ripple.enabled) {
				return func(a_waterSystem, a_enabled, a_arg3);
			}

			if (a_enabled && a_arg3 > 0.0f) {
				rippleTimer += Timer::GetSecondsSinceLastFrame();

				if (rippleTimer > rain->ripple.delay) {
					rippleTimer = 0.0f;

					const auto cell = RE::PlayerCharacter::GetSingleton()->GetParentCell();
					if (!cell) {
						return;
					}

					const auto bhkWorld = cell->GetbhkWorld();
					if (!bhkWorld) {
						return;
					}

					const auto playerPos = RE::PlayerCharacter::GetSingleton()->GetPosition();
					const auto radius = rain->ripple.rayCastRadius;

					const auto halfIter = rain->ripple.rayCastIterations / 2;
					const auto halfRadius = radius * 0.5f;

					for (std::uint32_t i = 0; i < halfIter; i++) {
						SKSE::GetTaskInterface()->AddTask([=] {
							const RayCast::Input rayCastInput{
								*RayCast::GenerateRandomPointAroundPlayer(radius, playerPos, false),
								settings->rayCastHeight,
								RE::TES::GetSingleton()->GetWaterHeight(playerPos, cell)
							};

							if (const auto rayCastOutput = GenerateRayCast(bhkWorld, rayCastInput); rayCastOutput && rayCastOutput->hitWater) {
								//generate points around raycast hit position to spawn more ripples
								for (std::uint32_t k = 0; k < halfIter; k++) {
									auto newHitPos = RayCast::GenerateRandomPointAroundPlayer(halfRadius, rayCastOutput->hitPos, false);
									a_waterSystem->AddRipple(*newHitPos, rain->ripple.rippleDisplacementAmount * 0.01f);
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
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(25638, 26179), OFFSET(0x238, 0x223) };  //Precipitation::Update
		stl::write_thunk_call<ToggleWaterSplashes>(target.address());

		logger::info("installed ripples");
	}
}

namespace Splashes
{
	static inline float splashTimer = 0.0f;

	struct Precipitation_Update
	{
		static void thunk(RE::Precipitation* a_precip)
		{
			func(a_precip);

			if (RE::Sky::GetSingleton()->IsRaining()) {
				const auto settings = Settings::GetSingleton();
				const auto rain = settings->GetRainType();
				if (!rain || !rain->splash.enabled) {
					return;
				}

				splashTimer += Timer::GetSecondsSinceLastFrame();

				if (splashTimer > rain->splash.delay) {
					splashTimer = 0.0f;

					const auto cell = RE::PlayerCharacter::GetSingleton()->GetParentCell();
					if (!cell) {
						return;
					}

					const auto bhkWorld = cell->GetbhkWorld();
					if (!bhkWorld) {
						return;
					}

					const auto playerPos = RE::PlayerCharacter::GetSingleton()->GetPosition();

					if (const auto rayOrigin = RayCast::GenerateRandomPointAroundPlayer(rain->splash.rayCastRadius, playerPos, true); rayOrigin) {
						SKSE::GetTaskInterface()->AddTask([=] {
							const RayCast::Input rayCastInput{
								*rayOrigin,
								settings->rayCastHeight,
								RE::TES::GetSingleton()->GetWaterHeight(playerPos, cell)
							};

							if (const auto rayCastOutput = GenerateRayCast(bhkWorld, rayCastInput); rayCastOutput && !rayCastOutput->hitWater) {
								const auto& model = !rayCastOutput->hitActor ? rain->splash.nif : rain->splash.nifActor;
								const float scale = !rayCastOutput->hitActor ? rain->splash.nifScale : rain->splash.nifScaleActor;

								RE::BSTempEffectParticle::Spawn(cell, 1.6f, model.c_str(), rayCastOutput->normal, rayCastOutput->hitPos, scale, 7, nullptr);
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
		std::array targets{
			std::make_pair(RELOCATION_ID(25682, 26229), OFFSET(0x463, 0x766)),  //Sky::Update
			std::make_pair(RELOCATION_ID(25679, 26222), OFFSET(0xAA, 0x128)),   //Sky::SetMode
		};

		for (const auto& [id, offset] : targets) {
			REL::Relocation<std::uintptr_t> target{ id, offset };
			stl::write_thunk_call<Precipitation_Update>(target.address());
		}

		logger::info("installed splashes");
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
