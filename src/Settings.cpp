#include "Settings.h"

namespace Settings
{
	void RainObject::LoadSettings(const toml::node_view<const toml::node>& a_node)
	{
		get_value(enabled, a_node, "Enabled"sv);
		get_value(rayCastRadius, a_node, "RaycastRadius"sv);
		get_value(rayCastIterations, a_node, "RaycastIterations"sv);
		get_value(delay, a_node, "SpawnDelay"sv);
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

	bool Manager::LoadSettings()
	{
		try {
			toml::table tbl = toml::parse_file(fmt::format("Data/SKSE/Plugins/{}.toml", Version::PROJECT));

			const auto& settings = tbl["Settings"];
			rayCastHeight = settings["RaycastHeight"].value_or(rayCastHeight);
			colLayerSplash = settings["CollisionLayerSplashes"].value_or(colLayerSplash);
			colLayerRipple = settings["CollisionLayerRipples"].value_or(colLayerRipple);
			disableRipplesAtFastSpeed = settings["DisableRipplesAtFastSpeeds"].value_or(disableRipplesAtFastSpeed);

			light.LoadSettings(tbl, Rain::TYPE::kLight, "LightRain");
			medium.LoadSettings(tbl, Rain::TYPE::kMedium, "MediumRain");
			heavy.LoadSettings(tbl, Rain::TYPE::kHeavy, "HeavyRain");

		} catch (const toml::parse_error& e) {
			std::ostringstream ss;
			ss
				<< "Error parsing file \'" << *e.source().path << "\':\n"
				<< '\t' << e.description() << '\n'
				<< "\t\t(" << e.source().begin << ')';
			logger::error(FMT_STRING("{}"sv), ss.str());

			return false;
		} catch (const std::exception& e) {
			logger::error("{}", e.what());

			return false;
		}

		logger::info("Success");

		return true;
	}

	Rain* Manager::GetRainType()
	{
		const auto sky = RE::Sky::GetSingleton();
		const auto currentWeather = sky->currentWeather;

		if (cache.weather == nullptr || cache.rainType == Rain::TYPE::kNone || cache.weather != currentWeather) {
			if (cache.weather = currentWeather; cache.weather) {
				if (const auto precip = cache.weather->precipitationData; precip && precip->data.size() >= 12) {
					switch (static_cast<std::uint32_t>(precip->data[11].f)) {  //particle density
					case 1:
					case 2:
					case 3:
					case 4:
						cache.rainType = Rain::TYPE::kLight;
						break;
					case 5:
					case 6:
					case 7:
						cache.rainType = Rain::TYPE::kMedium;
						break;
					case 8:
					case 9:
					case 10:
						cache.rainType = Rain::TYPE::kHeavy;
						break;
					default:
						cache.rainType = Rain::TYPE::kNone;
						break;
					}
				}
			}
		}

		switch (cache.rainType) {
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
}
