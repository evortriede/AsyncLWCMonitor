void *getLocalHotspot()
{
   Serial.println("scan start");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) 
  {
      Serial.println("no networks found");
  } else 
  {
    Serial.print(n);
    Serial.println(" networks found");
    int i=0;
    int j=0;
    int nHotspots=sizeof(savedHotspots)/sizeof(savedHotspots[0]);

    // first look for the configured ssid in the saved hotspots
    for (i=0; i<nHotspots; i++)
    {
      if (0==strcmp(configData.ssid,savedHotspots[i].ssid)) // found in saved hotspots
      {
        Serial.printf("Configured hotspot %s is saved hotspot %i\n",configData.ssid,i);
        for (j=0;j<n;j++) // look for it in network
        {
          const char* ssid=WiFi.SSID(j).c_str();
          Serial.printf("looking at %s\n",ssid);
          if (0==strcmp(configData.ssid,ssid))
          {
            Serial.printf("found configured hotspot %s in saved hotspots and in scan results\n",configData.ssid);
            return &savedHotspots[i];
          }
        }
        break; // in saved hotspots but not in network
      }
    }
    for (i = 0; i < n; ++i) 
    {
      String id = WiFi.SSID(i);
      Serial.printf("looking for %s\n",id.c_str());
      for (int j=0;j<nHotspots;j++)
      {
        Serial.printf("looking at %s\n",savedHotspots[j].ssid);
        if (strcmp(savedHotspots[j].ssid,id.c_str())==0)
        {
          Serial.println("found");
          return &savedHotspots[j];
        }
      }
    }
  }
  return (void *)&configData;
}

void wifiSTASetup()
{
  saved_hotspot_t *hotspot = (saved_hotspot_t*)getLocalHotspot();
  
  Serial.print("Connecting to ");
  Serial.println(hotspot->ssid);

  if (configData.finalOctet != 0)
  {
    IPAddress addr(192, 168, 10, configData.finalOctet);
    WiFi.config(addr);
  }
  WiFi.begin(hotspot->ssid, hotspot->pass);
  Serial.println("");

  long wifiTimeOut=millis()+30000l;
  // Wait for connection
  while ((WiFi.status() != WL_CONNECTED) && (millis()<wifiTimeOut)) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi STA setup timed out");
  }
  else
  {
    wifiStaConnected=true;
    Serial.println("");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

void displayIPs(int x, int y, boolean fSerialPrint)
{

  IPAddress myIP = WiFi.softAPIP();
  sprintf(rgIPTxtAP,"%u.%u.%u.%u",myIP[0],myIP[1],myIP[2],myIP[3]);
  
  factory_display.clear();
  factory_display.drawStringMaxWidth(x, y, 128, rgIPTxtAP);

  if (fSerialPrint)
  {
    Serial.print("AP IP address: ");
    Serial.println(rgIPTxtAP);
  }

  myIP = WiFi.localIP();
  sprintf(rgIPTxtSTN,"%u.%u.%u.%u",myIP[0],myIP[1],myIP[2],myIP[3]);
  factory_display.drawStringMaxWidth(x, y+16, 128, rgIPTxtSTN);

  factory_display.display();

  if (fSerialPrint)
  {
    Serial.print("Local IP address: ");
    Serial.println(rgIPTxtSTN);
  }
}

void wifiAPSetup()
{
  Serial.println("Configuring access point...");
  WiFi.mode(WIFI_AP_STA);
  
  wifiSTASetup();

  Serial.printf("Setting up soft AP for %s\n",configData.captive_ssid);

  WiFi.softAPConfig(myAddress, myAddress, subNet);
  
//  WiFi.softAP("LWCMonX","");
  WiFi.softAP(configData.captive_ssid,configData.captive_pass);

  displayIPs(0,0,true);
  
}

void reconnectWiFi()
{
  if (reconnectTime==0)
  {
    reconnectTime=millis();
    WiFi.disconnect();
    return;
  }
  if (millis()-reconnectTime < 60000) return;
  wifiAPSetup();
  reconnectTime=0;
}
