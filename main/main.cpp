#include "Arduino.h"
#include "ESPAsyncWebServer.h"
#include "WiFi.h"
#include "esp_log.h"
#include <array>

#ifndef APP_WIFI_SSID
#define APP_WIFI_SSID "unexplained_error"
#endif

#ifndef APP_WIFI_PASSWORD
#define APP_WIFI_PASSWORD "unexpected-bacon"
#endif

static const char *TAG = "web";
static const char *WIFI_SSID = APP_WIFI_SSID;
static const char *WIFI_PASSWORD = APP_WIFI_PASSWORD;
static uint8_t last_disconnect_reason = 0;

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");

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

static std::array<PinRuntime, 37> pin_states = {{
	{0, PinModeSetting::Input, false, true},
	{1, PinModeSetting::Input, false, true},
	{2, PinModeSetting::Input, false, true},
	{3, PinModeSetting::Input, false, true},
	{4, PinModeSetting::Input, false, true},
	{5, PinModeSetting::Input, false, true},
	{6, PinModeSetting::Input, false, true},
	{7, PinModeSetting::Input, false, true},
	{8, PinModeSetting::Input, false, true},
	{9, PinModeSetting::Input, false, true},
	{10, PinModeSetting::Input, false, true},
	{11, PinModeSetting::Input, false, true},
	{12, PinModeSetting::Input, false, true},
	{13, PinModeSetting::Input, false, true},
	{14, PinModeSetting::Input, false, true},
	{15, PinModeSetting::Input, false, true},
	{16, PinModeSetting::Input, false, true},
	{17, PinModeSetting::Input, false, true},
	{18, PinModeSetting::Input, false, true},
	{19, PinModeSetting::Input, false, true},
	{20, PinModeSetting::Input, false, true},
	{21, PinModeSetting::Input, false, true},
	{26, PinModeSetting::Input, false, true},
	{33, PinModeSetting::Input, false, true},
	{34, PinModeSetting::Input, false, true},
	{35, PinModeSetting::InputPullup, false, true},
	{36, PinModeSetting::Input, false, true},
	{37, PinModeSetting::InputPullup, false, true},
	{38, PinModeSetting::Input, false, true},
	{39, PinModeSetting::Input, false, true},
	{40, PinModeSetting::Input, false, true},
	{41, PinModeSetting::Input, false, true},
	{42, PinModeSetting::Input, false, true},
	{43, PinModeSetting::Input, false, true},
	{44, PinModeSetting::Input, false, true},
	{45, PinModeSetting::Input, false, true},
	{46, PinModeSetting::Input, false, true},
}};

extern const uint8_t web_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t web_index_html_end[] asm("_binary_index_html_end");
extern const uint8_t web_styles_css_start[] asm("_binary_styles_css_start");
extern const uint8_t web_styles_css_end[] asm("_binary_styles_css_end");
extern const uint8_t web_app_js_start[] asm("_binary_app_js_start");
extern const uint8_t web_app_js_end[] asm("_binary_app_js_end");

static void send_embedded(AsyncWebServerRequest *request, const char *content_type, const uint8_t *start, const uint8_t *end)
{
	size_t length = static_cast<size_t>(end - start);
	if (length > 0 && start[length - 1] == '\0')
	{
		length -= 1;
	}
	request->send(200, content_type, start, length);
}

static const char *mode_to_string(PinModeSetting mode)
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

static PinRuntime *find_pin(uint8_t gpio)
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

static void apply_mode(PinRuntime &pin)
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

static bool read_pin_level(uint8_t gpio)
{
	return digitalRead(gpio) == HIGH;
}

static void send_ws_error(AsyncWebSocketClient *client, const char *message)
{
	String payload;
	payload.reserve(80);
	payload = "{\"type\":\"error\",\"message\":\"";
	payload += message;
	payload += "\"}";
	if (client != nullptr)
	{
		client->text(payload);
	}
}

static String build_snapshot_payload()
{
	String payload;
	payload.reserve(1600);
	payload = "{\"type\":\"snapshot\",\"uptimeMs\":";
	payload += String(static_cast<unsigned long>(millis()));
	payload += ",\"pins\":[";

	for (size_t i = 0; i < pin_states.size(); ++i)
	{
		const auto &pin = pin_states[i];
		payload += "{\"gpio\":";
		payload += String(pin.gpio);
		payload += ",\"mode\":\"";
		payload += mode_to_string(pin.mode);
		payload += "\",\"value\":";
		payload += pin.value ? "true" : "false";
		payload += ",\"canControl\":";
		payload += pin.canControl ? "true" : "false";
		payload += "}";
		if (i + 1 < pin_states.size())
		{
			payload += ",";
		}
	}

	payload += "]}";
	return payload;
}

