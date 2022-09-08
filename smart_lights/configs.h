#define STR_(x) #x
#define STR(x) STR_(x)

// Device Config
#define DEVICE_NAME "ESP_RAINMAKER"
#define RGB_LIGHTS "Nanoleaf"
#define PROV_PASSWORD "1234567"
#define GPIO_RESET 4
#define FORMAT_LITTLEFS_IF_FAILED true
#define USER_CONFIG_FILE "/user_settings.txt"

// Set Light Values
#define LED_PIN 26
#define LED_SET 64
#define DEFAULT_LIGHT true
#define DEFAULT_BRIGHTNESS 127
#define DEFAULT_SATURATION 255
#define DEFAULT_HUE 128
#define LED_VOLTS 5
#define LED_MAX_AMPS 1500

// LED CONFIG
#define LED_TYPE WS2812
#define NUM_LEDS 192
#define COLOR_ORDER GRB
#define COLOR_TRANSITION_DELAY 1
#define DEFAULT_COLOR_PRESET "Custom"
#define DEFAULT_COLOR_CYCLE_TIME 200
#define CYCLE_HUE_RATE 2
#define BLEND_RATE 5
#define SIGNAL_REPETITION_TIME 50000

// Motion Detectors Configuration
#define MOTION_PIN 27
#define DEFAULT_MOTION false
#define DEFAULT_NIGHT_MOTION false
#define DEFAULT_MOTION_AWARENESS_PERIOD 4000
#define COLOR_SHIFT 41
#define NIGHT_MOTION_BRIGHTNESS 100
#define NIGHT_MOTION_TIMEOUT 10000

#define JSON_LENGTH 256
#define DEFAULT_SETTINGS_JSON "{\"hsv\" : {\"hue\" :" STR(DEFAULT_HUE) ", \"sat\" :" STR(DEFAULT_SATURATION) ", \"val\" : " STR(DEFAULT_BRIGHTNESS) "}, \"preset\" : \"" DEFAULT_COLOR_PRESET "\", \"motion\": { \"awareness\" : " STR(DEFAULT_MOTION) ", \"night_motion\" : " STR(DEFAULT_NIGHT_MOTION) " }}"
