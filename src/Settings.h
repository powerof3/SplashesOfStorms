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
	float rayCastRadius{ 1024.0f };
	std::uint32_t rayCastIterations{ 1 };

protected:
	std::string type;

	template <class T>
	void get_value(T& a_value, const toml::node_view<const toml::node>& a_node, std::string_view a_key)
	{
		a_value = a_node[type][a_key].value_or(a_value);
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
	{
		rayCastIterations = 15;
	}
	~Ripple() override = default;

	void LoadSettings(const toml::node_view<const toml::node>& a_node) override;

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
		kHeavy,
		kInvalid // Snow
	};

	void LoadSettings(const toml::table& a_tbl, TYPE a_type, std::string_view a_section);

	TYPE type{ TYPE::kNone };
	Splash splash;
	Ripple ripple;
};

namespace Settings
{
	class Manager
	{
	public:
		[[nodiscard]] static Manager* GetSingleton()
		{
			static Manager singleton;
			return std::addressof(singleton);
		}

		bool LoadSettings();

		Rain* GetRain(float a_particleDensity);
		Rain* GetRain();

        [[nodiscard]] Rain::TYPE GetRainType() const;
	    void SetRainType(Rain::TYPE a_type);

		bool enableDebugMarkerSplash{ false };
		bool enableDebugMarkerRipple{ false };

	private:
		Rain light;
		Rain medium;
		Rain heavy;

		Rain::TYPE currentRainType{ Rain::TYPE::kNone };
	};
}