static void send_snapshot_to_client(AsyncWebSocketClient *client)
{
	if (client == nullptr)
	{
		return;
	}
	client->text(build_snapshot_payload());
}

static void broadcast_snapshot()
{
	ws.textAll(build_snapshot_payload());
}

static bool parse_uint8_field(const String &message, const char *key, uint8_t *value)
{
	String token = "\"";
	token += key;
	token += "\":";
	const int token_index = message.indexOf(token);
	if (token_index < 0)
	{
		return false;
	}

	int cursor = token_index + token.length();
	while (cursor < static_cast<int>(message.length()) && message[cursor] == ' ')
	{
		cursor++;
	}

	if (cursor >= static_cast<int>(message.length()) || message[cursor] < '0' || message[cursor] > '9')
	{
		return false;
	}

	uint16_t parsed = 0;
	while (cursor < static_cast<int>(message.length()) && message[cursor] >= '0' && message[cursor] <= '9')
	{
		parsed = static_cast<uint16_t>(parsed * 10 + static_cast<uint16_t>(message[cursor] - '0'));
		cursor++;
	}

	if (parsed > 255)
	{
		return false;
	}

	*value = static_cast<uint8_t>(parsed);
	return true;
}

static bool parse_bool_field(const String &message, const char *key, bool *value)
{
	String token = "\"";
	token += key;
	token += "\":";
	const int token_index = message.indexOf(token);
	if (token_index < 0)
	{
		return false;
	}

	const int cursor = token_index + token.length();
	if (message.startsWith("true", cursor) || message.startsWith("1", cursor))
	{
		*value = true;
		return true;
	}
	if (message.startsWith("false", cursor) || message.startsWith("0", cursor))
	{
		*value = false;
		return true;
	}
	return false;
}

static bool parse_mode_field(const String &message, String *mode)
{
	const String token = "\"mode\":\"";
	const int token_index = message.indexOf(token);
	if (token_index < 0)
	{
		return false;
	}
	const int start = token_index + token.length();
	const int end = message.indexOf('"', start);
	if (end < 0)
	{
		return false;
	}
	*mode = message.substring(start, end);
	return true;
}

static bool update_pin_values()
{
	bool changed = false;
	for (auto &pin : pin_states)
	{
		const bool new_value = read_pin_level(pin.gpio);
		if (new_value != pin.value)
		{
			pin.value = new_value;
			changed = true;
		}
	}
	return changed;
}

static void handle_ws_message(AsyncWebSocketClient *client, uint8_t *data, size_t len)
{
	String message;
	message.reserve(len + 1);
	for (size_t i = 0; i < len; ++i)
	{
		message += static_cast<char>(data[i]);
	}

	if (message.indexOf("\"type\":\"set\"") >= 0)
	{
		uint8_t gpio = 0;
		String mode;
		if (!parse_uint8_field(message, "gpio", &gpio) || !parse_mode_field(message, &mode))
		{
			send_ws_error(client, "Invalid set payload");
			return;
		}

		PinRuntime *pin = find_pin(gpio);
		if (pin == nullptr)
		{
			send_ws_error(client, "Unsupported GPIO");
			return;
		}

		if (mode == "input")
		{
			pin->mode = PinModeSetting::Input;
		}
		else if (mode == "input_pullup")
		{
			pin->mode = PinModeSetting::InputPullup;
		}
		else if (mode == "output")
		{
			pin->mode = PinModeSetting::Output;
		}
		else
		{
			send_ws_error(client, "Unsupported mode");
			return;
		}

		apply_mode(*pin);

		bool level = false;
		if (pin->mode == PinModeSetting::Output && parse_bool_field(message, "value", &level))
		{
			digitalWrite(pin->gpio, level ? HIGH : LOW);
		}

		pin->value = read_pin_level(pin->gpio);
		broadcast_snapshot();
		return;
	}

	if (message.indexOf("\"type\":\"write\"") >= 0)
	{
		uint8_t gpio = 0;
		bool level = false;
		if (!parse_uint8_field(message, "gpio", &gpio) || !parse_bool_field(message, "value", &level))
		{
			send_ws_error(client, "Invalid write payload");
			return;
		}

		PinRuntime *pin = find_pin(gpio);
		if (pin == nullptr)
		{
			send_ws_error(client, "Unsupported GPIO");
			return;
		}
		if (pin->mode != PinModeSetting::Output)
		{
			send_ws_error(client, "GPIO not in output mode");
			return;
		}

		digitalWrite(pin->gpio, level ? HIGH : LOW);
		pin->value = read_pin_level(pin->gpio);
		broadcast_snapshot();
		return;
	}

	if (message.indexOf("\"type\":\"snapshot\"") >= 0)
	{
		send_snapshot_to_client(client);
		return;
	}

	send_ws_error(client, "Unsupported message type");
}

