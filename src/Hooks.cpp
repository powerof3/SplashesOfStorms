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
			const auto rain = settings->GetRainType();

			if (!rain || !rain->ripple.enabled) {
				return func(a_waterSystem, a_enabled, a_fadeAmount);
			}

			return Dynamic::ToggleWaterRipples(a_waterSystem, a_enabled, a_fadeAmount);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct TraverseSceneGraphObjects_AddWaterRecurse
	{
		struct DoAddWater
		{
			RE::TESWaterForm* waterForm;                                         // 00
			float waterHeight;                                                   // 08
			std::uint32_t pad0C;                                                 // 0C
			RE::BSTArray<RE::NiPointer<RE::BSMultiBoundAABB>>* multiboundArray;  // 10
			bool noDisplacement;                                                 // 18
			bool isProceduralWater;                                              // 19
		};
		static_assert(sizeof(DoAddWater) == 0x20);

		static RE::BSVisit::BSVisitControl thunk(RE::NiAVObject* a_object, DoAddWater& a_visitor)
		{
			return func(a_object, a_visitor);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(25638, 26179), OFFSET(0x238, 0x223) };  //Precipitation::Update
		stl::write_thunk_call<ToggleWaterSplashes>(target.address());

		//REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(31387, 32178), 0x36 };  //TESWaterSystem::AddWaterRecurse
		//stl::write_thunk_call<TraverseSceneGraphObjects_AddWaterRecurse>(target.address());

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

			if (const auto sky = RE::Sky::GetSingleton(); sky->mode.all(RE::Sky::Mode::kFull) && sky->IsRaining()) {
				const auto settings = Settings::Manager::GetSingleton();
				const auto rain = settings->GetRainType();

				if (!rain || !rain->splash.enabled) {
					return;
				}

				splashTimer += Timer::GetSecondsSinceLastFrame();

				if (splashTimer > rain->splash.delay) {
					splashTimer = 0.0f;

					const auto player = RE::PlayerCharacter::GetSingleton();
					const auto cell = player->GetParentCell();
					if (!cell) {
						return;
					}

					const auto bhkWorld = cell->GetbhkWorld();
					if (!bhkWorld) {
						return;
					}

					auto playerPos = player->GetPosition();
					playerPos.z += player->GetHeight();

					if (const auto rayOrigin = RayCast::GenerateRandomPointAroundPlayer(rain->splash.rayCastRadius, playerPos, false); rayOrigin) {
						SKSE::GetTaskInterface()->AddTask([=] {
							const RayCast::Input rayCastInput{
								*rayOrigin,
								settings->rayCastHeight,
								settings->colLayerSplash
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
