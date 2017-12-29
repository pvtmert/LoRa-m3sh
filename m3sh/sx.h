
#ifndef _SX_H_
#define _SX_H_

#include <ArduinoOTA.h>
#include <SPI.h>
#include <lora32.h>

extern "C" {
	#include "lora.h"
}

static struct {
	addr_t addr;
	struct {
		list_t *in;
		list_t *out;
		list_t *msg;
	} queue;
//	unsigned h_size;
//	packet_t hist[99];
} self;

typedef struct Message {
	size_t size;
	bool rdy;
	int rssi;
	union {
		void *none;
		char *data;
		packet_t *pkg;
	};
} msg_t;

/*
	if (message.rdy) {
		message.rdy = false;
		Serial.println("lora got:"
			+ String(message.size) + " "
			+ String(message.rssi) + " "
		);
		String buf = "pkg:" + String(message.size) + " "
			+ String(message.rssi) + "\n";
		hexdump(Serial, message.data, message.size);
		if(message.size != sizeof(packet_t)) {
			Serial.printf("size mismatch: pkg:%d msg:%d \r\n",
				sizeof(packet_t), message.size);
			return;
		}
		packet_t pkg = { 0 };
		memcpy(&pkg, message.data, sizeof(pkg));
		if(packet_verify(&pkg)) {
			Serial.printf("chksum error: %04x.%04x \r\n",
				pkg->checksum, pkg->data_crc);
			return;
		}
		buf += " " + String((char*) pkg.data) + "\n";
		if(service->getServer()->getConnectedCount()) {
			void *data = (void*) pkg.data;
			notify(characteristic, pkg.data, strlen((char*) pkg.data));
		}
		#ifdef _USE_DISP_
		display.clear();
		display.drawProgressBar(
			0, 54, 120, 8, map(message.rssi, -128, 0, 0, 100)
		);
		display.drawStringMaxWidth(0, 0, 128, buf);
		display.display();
		#endif
	} else if( (millis() - last) > 9999 || !digitalRead(0) ) {
		char msg[80]; last = millis();
		sprintf(msg, "bcn from: %llx\nat %ld\n", self.addr, last);
		send(msg, sizeof(msg));
	}
*/


void sx_split_cb(char *data, size_t size, void (*callback)(char*,size_t)) {
	int length = size, offset = 0;
	while(length > 0) {
		int size = length < _LORA_PKG_SIZE_ ? length : _LORA_PKG_SIZE_;
		char *buffer = (char*) calloc(size, sizeof(char));
		memcpy(buffer, data + offset, sizeof(char) * size);
		if(callback) {
			callback(buffer, size);
		}
		free(buffer);
		offset += _LORA_PKG_SIZE_;
		length -= _LORA_PKG_SIZE_;
		continue;
	}
	return;
}

void sx_send(void *data, size_t size) {
	LoRa.beginPacket();
	LoRa.write((unsigned char*) data, size);
	LoRa.endPacket();
	LoRa.receive();
	return yield();
}

void sx_sendto(void *data, size_t size, addr_t target) {
	packet_t *pkg = packet_make(
		self.addr,                    // src
		target ? target : UINT32_MAX, // dest
		PKG_TYPE_NONE,                // type
		0x00FF                        // TTL
	);
	int length = size, offset = 0;
	while(length > 0) {
		int size = length < _LORA_PKG_SIZE_ ? length : _LORA_PKG_SIZE_;
		packet_prepare(pkg, size, data + offset);
		sx_send(pkg, sizeof(packet_t));
		offset += _LORA_PKG_SIZE_;
		length -= _LORA_PKG_SIZE_;
		continue;
	}
	free(pkg);
	return yield();
}

void sx_receive(int size) {
	if (!size) {
		return;
	}
	msg_t *message = (msg_t*) calloc(1, sizeof(msg_t));
	message->rssi = LoRa.packetRssi();
	message->size = (size  ? size : LoRa.available());
	message->data = (char*) calloc(message->size, sizeof(char));
	LoRa.readBytes(message->data, message->size);
	Serial.printf("packet: rssi:%d, size:%d\r\n",
		message->rssi, message->size);
	hexdump(Serial, message->data, message->size);
	if(message->size == sizeof(packet_t)) {
		message->rdy = packet_verify(message->pkg) || !message->pkg->checksum;
		list_push(&self.queue.in, list_init(message, sizeof(msg_t)));
	} else {
		free(message);
	}
	return;
}

void sx_init(uint64_t id) {
	pinMode(26, INPUT);
	SPI.begin(5, 19, 27, 18);
	LoRa.crc();
	LoRa.setPins(18, 14, 26);
	LoRa.setSPIFrequency(8E6);
	while (!LoRa.begin(433E6)) {
		Serial.printf("%d lora error\r\n", millis());
		LoRa.dumpRegisters(Serial);
		delay(9999);
		while(true) {
			ArduinoOTA.handle();
			yield();
			continue;
		}
		continue;
	}
	self.queue.msg = NULL;
	self.queue.out = NULL;
	self.queue.in = NULL;
	self.addr = (uint32_t) (((id << 16) >> 32) & 0xFFFFFFFF);
	LoRa.dumpRegisters(Serial);
	LoRa.onReceive(sx_receive);
	LoRa.receive();
	return;
}

#endif
