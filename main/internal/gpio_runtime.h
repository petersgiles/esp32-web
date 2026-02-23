#pragma once

#include <cstdint>
#include <vector>

enum class PinModeSetting
{
	Input,
	InputPullup,
	Output
};

struct PinRuntime
{
	uint8_t gpio;
	PinModeSetting mode;
	bool value;
	bool canControl;
};

std::vector<PinRuntime> create_pin_states();
void log_pin_map_summary(const char *tag, const std::vector<PinRuntime> &pin_states, const char *target_name);
const char *mode_to_string(PinModeSetting mode);
PinRuntime *find_pin(std::vector<PinRuntime> &pin_states, uint8_t gpio);
void apply_mode(PinRuntime &pin);
bool read_pin_level(uint8_t gpio);
bool update_pin_values(std::vector<PinRuntime> &pin_states);
