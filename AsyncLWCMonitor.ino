#include "AsyncLWCMonitor.h"

#include "pages.h"

static const char statusFmt[] = "%s,%i gal,%i,%i,%2.2f%%,%s,%i,%s,%s,%i,%s,%i";

char request[128]; // we don't want this to be on the stack

void writeToCloud(char *dataToStore)
{
  if (!wifiStaConnected) return;
  if (configData.remoteServer[0]=='\0') return;
  
  Serial.printf("\nStarting connection to %s\n",configData.remoteServer);
  if (!httpClient.connect(configData.remoteServer, 80))
  {
    Serial.printf("Connection to %s!\n",configData.remoteServer);
  }
  else
  {
    Serial.printf("Connected to %s!",configData.remoteServer);
    // Make a HTTP request:
    sprintf(request,"GET /storeit.php?%s HTTP/1.0",
            dataToStore);
    Serial.println(request);
    httpClient.println(request);
    httpClient.printf("Host: %s\r\n",configData.remoteServer);
    httpClient.println("Connection: close");
    httpClient.println();

    while (httpClient.connected())
    {
      String line = httpClient.readStringUntil('\n');
      Serial.println(line);
      if (line == "\r")
      {
        Serial.println("headers received");
        break;
      }
    }
    // if there are incoming bytes available
    // from the server, read them and print them:
    while (httpClient.available())
    {
      char c = httpClient.read();
      Serial.write(c);
    }

    httpClient.stop();
  }
}
void handleAsyncStatusUpdate()
{
  Serial.printf("in handleStatusUpdate %s\n",dataTranslated);
  
  float duty=0.0;
  if (offDurration != 0)
  {
    duty=onDurration * 100;
    duty /= offDurration;
  }
  int turb=turbidity;
  float fturb=0.001*turbidity;
  char sturb[16];
  sprintf(sturb,"%0.3f ntu",fturb);
  const char* turbStyle=(turbidity<300)?"#00FF00":(turbidity<1000)?"#FFFF00":"#FF0000";

  int chlFill=chlorine/7;
  float fppm=chlorine;
  fppm/=1623.0;
  char sppm[16];
  sprintf(sppm,"%0.2f ppm",fppm);
  const char* chlStyle=(fppm<0.2)?"#FFFF00":(fppm>1.0)?"#FF0000":"#00FF00";
  sprintf(wsStatusBuffer,statusFmt,dataTranslated,gallons,gph,gpd,duty,turbStyle,turb,sturb,chlStyle,chlFill,sppm,pump);
  ws.textAll(String(wsStatusBuffer));
  sprintf(statusBuffer,"%i,%i,%i,%0.2f,%0.3f,%0.2f,%i\0",gallons,gph,gpd,duty,fturb,fppm,pump);
  writeToCloud(statusBuffer);
}


void handleConfig(AsyncWebServerRequest *request)
{
  sprintf(httpMsg
         ,configFmt
         ,configData.ssid
         ,configData.pass
         ,configData.captive_ssid
         ,configData.captive_pass
         ,configData.sf
         ,configData.dnsName
         ,configData.remoteServer
         );
  request->send(200, "text/html", httpMsg);
}


void reboot()
{
  ESP.restart();
}

void handleStatus(AsyncWebServerRequest *request)
{
  request->send(200, "text/html", statusPage);
  letsReboot=shouldReboot;
}

void handleSet(AsyncWebServerRequest *request)
{
  strcpy(configData.ssid,request->arg("ssid").c_str());
  strcpy(configData.pass,request->arg("pass").c_str());
  strcpy(configData.captive_ssid, request->arg("captive_ssid").c_str());
  strcpy(configData.captive_pass, request->arg("captive_pass").c_str());
  configData.sf=atoi(request->arg("sf").c_str());
  strcpy(configData.dnsName,request->arg("dns_name").c_str());
  strcpy(configData.remoteServer,request->arg("rmtserver").c_str());

  nvs_handle handle;
  esp_err_t res = nvs_open("lwc_data", NVS_READWRITE, &handle);
  Serial.printf("nvs_open %i\n",res);
  res = nvs_set_blob(handle, "lwc_mon_cfg", &configData, sizeof(configData));
  Serial.printf("nvs_set_blob %i\n",res);
  nvs_commit(handle);
  nvs_close(handle);
  
  request->send(200, "text/html", 
    "<html><head></head><body onload=\"location.replace('/config');\"></body></html>\n");
}

