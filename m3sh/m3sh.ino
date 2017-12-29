
#include <Wire.h>

#include <Preferences.h>
Preferences prefs;

//extern uint32_t fletcher32(uint16_t const*, size_t);
void hexchar(Stream &s, char c) {
	s.print(c >= 0x20 && c < 0x7f ? c : '.');
	return;
}

void hexdump(Stream &s, char *data, size_t size) {
	const char width = 16;
	for(int i = 0; i < size; i++) {
		if (!(i % width)) {
			s.print("\t");
			for(int j=width; i && j>0; j--) {
				hexchar(s, data[i-j]);
				continue;
			}
			s.println();
			for(int j=0xFFFF; j>0; j >>= 4) {
				s.print(" ");
				continue;
			}
			s.print(i, HEX);
			s.print("\t");
		}
		if (data[i] < 0x10) {
			s.print(" ");
		}
		s.print((char) data[i], HEX);
		s.print(" ");
		continue;
	}
	for(int i=0; i<(width-(size%width)); i++) {
		s.print("   ");
		continue;
	}
	s.print("\t");
	for(int i=size%width; i>0; i--) {
		hexchar(s, data[size - i]);
		continue;
	}
	s.println();
	return;
}

#define _USE_DISP_
#define CONV(x) ( 0 \
	| ((((x) >> 24) & 0xff) <<  0) \
	| ((((x) >> 16) & 0xff) <<  8) \
	| ((((x) >>  8) & 0xff) << 16) \
	| ((((x) >>  0) & 0xff) << 24) \
)

#ifdef _USE_DISP_
#include <SSD1306.h>
#endif
SSD1306 display(0x3C, 4, 15);
static volatile unsigned long last;

#include "sx.h"
#include "ble.h"
#include "ota.h"

#define BUTTON 0

void send_beacon(void) {
	last = 0;
	return;
}

void setup() {
	Serial.begin(115200);
	Serial.setDebugOutput(true);
	//WiFi.begin("m3sh");

	char name[32] = { 0 };
	uint64_t id = ESP.getEfuseMac();
	sprintf(name, "%s-%08x", "m3sh", (uint32_t) (id >> 16));

	prefs.begin(name, false);
	prefs.clear();
	if(prefs.getULong("magic", 0) != _LORA_MAGIC_) {
		prefs.clear();
		prefs.putULong("magic", _LORA_MAGIC_);
		prefs.putULong("dly", 9999);
	}

	ble_init(name);
	ota_init(name);
	sx_init(id);

	#ifdef _USE_DISP_
	pinMode(16, OUTPUT);
	digitalWrite(16, LOW);
	delay(50);
	digitalWrite(16, HIGH);
	display.init();
	display.flipScreenVertically();
	display.setFont(ArialMT_Plain_10);
	display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
	display.drawStringMaxWidth(64, 32, 128, name);
	display.display();
	#endif

	uint32_t addr = CONV(self.addr);
	notify(ble.addr, (unsigned char*) &addr, sizeof(self.addr));
	//memset(&message, 0, sizeof(message));
	// a5a5 44e9 43a4 ae30
	// a5a5 30e5 43a4 ae30
	// a5a5 a85f 1aa4 ae30
	// a5a5 1070 19a4 ae30
	// a5a5 94c6 27a4 ae30
	// a5a5 a8c0 27a4 ae30
	Serial.println(name);
	Serial.println(sizeof(packet_t));
	Serial.printf("%08x\r\n", self.addr);
	pinMode(BUTTON, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(BUTTON), send_beacon, RISING);
	packet_t *pkg = packet_make(
		self.addr, UINT32_MAX, (packet_type_t) 0xFF, 0x4321
	);
	Serial.printf("# %08x:", packet_prepare(pkg, 0, NULL));
	list_push(&self.queue.out, list_init(pkg, sizeof(packet_t)));
	last = millis();
	return yield();
}

void end(void) {
	delete ble.device.manufacturer;
	delete ble.device.model;
	delete ble.device.pass;
	delete ble.device.batt;
	delete ble.device.service;
	delete ble.prefs.beacon;
	delete ble.prefs.button;
	delete ble.prefs.service;
	delete ble.bcast;
	delete ble.data;
	delete ble.addr;
	delete ble.service;
	delete ble.server;
	LoRa.end();
}

