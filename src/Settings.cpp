#include "Settings.h"

void RainObject::LoadSettings(const toml::node_view<const toml::node>& a_node)
{
	get_value(enabled, a_node, "Enabled"sv);
	get_value(rayCastRadius, a_node, "RaycastRadius"sv);
	get_value(rayCastIterations, a_node, "RaycastIterations"sv);
}

void Splash::LoadSettings(const toml::node_view<const toml::node>& a_node)
{
	RainObject::LoadSettings(a_node);

	get_value(nif, a_node, "NifPath"sv);
	get_value(nifActor, a_node, "NifPathActor"sv);
	get_value(nifScale, a_node, "NifScale"sv);
	get_value(nifScaleActor, a_node, "NifScaleActor"sv);
}

void Ripple::LoadSettings(const toml::node_view<const toml::node>& a_node)
{
	RainObject::LoadSettings(a_node);

    get_value(rippleDisplacementAmount, a_node, "RippleDisplacementMult"sv);
}

void Rain::LoadSettings(const toml::table& a_tbl, TYPE a_type, std::string_view a_section)
{
	type = a_type;

	splash.LoadSettings(a_tbl[a_section]);
	ripple.LoadSettings(a_tbl[a_section]);
}

namespace Settings
{
	bool Manager::LoadSettings()
	{
		try {
			toml::table tbl = toml::parse_file(fmt::format("Data/SKSE/Plugins/{}.toml", Version::PROJECT));

			const auto& settings = tbl["Settings"];
			enableDebugMarkerSplash = settings["DebugSplashes"].value_or(enableDebugMarkerSplash);
			enableDebugMarkerRipple = settings["DebugRipples"].value_or(enableDebugMarkerRipple);

			light.LoadSettings(tbl, Rain::TYPE::kLight, "LightRain");
			medium.LoadSettings(tbl, Rain::TYPE::kMedium, "MediumRain");
			heavy.LoadSettings(tbl, Rain::TYPE::kHeavy, "HeavyRain");

		} catch (const toml::parse_error& e) {
			std::ostringstream ss;
			ss
				<< "Error parsing file \'" << *e.source().path << "\':\n"
				<< '\t' << e.description() << '\n'
				<< "\t\t(" << e.source().begin << ')';
			logger::error("{}", ss.str());

			return false;
		} catch (const std::exception& e) {
			logger::error("{}", e.what());

			return false;
		}

		logger::info("Success");

		return true;
	}

	Rain* Manager::GetRain(float a_particleDensity)
	{
		if (a_particleDensity < 5.0f) {
			currentRainType = Rain::TYPE::kLight;
		} else if (a_particleDensity >= 5.0f && a_particleDensity < 9.0f) {
			currentRainType = Rain::TYPE::kMedium;
		} else if (a_particleDensity >= 9.0f) {
			currentRainType = Rain::TYPE::kHeavy;
		}

		switch (currentRainType) {
		case Rain::TYPE::kLight:
			return &light;
		case Rain::TYPE::kMedium:
			return &medium;
		case Rain::TYPE::kHeavy:
			return &heavy;
		default:
			return nullptr;
		}
	}

	Rain* Manager::GetRain()
	{
		switch (currentRainType) {
		case Rain::TYPE::kLight:
			return &light;
		case Rain::TYPE::kMedium:
			return &medium;
		case Rain::TYPE::kHeavy:
			return &heavy;
		default:
			return nullptr;
		}
	}

    Rain::TYPE Manager::GetRainType() const
    {
		return currentRainType;
    }

    void Manager::SetRainType(Rain::TYPE a_type)
	{
		currentRainType = a_type;
	}
}
