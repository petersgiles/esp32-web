#include "internal/board_profile.h"
#include "sdkconfig.h"

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
	if (
		gpio == 12 || gpio == 13 ||
		gpio == 16 || gpio == 17 ||
		gpio == 24 || gpio == 25 || gpio == 26 || gpio == 27 || gpio == 28 || gpio == 29 || gpio == 30)
	{
		return true;
	}
#endif
#if CONFIG_IDF_TARGET_ESP32S3
	if (gpio == 19 || gpio == 20)
	{
		return true;
	}
#endif
	return false;
}
