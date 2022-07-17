#include "Debug.h"
#include "Settings.h"

namespace Debug
{
	namespace detail
	{
		constexpr auto LONG_NAME = "ReloadRain"sv;
		constexpr auto SHORT_NAME = "rain"sv;

		[[nodiscard]] const std::string& HelpString()
		{
			static auto help = []() {
				std::string buf;
				buf += "\"Reload Rain Splashes settings\" <id>";
				buf += "\n\t<id> ::= <integer>";
				return buf;
			}();
			return help;
		}

		bool Execute(const RE::SCRIPT_PARAMETER*, RE::SCRIPT_FUNCTION::ScriptData* a_scriptData, RE::TESObjectREFR*, RE::TESObjectREFR*, RE::Script*, RE::ScriptLocals*, double&, std::uint32_t&)
		{
			Settings::GetSingleton()->LoadSettings();

			if (const auto sky = RE::Sky::GetSingleton(); sky && !sky->overrideWeather) {
				std::string weather;
				switch (a_scriptData->GetIntegerChunk()->GetInteger()) {
				case 0:
					weather = "TestCloudyRain ";
					break;
				case 1:
					weather = "SkyrimOvercastRain";
					break;
				case 2:
					weather = "SkyrimStormRain";
					break;
				case 3:
					weather = "SkyrimClear";
					break;
				default:
					break;
				}
				if (!weather.empty()) {
					RE::TaskQueueInterface::GetSingleton()->QueueForceWeather(RE::TESForm::LookupByEditorID<RE::TESWeather>(weather), false);
				}
			}

			if (const auto console = RE::ConsoleLog::GetSingleton(); console && console->IsConsoleMode()) {
				console->Print("Rain Splashes settings reloaded");
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
