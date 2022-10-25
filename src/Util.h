#pragma once

namespace util
{
	class RNG
	{
	public:
		static RNG* GetSingleton()
		{
			static RNG singleton;
			return &singleton;
		}

		template <class T, class = std::enable_if_t<std::is_arithmetic_v<T>>>
		T Generate(T a_min, T a_max)
		{
			if constexpr (std::is_integral_v<T>) {
				std::uniform_int_distribution<T> distr(a_min, a_max);
				return distr(rng);
			} else {
				std::uniform_real_distribution<T> distr(a_min, a_max);
				return distr(rng);
			}
		}

		float Generate()
		{
			return XoshiroCpp::FloatFromBits(rng());
		}

	private:
		RNG() :
			rng(std::chrono::steady_clock::now().time_since_epoch().count())
		{}
		RNG(RNG const&) = delete;
		RNG(RNG&&) = delete;
		~RNG() = default;

		RNG& operator=(RNG const&) = delete;
		RNG& operator=(RNG&&) = delete;

		XoshiroCpp::Xoshiro128Plus rng;
	};

	inline std::pair<bool, float> point_in_water(const RE::NiPoint3& a_pos)
	{
	    for (const auto& waterObject : RE::TESWaterSystem::GetSingleton()->waterObjects) {
			if (waterObject) {
				for (const auto& bound : waterObject->multiBounds) {
					if (bound) {
						if (auto size{ bound->size }; size.z <= 10.0f) {  //avoid sloped water
							auto center{ bound->center };
							const auto boundMin = center - size;
							const auto boundMax = center + size;
							if (!(a_pos.x < boundMin.x || a_pos.x > boundMax.x || a_pos.y < boundMin.y || a_pos.y > boundMax.y)) {
								return { true, center.z };
							}
						}
					}
				}
			}
		}
		return { false, 0.0f };
	}
}

namespace RayCast
{
	using namespace util;

    struct Input
	{
		RE::NiPoint3 rayOrigin{};
	};

	struct Output
	{
		RE::NiPoint3 hitPos{};
		RE::NiMatrix3 normal{};
		bool hitActor{ false };
		bool hitWater{ false };
	};

	inline std::optional<RE::NiPoint3> GenerateRandomPointAroundPlayer(float a_radius, const RE::NiPoint3& a_posIn, bool a_inPlayerFOV)
	{
		const auto r = a_radius * std::sqrtf(RNG::GetSingleton()->Generate());
		const auto theta = RNG::GetSingleton()->Generate() * RE::NI_TWO_PI;

		const RE::NiPoint3 randPoint{
			a_posIn.x + r * std::cosf(theta),
			a_posIn.y + r * std::sinf(theta),
			a_posIn.z
		};

		if (!a_inPlayerFOV || RE::NiCamera::PointInFrustum(randPoint, RE::Main::WorldRootCamera(), 32.0f)) {
			return randPoint;
		}

		return std::nullopt;
	}

	inline std::optional<Output> GenerateRayCast(RE::TESObjectCELL* a_cell, const Input& a_input)
	{
		if (!a_cell || a_cell != RE::PlayerCharacter::GetSingleton()->GetParentCell()) {
			return std::nullopt;
		}

		const auto bhkWorld = a_cell->GetbhkWorld();
		if (!bhkWorld) {
			return std::nullopt;
		}

		RE::NiPoint3 rayStart = a_input.rayOrigin;
		RE::NiPoint3 rayEnd = a_input.rayOrigin;

		constexpr auto height = 9999.0f;

		rayStart.z += height;
		rayEnd.z -= height;

		RE::bhkPickData pickData;

		const auto havokWorldScale = RE::bhkWorld::GetWorldScale();
		pickData.rayInput.from = rayStart * havokWorldScale;
		pickData.rayInput.to = rayEnd * havokWorldScale;
		pickData.rayInput.enableShapeCollectionFilter = false;
		pickData.rayInput.filterInfo = RE::bhkCollisionFilter::GetSingleton()->GetNewSystemGroup() << 16 | stl::to_underlying(RE::COL_LAYER::kLOS);

		if (bhkWorld->PickObject(pickData); pickData.rayOutput.HasHit()) {
			Output output;

			const auto distance = rayEnd - rayStart;
			output.hitPos = rayStart + (distance * pickData.rayOutput.hitFraction);

			output.normal.SetEulerAnglesXYZ({ -0, -0, RNG::GetSingleton()->Generate(-RE::NI_PI, RE::NI_PI) });

			switch (static_cast<RE::COL_LAYER>(pickData.rayOutput.rootCollidable->broadPhaseHandle.collisionFilterInfo & 0x7F)) {
			case RE::COL_LAYER::kCharController:
			case RE::COL_LAYER::kBiped:
			case RE::COL_LAYER::kDeadBip:
			case RE::COL_LAYER::kBipedNoCC:
				output.hitActor = true;
				break;
			default:
				{
					if (auto [inWater, waterHeight] = point_in_water(output.hitPos); inWater && waterHeight > output.hitPos.z) {
						output.hitWater = true;
						output.hitPos.z = waterHeight;
					}
				}
				break;
			}

			return output;
		}

		return std::nullopt;
	}
}

