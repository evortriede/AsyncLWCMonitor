# AsyncLWCMonitor
 London Water Co-op monitor for Heltec ESP32 LoRa V2
 
## Overview
 
 ![The Monitor](/assets/PXL_20220309_233631942.jpg)

The London Water Cooperative (LWC) water treatment plant (WTP) has no Internet or Cell access. There is a microcontroller at the WTP (CurrentMonitor) that communicates with an off-site microcontroller (CurrentRecorder) using LoRa. The firmware that this project (AsyncLWCMonitor hensforth the Monitor) implements, listens in on the LoRa conversation between the WTP and off-site location, tracking and displaying metrics.

All of this firmware runs on Heltec ESP32 LoRa V2 microcontrollers which have an OLED display. The Monitor uses the OLED to display the current water storage tank level, gallons of water used in the last 60 minutes and the gallons of water used in the last 1440 minutes. The Monitor also controls a RGB LED and a buzzer. The LED is normally green but will turn yellow if no data has been received for a minute and red (along with beeping the buzzer on and off every 1/2 second) if a High Water Usage message is received over LoRa.

The Monitor also joins a local hotspot (configurable) as well as acting as a hotspot (access point) itself. The root page is an information display with a link to a "details" page (more information) with links to an OTA firmware upload and to a configuration page. The information on the root page is the same as displayed on the OLED plus the duty cycle of the pressure pump at the WTP (an indicator of instantaneous water usage) and the last message received over the LoRa link. Also displayed on the root page is a graphic (thermometer) for tank volume and when the WTP is running, similar graphics for the turbidity and chlorine content of the water being produced.

## Hardware

![Monitor Exploded View](/assets/PXL_20220310_051504749.jpg)

The Monitor is enclosed in the case that the Heltec microcontroller is delivered in. The microcontroller, LED, buzzer, a header for the battery and a header for a switch to disable the buzzer are mounted on a 23x24 stripboard. 

![The Monitor Stripboard](/assets/LWCMonitorVeeCAD.jpg)

Since very few of the microcontroller pins are used and those are on the corners of the board, only three pins are soldered to each corner. In order for it to fit into the case, the plastic on the bottom side of the pins is removed after soldering and the pins themselves are trimmed in order to give adequate clearance.

![Monitor Clearance](/assets/PXL_20220310_004426639.jpg)

## Firmware

### Features

- Setup
  - Read configuration from non-volatile storage
  - Create WiFi Access Point
  - Join a local Access Point
    - Scan available access points
    - If one matches an entry from a compiled list, join it
    - Otherwise, join configured AP
  - If local AP is connected and a DNS name is configured, update duckdns.org with local IP
    - Note: duckdns.org is only given the local (i.e., 192.168... or 10.2...) IP address so a DNS name can be used to find the Monitor within the local network.
  - Start LoRa
- Loop
  - Check for LoRa message (High Water Usage, Tank Level, Chlorine Pump Setting, Chlorine reading, Turbidity Reading, Pressure pump on, Pressure pump off)
    - On High Water Usage message, turn LED to red and turn on beeper
    - On Tank Level message, store value in circular buffer and compute usage for previous hour and day.
    - On Pressure pump on/off messages, track and compute duty cycle.
    - On all other messages, store values (reset beeper if going and set LED to green)
  - Update connected WiFi clients
  - Update OLED display
  - If local AP is connected and Remote Server address is configured, send HTTP request with metrics
  - If a minute has passed w/o a LoRa message, set LED to yellow.
  - Handle any HTTP requests

### Remote Access

The Monitor is designed to work within a local WiFi network or without any network at all. While one could punch a hole in a firewall to allow access to an instance of the Monitor in order to access its web pages, another solution has been provided which involves a remote web server that is accessible by an instance of the Monitor. On that server, two PHP scripts are installed to persist and retrieve metrics. If the Monitor is configured with a Remote Server address, whenever the metrics change it will send an HTTP GET request with the metrics to be persisted to the server as follows:

```
GET /storeit.php?<data to be persisted> HTTP/1.0
```

The data is a comma separated list of metric values. For details about the format, please refer to the code as changes would likely not appear here as they are inevitably made.

With that in place anyone can load `lwcmon.html` and it will display the metrics just as if they were accessed through the Monitor's root page. Note that the server address in `lwcmon.html` must match that configured into the Monitor instance that is persisting the data. Apart from having the correct server address, the server must have `getit.php` loaded for the whole thing to work.

Note: there should only ever be one instance of the Monitor persisting data to any given server. Also, the instance of the Monitor that is persisting data can be a naked Heltec ESP32 LoRa V2 (i.e., it doesn't need the LED or buzzer). Here's what `lwcmon.html` looks like when the WTP is not running:

![lwcmon.html](/assets/lwcmon-html.jpg)

And here's what it looks like when the WTP is running (note the negative GPH metric):

![lwcmon.html when WTP is running](/assets/lwcmon-html-WTP-running.jpg)

So, `lwcmon.html` creates a message listener function (i.e., `window.addEventListener("message", onMessage);`) and contains an `iframe` that loads `getit.php` once per minute. `getit.php` loads the data that was persisted by `storeit.php` and renders something like the following which ends up in the `iframe`:

```
<html>
<head>
</head>
<body>
<script>
  window.parent.postMessage(<data persisted by storeit.php>, "*");
</script>
</body>
</html>
```

The message listener function renders the data as shown above.