void eepromSetup()
{
  nvs_handle handle;

  esp_err_t res = nvs_open("lwc_data", NVS_READWRITE, &handle);
  Serial.printf("nvs_open %i\n",res);
  size_t sz=sizeof(configData);
  res = nvs_get_blob(handle, "lwc_mon_cfg", &configData, &sz);
  Serial.printf("nvs_get_blob %i; size %i\n",res,sz);
  nvs_close(handle);
  
  Serial.printf("ssid=%s\npass=%s\ncaptive_ssid=%s\ncaptive_pass=%s\nSF=%i\ndnsName=%s\nermoteServer%s\n"
               ,configData.ssid
               ,configData.pass
               ,configData.captive_ssid
               ,configData.captive_pass
               ,configData.sf
               ,configData.dnsName
               ,configData.remoteServer
               );
}

void webServerSetup()
{
  server.on("/details", HTTP_GET,  [](AsyncWebServerRequest *request) {
    //request->sendHeader("Connection", "close");
    request->send(200, "text/html", rootIndex);
  });
  server.on("/",handleStatus);
  server.on("/config",handleConfig);
  server.on("/set",handleSet);
  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/html", 
          "<html><head></head><body onload=\"location.replace('/');\">Rebooting</body></html>\n");
      shouldReboot=true;
  });
  server.on("/ota", HTTP_GET, [](AsyncWebServerRequest *request) {
    //request->sendHeader("Connection", "close");
    request->send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
    //request->sendHeader("Connection", "close");
    request->send(200, "text/html", "<html><head></head><body onload=\"location.replace('/');\">Rebooting...</body></html>\n");
    shouldReboot=true;
  }, 
  [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) 
  {
    timeToConnect=millis()+3600000L;
    Serial.printf("index: %u len: %u\n",index, len);
    if (!index) // upload start
    {
      Serial.printf("Update: %s %u\n", filename.c_str(),len);
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    }
     
    if (len) // write
    {
      /* flashing firmware to ESP*/
      if (Update.write(data, len) != len) {
        Update.printError(Serial);
      }
    } 
    
    if (final) 
    {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", index);
      } else {
        Update.printError(Serial);
      }
    }
  });
  server.begin();
}


void loraSend(byte *rgch,int len)
{
  delay(100);
  Serial.printf("lora sending %i bytes ",len);
  for (int i=0;i<len;i++)
  {
    Serial.printf(" %02x",rgch[i]);
  }
  Serial.println();
  LoRa.setTxPower(20,RF_PACONFIG_PASELECT_PABOOST);
  LoRa.beginPacket();
  for (int i=0;i<len;i++)
  {
    LoRa.write(*rgch++);
  }
  LoRa.endPacket();
  LoRa.receive();
}

void loraDump(int packetSize)
{
  Serial.println(packetSize);
  int i=0;
  while (LoRa.available())
  {
    Serial.printf("%02x ",LoRa.read());
    if (i++==16)
    {
      i=0;
      Serial.println();
    }
  }
  Serial.println();
}

void onLoRaReceive(int packetSize)
{
  if (packetSize == 0) return;          // if there's no packet, return
  if (packetSize>20)
  {
    loraDump(packetSize);
    return;
  }
  
  byte buff[100];
  for (int i=0;LoRa.available();i++)
  {
    buff[i]=LoRa.read();
    buff[i+1]=0;
  }
  Serial.printf("lora receive %i bytes ",packetSize);
  for (int i=0;i<packetSize;i++)
  {
    Serial.printf(" %02x",buff[i]);
  }
  Serial.println();
  gp.processRecv((void*)buff,packetSize);
}

void logger(const char *msg)
{
  Serial.print(msg);
}

// 0123456789012345678901234567890
//"raw tank reading 12345n" - len=23
//                  ^    ^=len-1
//                  |=17

void translate(byte *data,int len)
{
  unsigned *pi=(unsigned*)(&data[1]);
  if (data[0]=='c') // current is flowing
  {
    sprintf(dataTranslated,"current is flowing %i %i",pi[0],pi[1]);
  } else if (data[0]=='n') // no current flowing
  {
    sprintf(dataTranslated,"no current flowing %i %i",pi[0],pi[1]);
  } else if (data[0]=='r') //raw tank reading 
  {
    unsigned short *ps=(unsigned short*)(pi);
    sprintf(dataTranslated,"raw tank reading %i",*ps);
  } else if (data[0]=='W') // WATER USAGE VERY HIGH
  {
    sprintf(dataTranslated,"WATER USAGE VERY HIGH");
  }
}

