#include "internal/board_profile.h"
#include "sdkconfig.h"

#include <array>

const char *active_target_name()
{
#if CONFIG_IDF_TARGET_ESP32C6
	return "esp32c6";
#elif CONFIG_IDF_TARGET_ESP32S3
	return "esp32s3";
#elif CONFIG_IDF_TARGET_ESP32
	return "esp32";
#else
	return "unknown";
#endif
}

bool is_protected_gpio(uint8_t gpio)
{
#if CONFIG_IDF_TARGET_ESP32C6
	// These GPIOs are intentionally excluded even if they appear in the SoC valid mask.
	// They are used by board-critical functions (USB/JTAG, console, flash/strap related)
	// and changing mode on them can cause disconnects, reset loops, or unstable flashing.
	static constexpr std::array<uint8_t, 11> kProtectedPins = {
		12, 13,
		16, 17,
		24, 25, 26, 27, 28, 29, 30,
	};
	for (const auto protected_gpio : kProtectedPins)
	{
		if (gpio == protected_gpio)
		{
			return true;
		}
	}
#endif
#if CONFIG_IDF_TARGET_ESP32S3
	// USB-related pins must stay untouched for stable monitor/flash behavior.
	static constexpr std::array<uint8_t, 2> kProtectedPins = {19, 20};
	for (const auto protected_gpio : kProtectedPins)
	{
		if (gpio == protected_gpio)
		{
			return true;
		}
	}
#endif
	return false;
}
