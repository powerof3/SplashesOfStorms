#include "Debug.h"
#include "Settings.h"

namespace Debug
{
	namespace detail
	{
		constexpr auto LONG_NAME = "SplashesReload"sv;
		constexpr auto SHORT_NAME = "splashes"sv;

		[[nodiscard]] const std::string& HelpString()
		{
			static auto help = []() {
				std::string buf;
				buf += "Reload Splashes of Storms settings from config\n";
				buf += R"(<id> : 0 - clear weather | 1 - light rain | 2 -  medium rain | 3 - heavy rain | 4 - blizzard )";
				return buf;
			}();
			return help;
		}

		bool Execute(const RE::SCRIPT_PARAMETER*, RE::SCRIPT_FUNCTION::ScriptData* a_scriptData, RE::TESObjectREFR*, RE::TESObjectREFR*, RE::Script*, RE::ScriptLocals*, double&, std::uint32_t&)
		{
			constexpr auto print = [](const char* a_fmt) {
				if (RE::ConsoleLog::IsConsoleMode()) {
					RE::ConsoleLog::GetSingleton()->Print(a_fmt);
				}
			};

			print("[Splashes of Storms] Reloading settings..");

			logger::info("******************************");
			logger::info("Reloading settings..");

			if (Settings::Manager::GetSingleton()->LoadSettings()) {
				std::string weather;
				switch (a_scriptData->GetIntegerChunk()->GetInteger()) {
				case 0:
					weather = "SkyrimClear";
					break;
				case 1:
					weather = "TestCloudyRain";
					break;
				case 2:
					weather = "SkyrimOvercastRain";
					break;
				case 3:
					weather = "SkyrimStormRain";
					break;
				case 4:
					weather = "SkyrimStormSnow";
					break;
				default:
					break;
				}
				if (!weather.empty()) {
					RE::TaskQueueInterface::GetSingleton()->QueueForceWeather(RE::TESForm::LookupByEditorID<RE::TESWeather>(weather), false);
					print(fmt::format("[Splashes of Storms] Success! Weather forced to {}\n", weather).c_str());
				} else {
					print("[Splashes of Storms] Success!\n");
				}
			} else {
				print("[Splashes of Storms] Fail, errors detected in config! Check po3_SplashesOfStorms.log for more info");
				logger::error("Failed to load settings from .toml config!\n");
			}

			return true;
		}
	}

	void Install()
	{
		if (const auto function = RE::SCRIPT_FUNCTION::LocateConsoleCommand("CheckMemory"); function) {
			static RE::SCRIPT_PARAMETER params[] = {
				{ "Integer", RE::SCRIPT_PARAM_TYPE::kInt, true }
			};

			function->functionName = detail::LONG_NAME.data();
			function->shortName = detail::SHORT_NAME.data();
			function->helpString = detail::HelpString().data();
			function->referenceFunction = false;
			function->SetParameters(params);
			function->executeFunction = &detail::Execute;
			function->conditionFunction = nullptr;

			logger::debug("installed {}", detail::LONG_NAME);
		}
	}
}
