#include "internal/gpio_runtime.h"

#include "Arduino.h"
#include "esp_log.h"
#include "internal/board_profile.h"
#include "soc/soc_caps.h"

std::vector<PinRuntime> create_pin_states()
{
	std::vector<PinRuntime> pins;
	const uint8_t max_gpio = static_cast<uint8_t>(SOC_GPIO_PIN_COUNT < 64 ? SOC_GPIO_PIN_COUNT : 64);
	const uint64_t valid_mask = static_cast<uint64_t>(SOC_GPIO_VALID_GPIO_MASK);
	pins.reserve(max_gpio);

	for (uint8_t gpio = 0; gpio < max_gpio; ++gpio)
	{
		if ((valid_mask & (UINT64_C(1) << gpio)) == 0)
		{
			continue;
		}

		pins.push_back({gpio, PinModeSetting::Input, false, !is_protected_gpio(gpio)});
	}

	return pins;
}

void log_pin_map_summary(const char *tag, const std::vector<PinRuntime> &pin_states, const char *target_name)
{
	String usable;
	String reserved;
	for (const auto &pin : pin_states)
	{
		String entry = String(pin.gpio);
		if (pin.canControl)
		{
			if (!usable.isEmpty())
			{
				usable += ",";
			}
			usable += entry;
		}
		else
		{
			if (!reserved.isEmpty())
			{
				reserved += ",";
			}
			reserved += entry;
		}
	}

	ESP_LOGI(tag, "Target=%s GPIO total=%u", target_name, static_cast<unsigned>(pin_states.size()));
	ESP_LOGI(tag, "GPIO controllable: %s", usable.c_str());
	if (!reserved.isEmpty())
	{
		ESP_LOGI(tag, "GPIO reserved: %s", reserved.c_str());
	}
}

const char *mode_to_string(PinModeSetting mode)
{
	switch (mode)
	{
	case PinModeSetting::Input:
		return "input";
	case PinModeSetting::InputPullup:
		return "input_pullup";
	case PinModeSetting::Output:
		return "output";
	default:
		return "input";
	}
}

PinRuntime *find_pin(std::vector<PinRuntime> &pin_states, uint8_t gpio)
{
	for (auto &pin : pin_states)
	{
		if (pin.gpio == gpio)
		{
			return &pin;
		}
	}
	return nullptr;
}

void apply_mode(PinRuntime &pin)
{
	if (pin.mode == PinModeSetting::Input)
	{
		pinMode(pin.gpio, INPUT);
		return;
	}
	if (pin.mode == PinModeSetting::InputPullup)
	{
		pinMode(pin.gpio, INPUT_PULLUP);
		return;
	}
	pinMode(pin.gpio, OUTPUT);
}

bool read_pin_level(uint8_t gpio)
{
	return digitalRead(gpio) == HIGH;
}

bool update_pin_values(std::vector<PinRuntime> &pin_states)
{
	bool changed = false;
	for (auto &pin : pin_states)
	{
		if (!pin.canControl)
		{
			continue;
		}
		const bool new_value = read_pin_level(pin.gpio);
		if (new_value != pin.value)
		{
			pin.value = new_value;
			changed = true;
		}
	}
	return changed;
}
