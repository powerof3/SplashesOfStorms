#pragma once

class RainObject
{
public:
	RainObject() = delete;
	virtual ~RainObject() = default;

	explicit RainObject(std::string a_type) :
		type(std::move(a_type))
	{}

	virtual void LoadSettings(const toml::node_view<const toml::node>& a_node);

	bool enabled{ true };
	float rayCastRadius{ 1000.0f };
	float delay{ 0.0f };

protected:
	std::string type;

	template <class T>
	void get_value(T& a_value, const toml::node_view<const toml::node>& a_node, std::string_view a_key)
	{
		a_value = *a_node[type][a_key].value_or(a_value);
	}
};

class Splash : public RainObject
{
public:
	Splash() :
		RainObject("splashes")
	{}
	~Splash() override = default;

	void LoadSettings(const toml::node_view<const toml::node>& a_node) override;

	std::string nif{ "Effects\\rainSplashNoSpray.NIF" };
	std::string nifActor{ "Effects\\rainSplashNoSpray.NIF" };
	float nifScale{ 0.6f };
	float nifScaleActor{ 0.2f };
};

class Ripple : public RainObject
{
public:
	Ripple() :
		RainObject("ripples")
	{}
	~Ripple() override = default;

	void LoadSettings(const toml::node_view<const toml::node>& a_node) override;

	std::uint32_t rayCastIterations{ 15 };
	float rippleDisplacementAmount{ 0.4f };
};

class Rain
{
public:
	enum class TYPE
	{
		kNone,
		kLight,
		kMedium,
		kHeavy
	};

	void LoadSettings(const toml::table& a_tbl, TYPE a_type, std::string_view a_section);

	TYPE type{ TYPE::kNone };
	Splash splash;
	Ripple ripple;
};

class Settings
{
public:
	[[nodiscard]] static Settings* GetSingleton()
	{
		static Settings singleton;
		return std::addressof(singleton);
	}

	bool LoadSettings();
	Rain* GetRainType();

	float rayCastHeight{ 6000.0f };

private:
	Rain light;
	Rain medium;
	Rain heavy;

	struct
	{
		RE::TESWeather* weather{ nullptr };
		Rain::TYPE rainType{ Rain::TYPE::kNone };
	} cache;
};
