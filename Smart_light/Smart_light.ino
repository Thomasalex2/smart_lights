#include <FastLED.h>
#include "RMaker.h"
#include "WiFi.h"
#include "WiFiProv.h"
#include <wifi_provisioning/manager.h>
#include "configs.h"

// BLE Credentials
const char *service_name = "PROV_" DEVICE_NAME;
const char *pop = PROV_PASSWORD;

// GPIO
static uint8_t gpio_reset = 0;
bool light_state = false;
bool wifi_connected = 0;

// LED CONFIG
CRGB leds[NUM_LEDS];

// INITIAL VALUES
uint8_t appliedHue = DEFAULT_HUE;
uint8_t appliedSat = DEFAULT_SATURATION;
uint8_t appliedVal = DEFAULT_BRIGHTNESS;
uint8_t brightnessBeforeTurnOff = DEFAULT_BRIGHTNESS;
bool motion_detection = DEFAULT_MOTION;
bool night_motion_detection = DEFAULT_NIGHT_MOTION;

// Initialization Variables
String color_preset = DEFAULT_COLOR_PRESET;

// Timing Variables
unsigned long int detection_time = 0;
unsigned long int prev_color_cycle_time = 0;
unsigned long int last_color_change_time = 0;
unsigned long int last_split_color_change = 0;
unsigned long int transitionStartTime;

// Flag Variables
bool motion_flag = false;
bool night_motion_flag = false;
bool night_light_state = false;

CHSV colorCurrent;
CHSV colorStart;
CHSV colorTarget;

//------------------------------------------- Declaring Devices -----------------------------------------------------//

// The framework provides some standard device types like switch, lightbulb, fan, temperature sensor.
static LightBulb ws2812(RGB_LIGHTS);

void sysProvEvent(arduino_event_t *sys_event)
{
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_START:
    #if CONFIG_IDF_TARGET_ESP32
          Serial.printf("\nProvisioning Started with name \"%s\" and PoP \"%s\" on BLE\n", service_name, pop);
          printQR(service_name, pop, "ble");
    #else
          Serial.printf("\nProvisioning Started with name \"%s\" and PoP \"%s\" on SoftAP\n", service_name, pop);
          printQR(service_name, pop, "softap");
    #endif
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.printf("\nConnected to Wi-Fi!\n");
      wifi_connected = 1;
      delay(500);
      break;
    case ARDUINO_EVENT_PROV_CRED_RECV: {
        Serial.println("\nReceived Wi-Fi credentials");
        Serial.print("\tSSID : ");
        Serial.println((const char *) sys_event->event_info.prov_cred_recv.ssid);
        Serial.print("\tPassword : ");
        Serial.println((char const *) sys_event->event_info.prov_cred_recv.password);
        break;
      }
    case ARDUINO_EVENT_PROV_INIT:
      wifi_prov_mgr_disable_auto_stop(10000);
      break;
    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      Serial.println("Stopping Provisioning!!!");
      wifi_prov_mgr_stop_provisioning();
      break;
  }
}

void write_callback(Device *device, Param *param, const param_val_t val, void *priv_data, write_ctx_t *ctx)
{
  const char *device_name = device->getDeviceName();
  const char *param_name = param->getParamName();

  if (strcmp(device_name, RGB_LIGHTS) == 0)
  {
    if (strcmp(param_name, "Power") == 0)
    {
      Serial.printf("Received value = %s for %s - %s\n", val.val.b ? "true" : "false", device_name, param_name);
      light_state = val.val.b;
      if (light_state)
      {
        transitionHSVColor(appliedHue, appliedSat, brightnessBeforeTurnOff);
        Serial.println("Turning on " DEVICE_NAME);
      } 
      else 
      {
        brightnessBeforeTurnOff = appliedVal;
        transitionHSVColor(appliedHue, appliedSat, 0);
        Serial.println("Turning off " DEVICE_NAME);
      }
    }
    else if (strcmp(param_name, "Brightness") == 0)
    {
      Serial.printf("Received value = %d for %s - %s\n", val.val.i, device_name, param_name);
      uint8_t newVal = map(val.val.i, 0, 100, 0, 255);
      transitionHSVColor(appliedHue, appliedSat, newVal);
    }
    else if (strcmp(param_name, "Hue") == 0)
    {
      Serial.printf("Received value = %d for %s - %s\n", val.val.i, device_name, param_name);
      uint8_t newHue = map(val.val.i, 0, 360, 0, 255);
      transitionHSVColor(newHue, appliedSat, appliedVal);
    }
    else if (strcmp(param_name, "Saturation") == 0)
    {
      Serial.printf("Received value = %d for %s - %s\n", val.val.i, device_name, param_name);
      uint8_t newSat = map(val.val.i, 0, 100, 0, 255);
      transitionHSVColor(appliedHue, newSat, appliedVal);
    }
    else if (strcmp(param_name, "Colour Presets") == 0)
    {
      color_preset = val.val.s;
      Serial.printf("\nReceived value = %s for %s - %s\n", val.val.s, device_name, param_name);
    }
    else if (strcmp(param_name, "Motion Awareness") == 0)
    {
      motion_detection = val.val.b;
      Serial.println(motion_detection == true ? "Turning on Motion Awareness" : "Turning off Motion Awareness");
    }
    else if (strcmp(param_name, "Night Motion Detection") == 0)
    {
      night_motion_detection = val.val.b;
      Serial.println(night_motion_detection == true ? "Turning on Night Motion Awareness" : "Turning off Night Motion Awareness");
    }
    param->updateAndReport(val);
  }
}

