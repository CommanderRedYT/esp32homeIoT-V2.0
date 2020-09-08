#include <Arduino.h>
#define _DEBUG_

#ifdef _DEBUG_
#define DEBUG_ESP_PORT Serial
#define NODEBUG_WEBSOCKETS
#define NDEBUG
#endif

#include <Arduino.h>
#include <ThingerESP32.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiUdp.h>
#include <WakeOnLan.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <HTTPClient.h>
#include "SinricPro.h"
#include "SinricProLight.h"
#include <DHT.h>
#include <DHT_U.h>

const char *ssid = "";
const char *password = "";
const char *WIFI_SSID = "";
const char *WIFI_PASS = "";
#define USERNAME ""
#define DEVICE_ID ""
#define DEVICE_CREDENTIAL ""

#define APP_KEY ""                                         // Should look like "de0bxxxx-1x3x-4x3x-ax2x-5dabxxxxxxxx" //Sinric.pro
#define APP_SECRET "" // Should look like "5f36xxxx-x3x7-4x3x-xexe-e86724a9xxxx-4c4axxxx-3x3x-x5xe-x9x3-333d65xxxxxx"
#define LIGHT_ID ""                                                    // Should look like "5dc1564130xxxxxxxxxxxxxx"

const char *MACAddress = "";
const char *url = "";

#define DHTPIN 19
#define RELAY 4
const uint16_t SEND_PIN = 5;
#define DHTTYPE DHT11
#define BAUD_RATE 115200

bool led = false;
bool wol = false;
bool ledstripToggle = false;
bool ledstripBrighter = false;
bool ledstripDarker = false;
bool avPower = false;
bool avVolup = false;
bool avVoldown = false;
bool avMute = false;
bool avDVD = false;
bool avTuner = false;
bool avAux = false;
bool avPresetUp = false;
bool avPresetDown = false;
bool avMov = false;
bool avEnt = false;
bool webLight = false;
bool boolStripStatus = false;
bool requested = false;

int stripStatus = 0;

unsigned int dataNEC = 0x00000000;

float h;
float t;
float hic;

#define HEARTBEAT_INTERVAL 300000
uint64_t heartbeatTimestamp = 0;
bool isConnected = false;

int colorTemperatureArray[] = {2200, 2700, 4000, 5500, 7000};
int max_color_temperatures = sizeof(colorTemperatureArray) / sizeof(colorTemperatureArray[0]); // calculates how many elements are stored in colorTemperature array

std::map<int, int> colorTemperatureIndex;

void setupColorTemperatureIndex()
{
    Serial.printf("Setup color temperature lookup table\r\n");
    for (int i = 0; i < max_color_temperatures; i++)
    {
        colorTemperatureIndex[colorTemperatureArray[i]] = i;
        Serial.printf("colorTemperatureIndex[%i] = %i\r\n", colorTemperatureArray[i], colorTemperatureIndex[colorTemperatureArray[i]]);
    }
}

struct
{
    bool powerState = false;
    int brightness = 0;
    struct
    {
        byte r = 0;
        byte g = 0;
        byte b = 0;
    } color;
    int colorTemperature = colorTemperatureArray[0]; // set colorTemperature to first element in colorTemperatureArray array
} device_state;

bool onPowerState(const String &deviceId, bool &state)
{
    Serial.printf("Device %s power turned %s \r\n", deviceId.c_str(), state ? "on" : "off");
    device_state.powerState = state;
    if (state)
        {
            dataNEC = 0xFF02FD;
            requested = true;
            sendStripCode();
        }
        else
        {
            dataNEC = 0xFF02FD;
            requested = false;
            sendStripCode();
        }
    return true; // request handled properly
}

bool onBrightness(const String &deviceId, int &brightness)
{
    device_state.brightness = brightness;
    Serial.printf("Device %s brightness level changed to %d\r\n", deviceId.c_str(), brightness);
    return true;
}

bool onAdjustBrightness(const String &deviceId, int brightnessDelta)
{
    device_state.brightness += brightnessDelta;
    Serial.printf("Device %s brightness level changed about %i to %d\r\n", deviceId.c_str(), brightnessDelta, device_state.brightness);
    brightnessDelta = device_state.brightness;
    return true;
}

bool onColor(const String &deviceId, byte &r, byte &g, byte &b)
{
    device_state.color.r = r;
    device_state.color.g = g;
    device_state.color.b = b;
    Serial.printf("Device %s color changed to %d, %d, %d (RGB)\r\n", deviceId.c_str(), device_state.color.r, device_state.color.g, device_state.color.b);
    return true;
}

bool onColorTemperature(const String &deviceId, int &colorTemperature)
{
    device_state.colorTemperature = colorTemperature;
    Serial.printf("Device %s color temperature changed to %d\r\n", deviceId.c_str(), device_state.colorTemperature);
    return true;
}

