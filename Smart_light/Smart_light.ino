#include "RMaker.h"
#include "WiFi.h"
#include "WiFiProv.h"
#include <FastLED.h>
#include <wifi_provisioning/manager.h>
#include "configs.h"

// BLE Credentials
const char *service_name = "PROV_Nanoleaf";
const char *pop = "1234567";

// GPIO
#define LED_PIN 26
static uint8_t gpio_reset = 0;
bool light_state = false;
bool wifi_connected = 0;

// LED CONFIG
CRGB leds[NUM_LEDS];

// INITIAL VALUES
short int brightness = DEFAULT_BRIGHTNESS;
short int saturation = DEFAULT_SATURATION;
short int hue = DEFAULT_HUE;
short int prevHue = DEFAULT_HUE;
short int prevSat = DEFAULT_SATURATION;
short int prevVal = DEFAULT_BRIGHTNESS;

//------------------------------------------- Declaring Devices -----------------------------------------------------//

// The framework provides some standard device types like switch, lightbulb, fan, temperature sensor.
static LightBulb ws2812("Nanoleaf Light");

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
  Serial.println(device_name);

  if (strcmp(device_name, "Nanoleaf Light") == 0)
  {
    if (strcmp(param_name, "Power") == 0)
    {
      Serial.printf("Received value = %s for %s - %s\n", val.val.b ? "true" : "false", device_name, param_name);
      FastLED.clear();
      light_state = val.val.b;
      light_state == true ? setHSVColor(hue, saturation, brightness) : setHSVColor(hue, saturation, 0);
      Serial.println(light_state == true ? "Turning on Light" : "Turning off Light");
    }
    else if (strcmp(param_name, "Brightness") == 0)
    {
      Serial.printf("Received value = %d for %s - %s\n", val.val.i, device_name, param_name);
      brightness = map(val.val.i, 0, 100, 0, 255);
      setHSVColor(hue, saturation, brightness);
    }
    else if (strcmp(param_name, "Hue") == 0)
    {
      Serial.printf("Received value = %d for %s - %s\n", val.val.i, device_name, param_name);
      hue = map(val.val.i, 0, 360, 0, 255);
      setHSVColor(hue, saturation, brightness);
    }
    else if (strcmp(param_name, "Saturation") == 0)
    {
      Serial.printf("Received value = %d for %s - %s\n", val.val.i, device_name, param_name);
      saturation = map(val.val.i, 0, 100, 0, 255);
      setHSVColor(hue, saturation, brightness);
    }
    param->updateAndReport(val);
  }
}


void setup()
{
  // Configure the input GPIOs
  pinMode(gpio_reset, INPUT);
  pinMode(LED_PIN, INPUT);

  initWS2812();
  Serial.begin(115200);

  //------------------------------------------- Declaring Node -----------------------------------------------------//
  Node my_node;
  my_node = RMaker.initNode("Nanoleaf");

  Param hueParam("Hue", "esp.param.hue", value((int)DEFAULT_HUE), PROP_FLAG_READ | PROP_FLAG_WRITE);
  hueParam.addBounds(value(0), value(360), value(1));
  hueParam.addUIType(ESP_RMAKER_UI_HUE_CIRCLE);
  ws2812.addParam(hueParam);

  Param saturationParam("Saturation", "esp.param.saturation", value((int)DEFAULT_SATURATION), PROP_FLAG_READ | PROP_FLAG_WRITE);
  saturationParam.addBounds(value(0), value(100), value(1));
  saturationParam.addUIType(ESP_RMAKER_UI_SLIDER);
  ws2812.addParam(saturationParam);

  Param brightnessParam("Brightness", "esp.param.brightness", value((int)DEFAULT_BRIGHTNESS), PROP_FLAG_READ | PROP_FLAG_WRITE);
  brightnessParam.addBounds(value(0), value(100), value(1));
  brightnessParam.addUIType(ESP_RMAKER_UI_SLIDER);
  ws2812.addParam(brightnessParam);

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
  //-----------------------------------------------------------  Logic to Reset RainMaker

  // Read GPIO0 (external button to reset device
  if (digitalRead(gpio_reset) == LOW) { //Push button pressed
    Serial.printf("Reset Button Pressed!\n");
    // Key debounce handling
    delay(100);
    int startTime = millis();
    while (digitalRead(gpio_reset) == LOW) delay(50);
    int endTime = millis();

    if ((endTime - startTime) > 10000) {
      // If key pressed for more than 10secs, reset all
      Serial.printf("Reset to factory.\n");
      wifi_connected = 0;
      RMakerFactoryReset(2);
    } else if ((endTime - startTime) > 3000) {
      Serial.printf("Reset Wi-Fi.\n");
      wifi_connected = 0;
      // If key pressed for more than 3secs, but less than 10, reset Wi-Fi
      RMakerWiFiReset(2);
    }
  }
  delay(100);
}

void initWS2812() 
{
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
  FastLED.clear();
  FastLED.show();
}

void setHSVColor(int targetHue, int targetSat, int targetVal)
{
  short int currentHue = prevHue;
  short int currentSat = prevSat;
  short int currentVal = prevVal;

  short int satDir = targetSat >= currentSat ? COLOR_TRANSITION_RATE : COLOR_TRANSITION_RATE * -1;
  short int valDir = targetVal >= currentVal ? COLOR_TRANSITION_RATE : COLOR_TRANSITION_RATE * -1;
  
  while (currentHue != targetHue || currentSat != targetSat || currentVal != targetVal)
  {
    currentHue = targetHue;
    currentSat = currentSat + satDir;
    currentVal = currentVal + valDir;

    currentSat = targetSat * satDir > currentSat * satDir ? currentSat : targetSat;
    currentVal = targetVal * valDir > currentVal * valDir ? currentVal : targetVal;

    // Serial.printf("Current values: %d, %d, %d\n", currentHue, currentSat, currentVal);

    fill_solid(leds, NUM_LEDS, CHSV(currentHue, currentSat, currentVal));
    FastLED.delay(COLOR_TRANSITION_DELAY);
  }
  Serial.printf("Applied HSV values: %d, %d, %d\n", currentHue, currentSat, currentVal);
  prevHue = currentHue;
  prevSat = currentSat;
  prevVal = currentVal;
}

//void setHSVColor(int targetHue, int targetSat, int targetVal)
//{
//  static uint8_t k;
//  CHSV startColor = CHSV(prevHue, prevSat, prevVal);
//  CHSV targetColor = CHSV(targetHue, targetSat, targetVal);
//  CHSV currentColor;
//  
//  while (startColor != targetColor)
//  {
//    currentColor = blend(startColor, targetColor, k, SHORTEST_HUES);
//    fill_solid(leds, NUM_LEDS, currentColor);
//    k++;
//    FastLED.show();
//    Serial.printf("Current values: %d, %d, %d\n", currentColor.h, currentColor.s, currentColor.v);
//    delay(COLOR_TRANSITION_DELAY);
//  }
//  Serial.printf("Applied HSV values: %d, %d, %d\n", currentColor.h, currentColor.s, currentColor.v);
//  prevHue = currentColor.h;
//  prevSat = currentColor.s;
//  prevVal = currentColor.v;
//}
