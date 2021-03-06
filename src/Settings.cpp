#include "Settings.h"

void RainObject::LoadSettings(const toml::node_view<const toml::node>& a_node)
{
	get_value(enabled, a_node, "Enabled"sv);
	get_value(rayCastRadius, a_node, "RaycastRadius"sv);
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

	get_value(rayCastIterations, a_node, "RaycastIterations"sv);
	get_value(rippleDisplacementAmount, a_node, "RippleDisplacementMult"sv);
}

void Rain::LoadSettings(const toml::table& a_tbl, TYPE a_type, std::string_view a_section)
{
	type = a_type;

	splash.LoadSettings(a_tbl[a_section]);
	ripple.LoadSettings(a_tbl[a_section]);
}

bool Settings::LoadSettings()
{
	try {
		toml::table tbl = toml::parse_file(fmt::format("Data/SKSE/Plugins/{}.toml", Version::PROJECT));

		rayCastHeight = tbl["Settings"]["RaycastHeight"].value_or(rayCastHeight);

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
	}

	return true;
}

Rain* Settings::GetRainType()
{
	const auto sky = RE::Sky::GetSingleton();
	const auto currentWeather = sky->currentWeather;

	if (cache.weather == nullptr || cache.weather != currentWeather) {
		if (cache.weather = currentWeather; cache.weather) {
			if (const auto precip = cache.weather->precipitationData; precip && precip->data.size() >= 12) {
				switch (static_cast<std::uint32_t>(precip->data[11].f)) {
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
