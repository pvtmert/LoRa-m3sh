
#ifndef _BLE_H_
#define _BLE_H_

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

static struct {
	BLEServer *server;
	BLEService *service;
	BLECharacteristic *rssi;
	BLECharacteristic *addr;
	BLECharacteristic *data;
	BLECharacteristic *bcast;
	struct {
		BLEService *service;
		BLECharacteristic *button;
		BLECharacteristic *beacon;
		BLECharacteristic (*led)[3];
	} prefs;
	struct {
		BLEService *service;
		BLECharacteristic *manufacturer;
		BLECharacteristic *model;
		BLECharacteristic *batt;
		BLECharacteristic *pass;
	} device;
} ble;

class BLECallbacks:
	public BLEServerCallbacks, public BLECharacteristicCallbacks {
	void onConnect(BLEServer *server) {
		Serial.println("ble connected: ");
		return;
	};
	void onDisconnect(BLEServer *server) {
		Serial.println("ble disconnect: ");
		return;
	}
	void onRead(BLECharacteristic *chr) {
		Serial.printf("char read %p %s\r\n", chr);
		return;
	}
	void onWrite(BLECharacteristic *chr) {
		std::string str = chr->getValue();
		char *data = (char*) str.c_str();
		unsigned length = str.length();
		Serial.printf("ble data: %u\r\n", length);
		if(!length) {
			return;
		}
		hexdump(Serial, data, length);
		Serial.println(data);
		if(chr == ble.addr) {
			uint32_t addr = *(uint32_t*) data;
			if(!addr) {
				addr = CONV(self.addr);
				chr->setValue((unsigned char*) &addr, sizeof(addr));
				chr->notify();
			} else {
				prefs.putULong("addr", CONV(addr));
			}
		};
		if(chr == ble.data) {
			pkg(data, length, prefs.getULong("addr", 0));
		};
		if(chr == ble.bcast) {
			pkg(data, length, UINT32_MAX);
		};
		if(chr == ble.prefs.beacon) {
			prefs.putULong("dly", atoi(data));
		};
		if(chr == ble.prefs.button);
		if(chr == ble.device.pass) {
			prefs.putString("pass", data);
		};
		return;
	}
	void pkg(char *data, size_t size, addr_t target) {
		int length = size, offset = 0;
		while(length > 0) {
			packet_t *pkg = packet_make(
				self.addr, target, PKG_TYPE_NONE, 0x00FF
			);
			int size = length < _LORA_PKG_SIZE_ ? length : _LORA_PKG_SIZE_;
			packet_prepare(pkg, size, data + offset);
			list_push(&self.queue.out, list_init(pkg, sizeof(packet_t)));
			offset += _LORA_PKG_SIZE_;
			length -= _LORA_PKG_SIZE_;
			continue;
		}
		return;
	}
};

BLECharacteristic* ble_make_char(BLEService *service, uint16_t uuid,
	BLECharacteristicCallbacks *callbacks,
	bool read, bool write, bool notify, bool indicate) {
	BLECharacteristic *characteristic = service->createCharacteristic(
		BLEUUID(uuid), 0
		| (read     ? BLECharacteristic::PROPERTY_READ     : 0)
		| (write    ? BLECharacteristic::PROPERTY_WRITE    : 0)
		| (notify   ? BLECharacteristic::PROPERTY_NOTIFY   : 0)
		| (indicate ? BLECharacteristic::PROPERTY_INDICATE : 0)
	);
	characteristic->addDescriptor(new BLE2902());
	characteristic->setCallbacks(callbacks);
	return characteristic;
}


void notify(BLECharacteristic *chr, unsigned char *data, size_t size) {
	chr->setValue(std::string((char*) data, size));
	//chr->indicate();
	chr->notify();
	return yield();
}

void ble_init(char *name) {
	BLEDevice::init(name);
	BLECallbacks *callbacks = new BLECallbacks();
	ble.server = BLEDevice::createServer();
	ble.server->setCallbacks(callbacks);
	_main: {
		ble.service = ble.server->createService(BLEUUID((uint16_t) 0x1234));
		ble.rssi  = ble_make_char(ble.service, 0x0000,
			callbacks, true, false, true, false);
		ble.addr  = ble_make_char(ble.service, 0x0001,
			callbacks, true, true, true, false);
		ble.data  = ble_make_char(ble.service, 0x0002,
			callbacks, true, true, true, false);
		ble.bcast = ble_make_char(ble.service, 0x0003,
			callbacks, true, true, true, false);
		ble.service->start();
	}
	_prefs: {
		ble.prefs.service = ble.server->createService(BLEUUID((uint16_t) 0xFFF1));
		ble.prefs.button = ble_make_char(ble.prefs.service, 0x9991,
			callbacks, true, true, true, false);
		ble.prefs.beacon = ble_make_char(ble.prefs.service, 0x9992,
			callbacks, true, true, true, false);
		ble.prefs.service->start();
	}
	_device: {
		ble.device.service = ble.server->createService(BLEUUID((uint16_t) 0x1800));
		ble.device.manufacturer = ble_make_char(ble.device.service, 0x2A21,
			callbacks, true, false, false, false);
		ble.device.model        = ble_make_char(ble.device.service, 0x2A22,
			callbacks, true, false, false, false);
		ble.device.batt         = ble_make_char(ble.device.service, 0x2A23,
			callbacks, true, false, true, false);
		ble.device.pass         = ble_make_char(ble.device.service, 0x0000,
			callbacks, false, true, false, false
		);
	}
	ble.server->getAdvertising()->start();
	return;
}

#endif