bool onIncreaseColorTemperature(const String &deviceId, int &colorTemperature)
{
    int index = colorTemperatureIndex[device_state.colorTemperature]; // get index of stored colorTemperature
    index++;                                                          // do the increase
    if (index < 0)
        index = 0; // make sure that index stays within array boundaries
    if (index > max_color_temperatures - 1)
        index = max_color_temperatures - 1;                       // make sure that index stays within array boundaries
    device_state.colorTemperature = colorTemperatureArray[index]; // get the color temperature value
    Serial.printf("Device %s increased color temperature to %d\r\n", deviceId.c_str(), device_state.colorTemperature);
    colorTemperature = device_state.colorTemperature; // return current color temperature value
    return true;
}

bool onDecreaseColorTemperature(const String &deviceId, int &colorTemperature)
{
    int index = colorTemperatureIndex[device_state.colorTemperature]; // get index of stored colorTemperature
    index--;                                                          // do the decrease
    if (index < 0)
        index = 0; // make sure that index stays within array boundaries
    if (index > max_color_temperatures - 1)
        index = max_color_temperatures - 1;                       // make sure that index stays within array boundaries
    device_state.colorTemperature = colorTemperatureArray[index]; // get the color temperature value
    Serial.printf("Device %s decreased color temperature to %d\r\n", deviceId.c_str(), device_state.colorTemperature);
    colorTemperature = device_state.colorTemperature; // return current color temperature value
    return true;
}

void setupSinricPro()
{
    // get a new Light device from SinricPro
    SinricProLight &myLight = SinricPro[LIGHT_ID];

    // set callback function to device
    myLight.onPowerState(onPowerState);
    myLight.onBrightness(onBrightness);
    myLight.onAdjustBrightness(onAdjustBrightness);
    myLight.onColor(onColor);
    myLight.onColorTemperature(onColorTemperature);
    myLight.onIncreaseColorTemperature(onIncreaseColorTemperature);
    myLight.onDecreaseColorTemperature(onDecreaseColorTemperature);

    // setup SinricPro
    SinricPro.onConnected([]() { Serial.printf("Connected to SinricPro\r\n"); });
    SinricPro.onDisconnected([]() { Serial.printf("Disconnected from SinricPro\r\n"); });
    SinricPro.begin(APP_KEY, APP_SECRET);
}

ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);
HTTPClient http;
WiFiMulti wifiMulti;
WebSocketsClient webSocket;
WiFiClient client;
WiFiUDP UDP;
WakeOnLan WOL(UDP);
IRsend irsend(SEND_PIN);
DHT dht(DHTPIN, DHTTYPE);

void setup()
{
    Serial.begin(BAUD_RATE);
    WiFi.setHostname("esp32-homeIoT-V2");
    thing.add_wifi(ssid, password);
    Serial.printf("\r\n\r\n");
    setupColorTemperatureIndex(); // setup our helper map

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    int wifiTry = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
        wifiTry++;
        if(wifiTry == 10){
            wifiTry = 0;
            WiFi.begin(ssid, password);
        }
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    IPAddress localIP = WiFi.localIP();

    thing.add_wifi(ssid, password);
    setupSinricPro();
    pinMode(RELAY, OUTPUT);

    thing["led"] << [](pson &in) {
        led = in;
    };
    thing["wol"] << [](pson &in) {
        wol = in;
        if (wol == true)
        {
            WOL.sendMagicPacket(MACAddress);
            Serial.println("Magic packet was successfuly sent");
            Serial.println("");
            wol = false;
        }
    };
    thing["webledstripToggle"] << [](pson &in) {
        ledstripToggle = in;
        IRledstripToggle();
        ledstripToggle = false;
        Serial.println("ledstripToggle");
    };
    thing["webledstripBrighter"] << [](pson &in) {
        ledstripBrighter = in;
        IRledstripBrighter();
        ledstripBrighter = false;
        Serial.println("ledstripBrighter");
    };
    thing["webledstripDarker"] << [](pson &in) {
        ledstripDarker = in;
        IRledstripDarker();
        ledstripDarker = false;
        Serial.println("ledstripDarker");
    };
    thing["webavPower"] << [](pson &in) {
        avPower = in;
        IRavPower();
        avPower = false;
        Serial.println("webavPower");
    };
    thing["webavVolup"] << [](pson &in) {
        avVolup = in;
        IRavVolup();
        avVolup = false;
        Serial.println("avVolup");
    };
    thing["webavVoldown"] << [](pson &in) {
        avVoldown = in;
        IRavVoldown();
        avVoldown = false;
        Serial.println("avVoldown");
    };
    thing["webavMute"] << [](pson &in) {
        avMute = in;
        IRavMute();
        avMute = false;
        Serial.println("avMute");
    };
    thing["webavDVD"] << [](pson &in) {
        avDVD = in;
        IRavDVD();
        avDVD = false;
        Serial.println("avDVD");
    };
    thing["webavTuner"] << [](pson &in) {
        avTuner = in;
        IRavTuner();
        avTuner = false;
        Serial.println("avTuner");
    };
    thing["webavAux"] << [](pson &in) {
        avAux = in;
        IRavAux();
        avAux = false;
        Serial.println("avAux");
    };
    thing["webavPresetUp"] << [](pson &in) {
        avPresetUp = in;
        IRavPresetUp();
        avPresetUp = false;
        Serial.println("avPresetUp");
    };
    thing["webavPresetDown"] << [](pson &in) {
        avPresetDown = in;
        IRavPresetDown();
        avPresetDown = false;
        Serial.println("avPresetDown");
    };
    thing["webavMov"] << [](pson &in) {
        avMov = in;
        IRavMov();
        avMov = false;
        Serial.println("avMov");
    };
    thing["webavEnt"] << [](pson &in) {
        avEnt = in;
        IRavEnt();
        avEnt = false;
        Serial.println("avEnt");
    };
    thing["webLamp"] << [](pson &in) {
        webLight = in;
        webLamp();
        webLight = false;
        Serial.println("webLamp");
    };
    thing["stripSetStatus"] << [](pson &in) {
        boolStripStatus = in;
    };
    Serial.println("stripSetStatus");

    thing["stripStatus"] >> [](pson &out) {
        out = stripStatus;
        Serial.println("stripStatus");
    };
    thing["dht"] >> [](pson &out) {
        out["hum"] = dht.readHumidity();
        out["tmp"] = dht.readTemperature();
        Serial.println("dht dump");
    };

    thing["webledstripOff"] << [](pson &in) {
        dataNEC = 0xFF02FD;
        requested = false;
        sendStripCode();
        Serial.println("ledstripOff");
    };

    thing["webledstripOn"] << [](pson &in) {
        dataNEC = 0xFF02FD;
        requested = true;
        sendStripCode();
        Serial.println("ledstripOn");
    };

    thing["webledstripSwitch"] << [](pson &in) {
        if (in)
        {
            dataNEC = 0xFF02FD;
            requested = true;
            sendStripCode();
        }
        else
        {
            dataNEC = 0xFF02FD;
            requested = false;
            sendStripCode();
        }
        Serial.println("ledstripSwitch");
    };
    delay(100);
    dht.begin();
    irsend.begin();
}