namespace Ripples
{
	struct Static
	{
		static inline bool showProceduralWater = false;

		static void ToggleWaterRipples(RE::TESWaterSystem* a_waterSystem, bool a_enabled, float a_fadeAmount)
		{
			float fadeAmount = a_fadeAmount;
			if (a_enabled && a_fadeAmount > 0.0f) {
				showProceduralWater = true;
			} else {
				if (!showProceduralWater) {
					return;
				}
				showProceduralWater = false;
				fadeAmount = 0.0f;
			}
			for (auto& waterObject : a_waterSystem->waterObjects) {
				if (waterObject) {
					if (const auto& rippleObject = waterObject->waterRippleObject; rippleObject) {
						if (a_enabled) {
							rippleObject->SetAppCulled(false);
						} else {
							rippleObject->SetAppCulled(true);
						}

						RE::BSVisit::TraverseScenegraphGeometries(rippleObject.get(), [&](RE::BSGeometry* a_geometry) -> RE::BSVisit::BSVisitControl {
							using State = RE::BSGeometry::States;
							using Feature = RE::BSShaderMaterial::Feature;

							if (const auto effect = a_geometry->properties[State::kEffect].get()) {
								if (const auto effectShaderProp = netimmerse_cast<RE::BSEffectShaderProperty*>(effect)) {
									if (const auto material = static_cast<RE::BSEffectShaderMaterial*>(effectShaderProp->material)) {
										material->baseColor.alpha = fadeAmount;
									}
								}
							}

							return RE::BSVisit::BSVisitControl::kContinue;
						});
					}
				}
			}
		}
	};

	struct Dynamic
	{
		static inline float rippleTimer = 0.0f;
		static constexpr float rippleDelay = 0.01f;

		static void ToggleWaterRipples(Rain* a_rain, RE::TESWaterSystem* a_waterSystem)
		{
			const auto settings = Settings::Manager::GetSingleton();

			rippleTimer += RE::GetSecondsSinceLastFrame();

			if (rippleTimer > rippleDelay) {
				rippleTimer = 0.0f;

				const auto player = RE::PlayerCharacter::GetSingleton();
				const auto cell = player->GetParentCell();
				if (!cell) {
					return;
				}

				const auto playerPos = RE::PlayerCharacter::GetSingleton()->GetPosition();

				const auto rayCastRadius = a_rain->ripple.rayCastRadius;
				const auto rayCastIterations = a_rain->ripple.rayCastIterations;

				const auto enableDebugMarker = settings->enableDebugMarkerRipple;

				for (std::size_t i = 0; i < rayCastIterations; i++) {
					SKSE::GetTaskInterface()->AddTask([=] {
						const RayCast::Input rayCastInput{
							*RayCast::GenerateRandomPointAroundPlayer(rayCastRadius, playerPos, false),
						};
						if (const auto rayCastOutput = GenerateRayCast(cell, rayCastInput); rayCastOutput) {
							if (rayCastOutput->hitWater) {
								if (!enableDebugMarker) {
									a_waterSystem->AddRipple(rayCastOutput->hitPos, a_rain->ripple.rippleDisplacementAmount * 0.0099999998f);
								} else {
									RE::BSTempEffectParticle::Spawn(cell, 1.6f, "MarkerX.nif", rayCastOutput->normal, rayCastOutput->hitPos, 0.5f, 7, nullptr);
								}
							}
						}
					});
				}
			}
		}
	};
}
