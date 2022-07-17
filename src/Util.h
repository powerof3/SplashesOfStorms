#pragma once

namespace Timer
{
	inline float GetSecondsSinceLastFrame()
	{
		REL::Relocation<float*> timer{ RELOCATION_ID(523660, 410199) };
		return *timer;
	}
}

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
	struct Input
	{
		RE::NiPoint3 rayOrigin{};
		float height{ 0.0f };
		float waterHeight{ 0.0f };
	};

	struct Output
	{
		RE::NiPoint3 hitPos{};
		RE::NiPoint3 normal{};
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

		if (const auto worldCam = RE::Main::WorldRootCamera(); !a_inPlayerFOV || Camera::PointInFrustum(randPoint, worldCam, 32.0f)) {
			return randPoint;
		}

		return std::nullopt;
	}

	inline std::optional<Output> GenerateRayCast(RE::bhkWorld* a_havokWorld, const Input& a_input)
	{
		RE::NiPoint3 rayStart = a_input.rayOrigin;
		rayStart.z = a_input.height;
		RE::NiPoint3 rayEnd = rayStart;
		rayEnd.z = -a_input.height;

		RE::bhkPickData pickData;

		auto havokWorldScale = RE::bhkWorld::GetWorldScale();
		pickData.rayInput.from = rayStart * havokWorldScale;
		pickData.rayInput.to = rayEnd * havokWorldScale;
		pickData.rayInput.enableShapeCollectionFilter = false;
		pickData.rayInput.filterInfo = RE::bhkCollisionFilter::GetSingleton()->GetNewSystemGroup() << 16 | stl::to_underlying(RE::COL_LAYER::kLOS);

		if (a_havokWorld->PickObject(pickData); pickData.rayOutput.HasHit()) {
			Output output;

			const auto distance = rayEnd - rayStart;
			output.hitPos = rayStart + (distance * pickData.rayOutput.hitFraction);

			output.normal = { -0, -0, rng::GetSingleton()->Generate(-RE::NI_PI, RE::NI_PI) };

			if (a_input.waterHeight >= output.hitPos.z) {
				output.hitWater = true;
				output.hitPos.z = a_input.waterHeight;
			}

			auto collidingLayer = static_cast<RE::COL_LAYER>(pickData.rayOutput.rootCollidable->broadPhaseHandle.collisionFilterInfo & 0x7F);
			if (stl::is_in(collidingLayer, RE::COL_LAYER::kCharController, RE::COL_LAYER::kBiped, RE::COL_LAYER::kDeadBip)) {
				output.hitActor = true;
			}

			return output;
		}

		return std::nullopt;
	}
}