void loop() {
	ArduinoOTA.handle();
	if(Serial.available()) {
		Serial.printf("m, l, m-l: %ld %ld %ld \r\n",
			millis(), last, millis() - last);
		Serial.printf("%p %p %p\r\n", self.queue.in, self.queue.out, self.queue.msg);
		Serial.print(".");
		Serial.flush();
		Serial.read();
	}
	if(ota) {
		return;
	}
	if(list_size(self.queue.in) || self.queue.in) {
		list_t *node = list_find(self.queue.in, list_last(self.queue.in));
		node = list_del(&(node->next ? node->next : self.queue.in));
		msg_t *message = (msg_t*) node->data;
		if(message->rdy) {
			packet_t *pkg = (packet_t*) calloc(1, sizeof(packet_t));
			memcpy(pkg, message->pkg, sizeof(packet_t));
			list_t *item = list_init(pkg, (unsigned) message->rssi);
			if(pkg->dst == self.addr || pkg->dst == UINT32_MAX) {
				list_push(&self.queue.msg, item);
			} else if(pkg->ttl > 0) {
				pkg->ttl -= 1;
				packet_prepare(pkg, 0, NULL);
				item->size = sizeof(packet_t);
				list_push(&self.queue.out, item);
			}
			notify(ble.rssi, (unsigned char*) &message->rssi, sizeof(message->rssi));
		} else {
			hexdump(Serial, message->data, message->size);
			Serial.printf("chksum failure: %08x %08x \r\n",
				message->pkg->checksum, packet_getsum(message->pkg)
			);
		}
		free(message->data);
		free(node->data);
		free(node);
		yield();
	}
	if(list_size(self.queue.out) || self.queue.out) {
		list_t *node = list_find(self.queue.out, list_last(self.queue.out));
		node = list_del(&(node->next ? node->next : self.queue.out));
		packet_t *pkg = (packet_t*) node->data;
		hexdump(Serial, (char*) node->data, node->size);
		LoRa.beginPacket();
		LoRa.write((unsigned char*) node->data, node->size);
		LoRa.endPacket();
		LoRa.receive();
		//uint32_t sum = pkg->checksum;
		//Serial.printf("chksum: %08x %08x \r\n", sum, packet_getsum(pkg));
		free(node->data);
		free(node);
		yield();
	}
	if(list_size(self.queue.msg) || self.queue.msg) {
		list_t *node = list_find(self.queue.msg, list_last(self.queue.msg));
		node = list_del(&(node->next ? node->next : self.queue.msg));
		packet_t *pkg = (packet_t*) node->data;
		if(ble.server->getConnectedCount()) {
			uint32_t addr = CONV(pkg->src);
			notify(ble.addr, (unsigned char*) &addr, sizeof(addr));
			notify(
				pkg->dst == self.addr ? ble.data : ble.bcast,
				pkg->data, pkg->length < _LORA_PKG_SIZE_ ? pkg->length : _LORA_PKG_SIZE_
			);
			Serial.printf("# %u\r\n", pkg->length);
		}
		#ifdef _USE_DISP_
		display.clear();
		display.drawProgressBar(
			0, 54, 120, 8, map(node->size, -128, 0, 0, 100)
		);
		display.setTextAlignment(TEXT_ALIGN_LEFT);
		display.drawStringMaxWidth(0, 0, 128, (const char*) pkg->data);
		display.display();
		#endif
		free(node->data);
		free(node);
		yield();
	}
	if( !last || (millis() - last) > prefs.getULong("dly", 99999) ) {
		last = millis();
		char msg[64] = { 0 };
		unsigned size = sprintf(msg,
			" bcn from: %08lx \nat % 16ld\n", self.addr, last);
		packet_t *pkg = packet_make(
			self.addr,     // src
			UINT32_MAX,    // dest (broadcast)
			PKG_TYPE_NONE, // type
			0x0123         // TTL
		);
		// hexdump(Serial, (char*) pkg, sizeof(packet_t));
		Serial.printf("# %08x:", packet_prepare(pkg, size, msg));
		list_push(&self.queue.out, list_init(pkg, sizeof(packet_t)));
		yield();
	}
	delay(9);
	return yield();
}
