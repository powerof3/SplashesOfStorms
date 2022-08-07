#pragma once

namespace Camera
{
	inline bool PointInFrustum(const RE::NiPoint3& a_point, RE::NiCamera* a_camera, float a_radius)
	{
		using func_t = decltype(&PointInFrustum);
		REL::Relocation<func_t> func{ RELOCATION_ID(15672, 15900) };
		return func(a_point, a_camera, a_radius);
	}
}

namespace RayCast
{
	namespace
	{
		std::pair<bool, float> get_within_water_bounds(const RE::NiPoint3& a_pos)
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

	struct Input
	{
		RE::NiPoint3 rayOrigin{};
		float height{ 0.0f };
		std::uint32_t collisionLayer{ 0 };
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
		const auto r = a_radius * std::sqrtf(rng::GetSingleton()->Generate(0.0f, 1.0f));
		const auto theta = rng::GetSingleton()->Generate(0.0f, 1.0f) * RE::NI_TWO_PI;

		const RE::NiPoint3 randPoint{
			a_posIn.x + r * std::cosf(theta),
			a_posIn.y + r * std::sinf(theta),
			a_posIn.z
		};

		const auto worldCam = RE::Main::WorldRootCamera();
		if (!a_inPlayerFOV || !worldCam || Camera::PointInFrustum(randPoint, worldCam, 32.0f)) {
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

		rayStart.z += a_input.height;
		rayEnd.z -= a_input.height;

		RE::bhkPickData pickData;

		auto havokWorldScale = RE::bhkWorld::GetWorldScale();
		pickData.rayInput.from = rayStart * havokWorldScale;
		pickData.rayInput.to = rayEnd * havokWorldScale;
		pickData.rayInput.enableShapeCollectionFilter = false;
		pickData.rayInput.filterInfo = RE::bhkCollisionFilter::GetSingleton()->GetNewSystemGroup() << 16 | a_input.collisionLayer;

		if (bhkWorld->PickObject(pickData); pickData.rayOutput.HasHit()) {
			Output output;

			const auto distance = rayEnd - rayStart;
			output.hitPos = rayStart + (distance * pickData.rayOutput.hitFraction);

			output.normal.SetEulerAnglesXYZ({ -0, -0, rng::GetSingleton()->Generate(-RE::NI_PI, RE::NI_PI) });

			if (auto [inWater, waterHeight] = get_within_water_bounds(output.hitPos); inWater && waterHeight > output.hitPos.z) {
				output.hitWater = true;
				output.hitPos.z = waterHeight;
			}

			switch (static_cast<RE::COL_LAYER>(pickData.rayOutput.rootCollidable->broadPhaseHandle.collisionFilterInfo & 0x7F)) {
			case RE::COL_LAYER::kCharController:
			case RE::COL_LAYER::kBiped:
			case RE::COL_LAYER::kDeadBip:
			case RE::COL_LAYER::kBipedNoCC:
				output.hitActor = true;
				break;
			default:
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

		static void ToggleWaterRipples(RE::TESWaterSystem* a_waterSystem, bool a_enabled, float a_fadeAmount)
		{
			if (a_enabled && a_fadeAmount > 0.0f) {
				if (RE::Sky::GetSingleton()->mode.none(RE::Sky::Mode::kFull)) {
					return;
				}

				const auto settings = Settings::Manager::GetSingleton();
				const auto rain = settings->GetRainType();

				rippleTimer += RE::GetSecondsSinceLastFrame();

				if (rippleTimer > rain->ripple.delay) {
					rippleTimer = 0.0f;

					const auto player = RE::PlayerCharacter::GetSingleton();
					const auto cell = player->GetParentCell();
					if (!cell) {
						return;
					}

					if (!settings->disableRipplesAtFastSpeed || player->DoGetMovementSpeed() <= 120.0f) {  //walking speed
						const auto playerPos = RE::PlayerCharacter::GetSingleton()->GetPosition();
						const auto radius = rain->ripple.rayCastRadius;

						bool enableDebugMarker = settings->enableDebugMarkerRipple;

						for (std::uint32_t i = 0; i < rain->ripple.rayCastIterations; i++) {
							SKSE::GetTaskInterface()->AddTask([=] {
								const RayCast::Input rayCastInput{
									*RayCast::GenerateRandomPointAroundPlayer(radius, playerPos, false),
									settings->rayCastHeight,
									settings->colLayerRipple
								};

								if (const auto rayCastOutput = GenerateRayCast(cell, rayCastInput); rayCastOutput) {
									if (rayCastOutput->hitWater) {
										if (!enableDebugMarker) {
											a_waterSystem->AddRipple(rayCastOutput->hitPos, rain->ripple.rippleDisplacementAmount * 0.0099999998f);
										} else {
											RE::BSTempEffectParticle::Spawn(cell, 1.6f, "MarkerX.nif", rayCastOutput->normal, rayCastOutput->hitPos, 0.5f, 7, nullptr);
										}
									}
								}
							});
						}
					}
				}
			}
		}
	};
}
