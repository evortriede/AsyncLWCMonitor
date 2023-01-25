# AsyncLWCMonitor
 London Water Co-op monitor for Heltec ESP32 LoRa V2
 
 ![The Monitor](/assets/PXL_20220309_233631942.jpg)

The London Water Cooperative (LWC) water treatment plant (WTP) has no Internet or Cell access. There is a microcontrollwe at the WTP (CurrentMonitor) that communicates with a microcontroller off site (CurrentRecorder) using LoRa. The firmware that this project (AsyncLWCMonitor hensforth the Monitor) implements, listens in on the LoRa conversation between the WTP and off site location, tracking and displaying metrics.

All of these firmwares run on Heltec ESP32 LoRa V2 microcontrollers which have an OLED display. The Monitor uses the OLED to display the current water storage tank level, gallons of water used in the last 60 minutes and the gallons of water used in the last 1440 minutes The Monitor also controls a RGB LED and a buzzer. The LED is normally green but will turn yellow if no data has been received for a minute and red (along with beeping the buzzer on and off every 1/2 second) if a High Water Usage message is received.

The Monitor also joins a local hotspot (configurable) as well as acting as a hotspot itself. The root page is an information display with a link to a "details" page (more information) with links to an OTA firmware upload and to a configuration page. The information on the root page is the same as displayed on the OLED plus the duty cycle of the pressure pump at the WTP (an indicator of instantanious water usage) and the last message received over the LoRa link. Also displayed on the root page is a graphic (thermometer) for tank volume and when the WTP is running, similar graphics for the turbidity and chlorine content of the water being produced.