void gotData(byte *data,int len)
{
  translate(data, len);
  unsigned *pu=(unsigned *)(&data[1]);
  long minutes = (millis()-startTime)/60000;
  Serial.println((const char *)dataTranslated);
  unsigned short *ps = (unsigned short*)(&data[1]);
  if (*data=='r') // raw tank level
  {
    lastRealData=millis();
    lastVolume=millis();
    if (*ps > 3310) return;
    total+=*ps;
    count++;
    avg=((total / count) * 719) / 100;
    gallons=(*ps *719) / 100;
    
    while(ndex <= minutes)
    {
      rgHour[ndex % 60]=*ps;
      rgDay[ndex % 1440]=*ps;
      ndex++;
    }
    gpd=(719 * (rgDay[(ndex<1440)?0:ndex%1440]-*ps)) / 100;
    int tgph=(719 * (rgHour[(ndex<60)?0:ndex%60]-*ps)) / 100;
    if (gph<0 && tgph>0)
    {
      turbidity=0;
      chlorine=0;
    }
    gph=tgph;
    sprintf(msg,"%i gal\nGPH %i\nGPD %i",gallons,gph,gpd);
  }
  else if (*data=='T') // turbidity
  {
    lastRealData=lastTurbidity=millis();
    turbidity=*ps;
  }
  else if (*data=='C') // chlorine
  {
    lastRealData=lastChlorine=millis();
    chlorine=*ps;
  }
  else if (*data=='P') // pump
  {
    lastRealData=millis();
    pump=*ps;
  }
  else if (*data=='c') // current is flowing
  {
    //onDurration=pu[0];
    offDurration=pu[1];
    lastRealData=millis();
    fOn=true;
  }
  else if (*data=='n') // no current flowing
  {
    onDurration=pu[0];
    if (pu[1]>offDurration)
    {
      offDurration=pu[1];
    }
    lastRealData=millis();
    if (fOn)
    {
      total=0;
      count=0;
      fOn=false;
    }
    if (highUsageAlarm)
    {
      highUsageAlarm=false;
      buzzer(false);
    }
  }
  else if (*data=='W') // WATER USAGE VERY HIGH
  {
    lastRealData=millis();
    highUsageAlarm=true;
  }
  char *pch=dataBuff;
  for (int i=0;i<1440;i++)
  {
    if (i)
    {
      *pch++=',';
    }
    pch+=sprintf(pch,"%d",rgDay[(ndex+i)%1440]);
  }
  sprintf(rootIndex,rootFmt
         ,dataTranslated
         ,gallons,total,count
         ,gph
         ,gpd
         ,onDurration
         ,offDurration
         , minutes
         , WiFi.SSID().c_str()
         , rgIPTxtSTN
         , lastVolume, lastTurbidity, lastChlorine
         ,dataBuff
         );
  handleAsyncStatusUpdate();
}

void connected()
{
  Serial.println("connected");
  _connected=true;
}

void disconnected()
{
  Serial.println("disconnected");
  _connected=false;
}

int myprintf(const char *format, va_list list)
{
  return Serial.printf(format,list);
}

void duckDNSSetup()
{
  if (!wifiStaConnected) return;
  if (strlen(configData.dnsName)==0) return;

  Serial.println("\nStarting connection to server...");
  if (!client.connect("www.duckdns.org", 443))
  {
    Serial.println("Connection failed!");
  }
  else
  {
    Serial.println("Connected to server!");
    // Make a HTTP request:
    char request[128];
    sprintf(request,"GET https://www.duckdns.org/update/%s/%s/%s HTTP/1.0",
            configData.dnsName,duckDNSToken,rgIPTxtSTN);
    Serial.println(request);
    client.println(request);
    client.println("Host: www.duckdns.org");
    client.println("Connection: close");
    client.println();

    while (client.connected())
    {
      String line = client.readStringUntil('\n');
      Serial.println(line);
      if (line == "\r")
      {
        Serial.println("headers received");
        break;
      }
    }
    // if there are incoming bytes available
    // from the server, read them and print them:
    while (client.available())
    {
      char c = client.read();
      Serial.write(c);
    }

    client.stop();
  }

}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      ws.textAll(String(wsStatusBuffer));
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void setup() 
{
  sprintf(dataTranslated,"NoData");
  pinMode(0,INPUT_PULLUP);
  pin0State=pin0Value=digitalRead(0);
  pinMode(RED_LED,OUTPUT);
  digitalWrite(RED_LED,LOW);
  ledcAttachPin(RED_LED,RED_CHANNEL);
  ledcSetup(RED_CHANNEL,5000,8);
  ledcWrite(RED_CHANNEL,0);
  pinMode(GREEN_LED,OUTPUT);
  digitalWrite(GREEN_LED,LOW);
  ledcAttachPin(GREEN_LED,GREEN_CHANNEL);
  ledcSetup(GREEN_CHANNEL,5000,8);
  ledcWrite(GREEN_CHANNEL,0);
  pinMode(BLUE_LED,OUTPUT);
  digitalWrite(BLUE_LED,LOW);
  ledcAttachPin(BLUE_LED,BLUE_CHANNEL);
  ledcSetup(BLUE_CHANNEL,5000,8);
  ledcWrite(BLUE_CHANNEL,0);
  pinMode(BUZZER_PIN,OUTPUT);
  digitalWrite(BUZZER_PIN,HIGH);
  
  esp_log_set_vprintf(&myprintf);
  sprintf(msg,"%s","no data");
  
  Heltec.begin(true /*DisplayEnable Enable*/, 
               true /*Heltec.LoRa Disable*/, 
               true /*Serial Enable*/, 
               true /*PABOOST Enable*/, 
               BAND /*long BAND*/);
               
  Serial.begin(115200);
  
  Heltec.display->flipScreenVertically();
  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setLogBuffer(5, 64);
  Heltec.display->clear();
  Heltec.display->drawStringMaxWidth(0, 0, 128, "about to set up LoRa");
  Heltec.display->display();
  
  eepromSetup();
  
  LoRa.setSpreadingFactor(configData.sf);
  LoRa.receive();

  Serial.println("Heltec.LoRa init succeeded.");

  wifiAPSetup();
  duckDNSSetup();

  MDNS.begin(configData.captive_ssid);
  MDNS.addService("http", "tcp", 80);

  sprintf(rootIndex,rootFmt,"No Data", 0, 0, 0, 0, 0, 0, 0, 0l, WiFi.SSID().c_str(), rgIPTxtSTN,0L,0L,0L,"0");

  initWebSocket();
  webServerSetup();
  
  Serial.println("back in setup after WiFi setup");
  gp.setSendMethod(&loraSend);
  gp.setOnReceive(&gotData);
  gp.setOnConnect(&connected);
  gp.setOnDisconnect(&disconnected);
  gp.setLogMethod(&logger);
  gp.setMonitorMode(true);
  gp.setTimeout(2000);

  startTime=millis();

}

