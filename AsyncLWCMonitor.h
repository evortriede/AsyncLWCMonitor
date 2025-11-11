#include "Arduino.h"

#include <fsm.h>

#include <GenericProtocol.h>

#include "heltec.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <esp_log.h>
#include <nvs.h>
#include <ESPmDNS.h>
#include <Wire.h>  
#include "HT_SSD1306Wire.h"
#include "LoRaWan_APP.h"


#define CONVERSION_FACTOR_MULTIPLIER 698
#define CONVERSION_FACTOR_DENOMINATOR 100

typedef struct
{
  char ssid[25];
  char pass[25];
  char captive_ssid[25];
  char captive_pass[25];
  char dnsName[25];
  int sf;
  char remoteServer[64];
  float chlConversionFactor;
  int finalOctet;
} config_data_t;

config_data_t configData={"TP-Link_32E6","","ChangeMe","admin","LWCMonitor",9,"\0",2180.0,0};

typedef struct 
{
  char ssid[25];
  char pass[25];
} saved_hotspot_t;

#include <hotspots.h>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
WiFiClientSecure client;
WiFiClient httpClient;

char rootIndex[10240];

bool fTextAll=false;
unsigned short turbidity=0;
unsigned short chlorine=0;
unsigned short pump=0;
char dataTranslated[100]={0};
char msg[100];
int onDurration=0;
int offDurration=0;
int gpd=0;
int gph=0;
int avg=0;
int gallons=0;
char statusBuffer[512];
char wsStatusBuffer[512];
char httpMsg[4096];

long timeToConnect=0;

bool wifiStaConnected=false;

char rgIPTxtAP[32];
char rgIPTxtSTN[32];

IPAddress myAddress(192, 168, 4, 4);
IPAddress subNet(255, 255, 255, 0);

long reconnectTime=0;

GenericProtocol gp;

bool _connected=false;

int total;
int count;
bool fOn=false;
int rgHour[60];
int rgDay[1440];
int ndex=0;
long startTime=0;
bool highUsageAlarm=false;
long lastRealData=0;
bool testMode=false;
int pin0State=HIGH;
int pin0Value=HIGH;

char dataBuff[1441*5];

#define RED_LED_PIN 12
#define GREEN_LED_PIN 13
#define BLUE_LED_PIN 17
#define BUZZER_PIN 2
#define RED_CHANNEL 0
#define GREEN_CHANNEL 1
#define BLUE_CHANNEL 2

#define BAND    915E6  //you can set band here directly,e.g. 868E6,915E6

int x=0;
int y=0;
int xinc=1;
int yinc=1;
long stamp=0;
long displayTime=0;
bool shouldReboot=false;
bool letsReboot=false;
long lastVolume=0;
long lastTurbidity=0;
long lastChlorine=0;

void reconnectWiFi();
void wifiAPSetup();
void displayIPs(int x, int y, boolean fSerialPrint);
void wifiSTASetup();
void *getLocalHotspot();

/* binary_to_base64:
 *   Description:
 *     Converts a single byte from a binary value to the corresponding base64 character
 *   Parameters:
 *     v - Byte to convert
 *   Returns:
 *     ascii code of base64 character. If byte is >= 64, then there is not corresponding base64 character
 *     and 255 is returned
 */
unsigned char binary_to_base64(unsigned char v);

/* base64_to_binary:
 *   Description:
 *     Converts a single byte from a base64 character to the corresponding binary value
 *   Parameters:
 *     c - Base64 character (as ascii code)
 *   Returns:
 *     6-bit binary value
 */
unsigned char base64_to_binary(unsigned char c);

/* encode_base64_length:
 *   Description:
 *     Calculates length of base64 string needed for a given number of binary bytes
 *   Parameters:
 *     input_length - Amount of binary data in bytes
 *   Returns:
 *     Number of base64 characters needed to encode input_length bytes of binary data
 */
unsigned int encode_base64_length(unsigned int input_length);

/* decode_base64_length:
 *   Description:
 *     Calculates number of bytes of binary data in a base64 string
 *     Variant that does not use input_length no longer used within library, retained for API compatibility
 *   Parameters:
 *     input - Base64-encoded null-terminated string
 *     input_length (optional) - Number of bytes to read from input pointer
 *   Returns:
 *     Number of bytes of binary data in input
 */
unsigned int decode_base64_length(const unsigned char input[]);
unsigned int decode_base64_length(const unsigned char input[], unsigned int input_length);

/* encode_base64:
 *   Description:
 *     Converts an array of bytes to a base64 null-terminated string
 *   Parameters:
 *     input - Pointer to input data
 *     input_length - Number of bytes to read from input pointer
 *     output - Pointer to output string. Null terminator will be added automatically
 *   Returns:
 *     Length of encoded string in bytes (not including null terminator)
 */
unsigned int encode_base64(const unsigned char input[], unsigned int input_length, unsigned char output[]);

/* decode_base64:
 *   Description:
 *     Converts a base64 null-terminated string to an array of bytes
 *   Parameters:
 *     input - Pointer to input string
 *     input_length (optional) - Number of bytes to read from input pointer
 *     output - Pointer to output array
 *   Returns:
 *     Number of bytes in the decoded binary
 */
unsigned int decode_base64(const unsigned char input[], unsigned char output[]);
unsigned int decode_base64(const unsigned char input[], unsigned int input_length, unsigned char output[]);
unsigned int collaps_base64(unsigned char text[]);

byte *favicon;
int faviconlen;
