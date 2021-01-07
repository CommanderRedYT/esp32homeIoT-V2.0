#include <Arduino.h>
//Start User Settings

//#define _DEBUG_ //Uncomment if you want to enable debug mode

    //WiFi
    const char *ssid = "";
    const char *password = "";
    const char *WIFI_SSID = "";
    const char *WIFI_PASS = "";

    //thinger.io
    #define USERNAME ""
    #define DEVICE_ID ""
    #define DEVICE_CREDENTIAL ""

    //sinric.pro
    #define APP_KEY ""    // Should look like "de0bxxxx-1x3x-4x3x-ax2x-5dabxxxxxxxx" //Sinric.pro
    #define APP_SECRET "" // Should look like "5f36xxxx-x3x7-4x3x-xexe-e86724a9xxxx-4c4axxxx-3x3x-x5xe-x9x3-333d65xxxxxx"
    #define LIGHT_ID ""   // Should look like "5dc1564130xxxxxxxxxxxxxx"

    //WakeOnLan
    const char *MACAddress = "";

    //Custom HTTP GET-Request (I use it in combination with IFTTT to control a TP-Link Smartplug)
    const char *url = "";

    //DHT-config
    #define DHTPIN 19
    #define DHTTYPE DHT11

    //External Relay
    #define RELAY 4

    //IR-pin (connected to transistor to control ir led)
    const uint16_t SEND_PIN = 5;

    //Serial Monitor Baud Rate
    #define BAUD_RATE 115200


//End User Settings