bool redLEDOn=false;

void redLED(int dutyCycle, bool fOn)
{
  if (fOn != redLEDOn)
  {
    ledcWrite(RED_CHANNEL,fOn?dutyCycle:0);
    redLEDOn=fOn;
  }
}

bool greenLEDOn=false;

void greenLED(int dutyCycle, bool fOn)
{
  if (fOn != greenLEDOn)
  {
    ledcWrite(GREEN_CHANNEL,fOn?dutyCycle:0);
    greenLEDOn=fOn;
  }
}

bool buzzerOn=false;

void buzzer(bool fOn)
{
  if (fOn != buzzerOn)
  {
    digitalWrite(BUZZER_PIN, fOn?LOW:HIGH);
    buzzerOn=fOn;
  }
}

bool debouncePin(int pin, int &pinState, long &stamp, int &pinValue)
{
  if (stamp==0)
  {
    pinState=digitalRead(pin);
    if (pinState != pinValue)
    {
      stamp=millis();
    }
    return false;
  }
  
  // we know that we're timing an event (i.e., stamp != 0)
  
  if (millis()-stamp > 30)
  {
    stamp = 0;
    if (pinState == digitalRead(pin))
    {
      pinValue = pinState;
      return true;
    }
  }
  return false;
}

void loop()
{
  if (letsReboot) 
  {
    delay(1000);
    reboot();
  }
  
  onLoRaReceive(LoRa.parsePacket());
  ws.cleanupClients();
  
  if (debouncePin(0, pin0State, stamp, pin0Value))
  {
    if (pin0Value==LOW)
    {
      testMode = !testMode;
      buzzer(true);
    }
    else
    {
      buzzer(false);
    }
  }
  
  if (displayTime<=millis())
  {
    displayTime=millis()+300;
    if (testMode)
    {
      displayIPs(x, y, false);
    }
    else
    {
      Heltec.display->clear();
      Heltec.display->drawString(x, y, msg);
      Heltec.display->display();
    }
    x = (x+xinc);
    if (x>=50)
    {
      xinc=-1;
    }
    else if (x<=0)
    {
      xinc=1;
    }
    y = (y+yinc);
    if (y>=11)
    {
      yinc=-1;
    }
    else if (y<=0)
    {
      yinc=-yinc;
    }
  }
  if (highUsageAlarm) // flashing red and buzzer
  {
    greenLED(0,false);
    redLED(255,millis()%2000<1000);
    buzzer(millis()%2000<1000);
  }
  else if (millis()-lastRealData > 65000) // flashing yellow - no buzzer
  {
    redLED(5,millis()%2000<1000);
    greenLED(7,millis()%2000<1000);
//    buzzer(false);
  }
  else //green - no buzzer
  {
    redLED(0,false);
    greenLED(5,true);
//    buzzer(false);
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    reconnectWiFi();
  }
}