extern "C" void app_main(void)
{
	initArduino();

	WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info)
				 {
                 if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED)
                 {
                   last_disconnect_reason = info.wifi_sta_disconnected.reason;
                   ESP_LOGW(TAG, "Wi-Fi disconnected, reason=%u", info.wifi_sta_disconnected.reason);
                 } });

	WiFi.mode(WIFI_STA);
	WiFi.setSleep(false);
	WiFi.setAutoReconnect(true);
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

	ESP_LOGI(TAG, "Connecting to Wi-Fi SSID: %s", WIFI_SSID);

	int retry_count = 0;
	while ((WiFi.status() != WL_CONNECTED || WiFi.localIP() == INADDR_NONE) && retry_count < 60)
	{
		delay(500);
		retry_count++;
	}

	if (WiFi.status() != WL_CONNECTED || WiFi.localIP() == INADDR_NONE)
	{
		ESP_LOGE(TAG, "Failed to connect to Wi-Fi (status=%d, reason=%u)", WiFi.status(), last_disconnect_reason);
		return;
	}

	const IPAddress sta_ip = WiFi.localIP();
	ESP_LOGI(TAG, "Wi-Fi connected");
	ESP_LOGI(TAG, "Open http://%s", sta_ip.toString().c_str());

	for (auto &pin : pin_states)
	{
		apply_mode(pin);
		pin.value = read_pin_level(pin.gpio);
		ESP_LOGI(TAG, "GPIO %u mode=%s value=%s", pin.gpio, mode_to_string(pin.mode), pin.value ? "HIGH" : "LOW");
	}

	ws.onEvent([](AsyncWebSocket *socket, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
			   {
				   (void)socket;
				   if (type == WS_EVT_CONNECT)
				   {
					   ESP_LOGI(TAG, "WebSocket client connected: %u", client->id());
					   send_snapshot_to_client(client);
					   return;
				   }
				   if (type == WS_EVT_DISCONNECT)
				   {
					   ESP_LOGI(TAG, "WebSocket client disconnected: %u", client->id());
					   return;
				   }
				   if (type == WS_EVT_DATA)
				   {
					   AwsFrameInfo *info = reinterpret_cast<AwsFrameInfo *>(arg);
					   if (info != nullptr && info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
					   {
						   handle_ws_message(client, data, len);
					   }
				   } });

	server.addHandler(&ws);

	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
			  { send_embedded(request, "text/html", web_index_html_start, web_index_html_end); });
	server.on("/", HTTP_HEAD, [](AsyncWebServerRequest *request)
			  { send_embedded(request, "text/html", web_index_html_start, web_index_html_end); });

	server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request)
			  { send_embedded(request, "text/css", web_styles_css_start, web_styles_css_end); });
	server.on("/styles.css", HTTP_HEAD, [](AsyncWebServerRequest *request)
			  { send_embedded(request, "text/css", web_styles_css_start, web_styles_css_end); });

	server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest *request)
			  { send_embedded(request, "application/javascript", web_app_js_start, web_app_js_end); });
	server.on("/app.js", HTTP_HEAD, [](AsyncWebServerRequest *request)
			  { send_embedded(request, "application/javascript", web_app_js_start, web_app_js_end); });

	server.onNotFound([](AsyncWebServerRequest *request)
					  {
					 const String path = request->url();
					 if (path.startsWith("/styles.css"))
					 {
						 send_embedded(request, "text/css", web_styles_css_start, web_styles_css_end);
						 return;
					 }
					 if (path.startsWith("/app.js"))
					 {
						 send_embedded(request, "application/javascript", web_app_js_start, web_app_js_end);
						 return;
					 }
					 if (path == "/favicon.ico")
					 {
						 request->send(204);
						 return;
					 }
					 request->send(404, "text/plain", "Not Found"); });

	server.begin();
	ESP_LOGI(TAG, "HTTP server started on port 80");
	broadcast_snapshot();

	uint32_t last_snapshot_ms = millis();

	for (;;)
	{
		ws.cleanupClients();

		const bool pin_changed = update_pin_values();
		const uint32_t now = millis();
		if (pin_changed || (now - last_snapshot_ms) >= 700)
		{
			broadcast_snapshot();
			last_snapshot_ms = now;
		}

		delay(30);
	}
}
