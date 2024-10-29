#pragma once


#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include "toml++/toml.hpp"

#include <spdlog/sinks/basic_file_sink.h>

#include "ClibUtil/string.hpp"
#include "ClibUtil/rng.hpp"
#include "ClibUtil/singleton.hpp"

#define DLLEXPORT __declspec(dllexport)

namespace logger = SKSE::log;
namespace string = clib_util::string;

using namespace std::literals;
using namespace clib_util::singleton;

namespace stl
{
	using namespace SKSE::stl;

	template <class T>
	void write_thunk_call(std::uintptr_t a_src)
	{
		SKSE::AllocTrampoline(14);

	    auto& trampoline = SKSE::GetTrampoline();
		T::func = trampoline.write_call<5>(a_src, T::thunk);
	}
}

#include "Version.h"

#ifdef SKYRIM_AE
#	define OFFSET(se, ae) ae
#else
#	define OFFSET(se, ae) se
#endif