void loop()
{
    thing.handle();
    SinricPro.handle();
    digitalWrite(RELAY, led);
    if (boolStripStatus == false) {
    stripStatus = 0;
  } else if (boolStripStatus == true) {
    stripStatus = 1;
  }
}

void dhtRead() {
    float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);
    Serial.print(F("Humidity: "));
     Serial.print(h);
     Serial.print(F("%  Temperature: "));
     Serial.print(t);
     Serial.print(F("°C "));
     Serial.print(f);
     Serial.print(F("°F  Heat index: "));
     Serial.print(hic);
     Serial.print(F("°C "));
     Serial.print(hif);
     Serial.println(F("°F"));
    delay(250);
}

void IRledstripToggle() {
  dataNEC = 0xFF02FD;
  sendNECcode();
  boolStripStatus = !boolStripStatus;
}
void IRledstripBrighter() {
  dataNEC = 0xFF9867;
  sendNECcode();
}
void IRledstripDarker() {
  dataNEC = 0xFF18E7;
  sendNECcode();
}
void IRavPower() {
  dataNEC = 0x5EA1F807;
  sendNECcode();
}
void IRavVolup() {
  dataNEC = 0x5EA158A7;
  sendNECcode();
}
void IRavVoldown() {
  dataNEC = 0x5EA1D827;
  sendNECcode();
}
void IRavMute() {
  dataNEC = 0x5EA138C7;
  sendNECcode();
}
void IRavDVD() {
  dataNEC = 0x5EA1837C;
  sendNECcode();
}
void IRavTuner() {
  dataNEC = 0x5EA16897;
  sendNECcode();
}
void IRavAux() {
  dataNEC = 0x5EA1AA55;
  sendNECcode();
}
void IRavPresetUp() {
  dataNEC = 0x5EA108F7;
  sendNECcode();
}
void IRavPresetDown() {
  dataNEC = 0x5EA18877;
  sendNECcode();
}

void IRavMov() {
  dataNEC = 0x5EA1F10E;
  sendNECcode();
}

void IRavEnt() {
  dataNEC = 0x5EA1D12E;
  sendNECcode();
}
void sendNECcode() {
  irsend.sendNEC(dataNEC);
}
//void sendSamsungCode() {
//  irsend.sendSAMSUNG(dataSamsung, 32);
//}

void webLamp() {
  http.begin("https://maker.ifttt.com/trigger/espLight/with/key/-Sb96lg3sUpBIi7l4MnOkP4ZtCOslMbsEzT73txf9k");
  http.GET();
}

void sendStripCode() {
  if (requested == !boolStripStatus) {
    irsend.sendNEC(dataNEC);
    boolStripStatus = !boolStripStatus;
  }
}