void setup()
{
  // Configure the input GPIOs
  pinMode(gpio_reset, INPUT);
  pinMode(MOTION_PIN, INPUT);

  initWS2812();
  Serial.begin(115200);

  //------------------------------------------- Declaring Node -----------------------------------------------------//
  Node my_node;
  my_node = RMaker.initNode("Nanoleaf");

  static const char *colorPresetModes[] = {"Custom", "Cycle", "Split Cycle"};
  Param colorPresets("Colour Presets", ESP_RMAKER_PARAM_MODE, value("Custom"), PROP_FLAG_READ | PROP_FLAG_WRITE);
  colorPresets.addValidStrList(colorPresetModes, 3);
  colorPresets.addUIType(ESP_RMAKER_UI_DROPDOWN);
  ws2812.addParam(colorPresets);

  Param hueParam("Hue", "esp.param.hue", value((int)map(DEFAULT_HUE, 0, 255, 0, 360)), PROP_FLAG_READ | PROP_FLAG_WRITE);
  hueParam.addBounds(value(0), value(360), value(1));
  hueParam.addUIType(ESP_RMAKER_UI_HUE_CIRCLE);
  ws2812.addParam(hueParam);

  Param brightnessParam("Brightness", "esp.param.brightness", value((int)map(DEFAULT_BRIGHTNESS, 0, 255, 0, 100)), PROP_FLAG_READ | PROP_FLAG_WRITE);
  brightnessParam.addBounds(value(0), value(100), value(1));
  brightnessParam.addUIType(ESP_RMAKER_UI_SLIDER);
  ws2812.addParam(brightnessParam);

  Param saturationParam("Saturation", "esp.param.saturation", value((int)map(DEFAULT_SATURATION, 0, 255, 0, 100)), PROP_FLAG_READ | PROP_FLAG_WRITE);
  saturationParam.addBounds(value(0), value(100), value(1));
  saturationParam.addUIType(ESP_RMAKER_UI_SLIDER);
  ws2812.addParam(saturationParam);

  Param motionParam("Motion Awareness", "esp.param.power", value((bool)DEFAULT_MOTION), PROP_FLAG_READ | PROP_FLAG_WRITE);
  motionParam.addUIType(ESP_RMAKER_UI_TOGGLE);
  ws2812.addParam(motionParam);

  Param nightMotionParam("Night Motion Detection", "esp.param.power", value((bool)DEFAULT_MOTION), PROP_FLAG_READ | PROP_FLAG_WRITE);
  nightMotionParam.addUIType(ESP_RMAKER_UI_TOGGLE);
  ws2812.addParam(nightMotionParam);

  //Standard switch device
  ws2812.addCb(write_callback);

  //------------------------------------------- Adding Devices in Node -----------------------------------------------------//
  my_node.addDevice(ws2812);

  //This is optional
  RMaker.enableOTA(OTA_USING_PARAMS);
  //If you want to enable scheduling, set time zone for your region using setTimeZone().
  //The list of available values are provided here https://rainmaker.espressif.com/docs/time-service.html
  RMaker.setTimeZone("Asia/Kolkata");
  // Alternatively, enable the Timezone service and let the phone apps set the appropriate timezone
  RMaker.enableTZService();
  RMaker.enableSchedule();

  Serial.printf("\nStarting ESP-RainMaker\n");
  RMaker.start();

  WiFi.onEvent(sysProvEvent);

  #if CONFIG_IDF_TARGET_ESP32
    WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM, WIFI_PROV_SECURITY_1, pop, service_name);
  #else
    WiFiProv.beginProvision(WIFI_PROV_SCHEME_SOFTAP, WIFI_PROV_SCHEME_HANDLER_NONE, WIFI_PROV_SECURITY_1, pop, service_name);
  #endif

}

void loop()
{
  if (motion_detection) {
    checkForMotion();
  }
  if (night_motion_detection) {
    checkForNightMotion();
  }
  if (color_preset != "Custom") {
    checkColorPresets();
  }
  if (millis() - last_color_change_time > SIGNAL_REPETITION_TIME) {
    Serial.println("Signal Repetition Process");
    changeHSVcolor(appliedHue, appliedSat, appliedVal);
    last_color_change_time = millis();
  }
  rainmakerResetListener();
}

