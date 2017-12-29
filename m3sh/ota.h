
#ifndef _OTA_H_
#define _OTA_H_

//#define OTA_DEBUG

#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

static volatile bool ota;
void ota_init(char *name) {
	//WiFi.onEvent(WiFiEvent);
	WiFi.begin("m3sh", NULL, 0, NULL, true);
	WiFi.softAP(name, NULL, 6, true, 1);
	WiFi.mode(WIFI_AP_STA);
	ArduinoOTA.setMdnsEnabled(true);
	ArduinoOTA.setPassword(prefs.getString("pass", "null").c_str());
	ArduinoOTA.setHostname(name);
	ArduinoOTA
		.onStart([]() {
			ota = true;
			Serial.printf("OTA Started: %d\r\n", ArduinoOTA.getCommand());
			#ifdef _USE_DISP_
			display.clear();
			display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
			display.drawStringMaxWidth(64, 42, 128, "Downloading...");
			display.display();
			#endif
			void end(void); end();
			return;
		})
		.onEnd([]() {
			ota = false;
			Serial.printf("\r\nOTA Ended\r\n");
			#ifdef _USE_DISP_
			display.clear();
			display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
			display.drawStringMaxWidth(64, 32, 128, "DONE!");
			display.display();
			#endif
			return;
		})
		.onProgress([](unsigned int progress, unsigned int total) {
			Serial.printf("OTA Progress: %u/%u (%6.2f)\r\n",
				progress, total, 100.0 * (float) progress / (float)total
			);
			#ifdef _USE_DISP_
			display.drawProgressBar(
				0, 54, 120, 8, map(progress, 0, total, 0, 100)
			);
			display.display();
			#endif
			return;
		})
		.onError([](ota_error_t error) {
			ota = false;
			Serial.printf("OTA Error: %u\r\n", error);
			#ifdef _USE_DISP_
			display.clear();
			display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
			display.drawStringMaxWidth(64, 32, 128, "ERROR! " + String(error));
			display.display();
			#endif
			return;
		});
	ArduinoOTA.begin();
	//if(MDNS.begin(name)) MDNS.addService("http", "tcp", 80);
	ota = false;
	return;
}

/*
	#define PORT 1234
	IPAddress addr(224, 0, 0, 1);
	void WiFiEvent(WiFiEvent_t event) {
	switch (event) {
		case SYSTEM_EVENT_STA_GOT_IP:
			udp.begin(WiFi.localIP(), PORT);
			break;
		default:
			break;
	}
	return;
	}
	class Udp: public WiFiUDP {
	public:
		void msend(char *data, size_t size) {
			beginPacket(addr, PORT);
			write((unsigned char*) data, size);
			endPacket();
			return;
		}
		void mrecv(unsigned size) {
			if (!size) {
				return;
			}
			if (message.size && message.contents) {
				free(message.contents);
			}
			message.rssi = remotePort();
			message.size = size;
			message.contents = (char*) malloc(message.size);
			read(message.contents, message.size);
			characteristic->setValue(
				(unsigned char*) message.contents,
				message.size
			);
			characteristic->notify();
			return;
		}
	}; Udp udp;
*/

#endif
