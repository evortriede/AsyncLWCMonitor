#include <fsm.h>

#include <GenericProtocol.h>

#include "Arduino.h"
#include "heltec.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <esp_log.h>
#include <nvs.h>

typedef struct
{
  char ssid[25];
  char pass[25];
  char captive_ssid[25];
  char captive_pass[25];
  char dnsName[25];
  int sf;
  char remoteServer[64];
} config_data_t;

config_data_t configData={"","","ChangeMe","admin","LWCMonitor",9,"\0"};

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

#define RED_LED 12
#define GREEN_LED 13
#define BLUE_LED 17
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