void rainmakerResetListener() 
{
  // Read GPIO0 (external button to reset device
  if (digitalRead(gpio_reset) == LOW)
  { // Push button pressed
    Serial.printf("Reset Button Pressed!\n");
    // Key debounce handling
    delay(100);
    int startTime = millis();
    while (digitalRead(gpio_reset) == LOW)
      delay(50);
    int endTime = millis();

    if ((endTime - startTime) > 10000)
    {
      // If key pressed for more than 10secs, reset all
      Serial.printf("Reset to factory.\n");
      wifi_connected = 0;
      RMakerFactoryReset(2);
    }
    else if ((endTime - startTime) > 3000)
    {
      Serial.printf("Reset Wi-Fi.\n");
      wifi_connected = 0;
      // If key pressed for more than 3secs, but less than 10, reset Wi-Fi
      RMakerWiFiReset(2);
    }
  }
}

void initWS2812()
{
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(LED_VOLTS, LED_MAX_AMPS);
  FastLED.clear();
  if (DEFAULT_LIGHT)
  {
    fill_solid(leds, NUM_LEDS, CHSV(appliedHue, appliedSat, appliedVal));
  }
  FastLED.show();
}

void changeHSVcolor(int hue, int sat, int val)
{
  fill_solid(leds, NUM_LEDS, CHSV(hue, sat, val));
  FastLED.show();
  appliedHue = hue;
  appliedSat = sat;
  appliedVal = val;
  last_color_change_time = millis();
}

void splitColourSelector()
{
  static int counter = 0;
  uint8_t newHue = random8();
  for (int i = 0 + (LED_SET * counter); i < LED_SET * (counter + 1); i += 8)
  {
    for (int j = 0; j < 8; j++)
    {
      leds[i + j] = CHSV(newHue, 255, appliedVal);
    }
    FastLED.delay(cubicwave8(i*4) / 8.0);
  }
  counter++;
  if (counter * LED_SET >= NUM_LEDS)
  {
    counter = 0;
  }
  FastLED.show();
  last_color_change_time = millis();
}

void transitionHSVColor(int targetHue, int targetSat, int targetVal)
{
  transitionStartTime = millis();
  uint8_t k = 0;
  Serial.printf("Target HSV values: %d, %d, %d -> %d, %d, %d\n", appliedHue, appliedSat, appliedVal, targetHue, targetSat, targetVal);
  CHSV colorStart = CHSV(appliedHue, appliedSat, appliedVal);
  CHSV colorTarget = CHSV(targetHue, targetSat, targetVal);
  CHSV colorCurrent = colorStart;

  while (colorCurrent != colorTarget)
  {
    colorCurrent = blend(colorStart, colorTarget, k, SHORTEST_HUES);
    Serial.printf("Current values: %d, %d, %d\n", colorCurrent.h, colorCurrent.s, colorCurrent.v);
    changeHSVcolor(colorCurrent.h, colorCurrent.s, colorCurrent.v);
    FastLED.delay(cubicwave8(k)/100.0);
    k++;
  }
  Serial.printf("Applied HSV values: %d, %d, %d\n", colorCurrent.h, colorCurrent.s, colorCurrent.v);
  
  //! TODO: Fix the time function below - gives incorrect time
  Serial.print("Elapsed transition time: ");
  Serial.println((millis() - transitionStartTime) / 1000.0);
}

void checkColorPresets() 
{
  if (millis() - prev_color_cycle_time >= DEFAULT_COLOR_CYCLE_TIME)
  {
    if (color_preset == "Cycle")
    {
      uint8_t newHue = appliedHue + 1;
      changeHSVcolor(newHue, appliedSat, appliedVal);
    }
    if (color_preset == "Split Cycle")
    {
      if (millis() - last_split_color_change > 1000)
      {
        splitColourSelector();
        last_split_color_change = millis();
      }
    }
    prev_color_cycle_time = millis();
  }
}

void checkForMotion()
{

  if (digitalRead(MOTION_PIN))
  {
    if(!motion_flag)
    {
      Serial.println("Motion detected");
      uint8_t prevHue = appliedHue;
      uint8_t newHue = appliedHue + COLOR_SHIFT;
      Serial.printf("Changing Hue: %d -> %d\n", prevHue, newHue);
      transitionHSVColor(newHue, appliedSat, appliedVal);
      FastLED.delay(DEFAULT_MOTION_AWARENESS_PERIOD);
      transitionHSVColor(prevHue, appliedSat, appliedVal);
      motion_flag = true;
    }
  }
  else {
    motion_flag = false;
  }
}

void checkForNightMotion()
{
  if (digitalRead(MOTION_PIN) && !night_motion_flag && !light_state)
  {
    Serial.println("Night motion detected");
    transitionHSVColor(appliedHue, appliedSat, NIGHT_MOTION_BRIGHTNESS);
    night_motion_flag = true;
    night_light_state = true;
    detection_time = millis();
  }
  else
  {
    night_motion_flag = false;
    if (millis() - detection_time > NIGHT_MOTION_TIMEOUT && !light_state && night_light_state)
    {
      transitionHSVColor(appliedHue, appliedSat, 0);
      night_light_state = false;
    } 
  }
}
