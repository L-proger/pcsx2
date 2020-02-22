#include "UsbGamepad.h"

#pragma comment(lib, "User32")
#pragma comment(lib, "Advapi32")
#pragma comment(lib, "Ole32")
#pragma comment(lib, "Winusb")
#pragma comment(lib, "SetupAPI")

#include <LFramework/USB/Host/UsbHostManager.h>
#include <LFramework/USB/Host/UsbHDevice.h>



UsbGamepad::UsbGamepad(const std::string& usbDevice) : _usbDevice(usbDevice){
	_usbInterface = _usbDevice.getInterface(0);
	_txEndpoint = _usbInterface->getEndpoint(false, 0);
	_rxEndpoint = _usbInterface->getEndpoint(true, 0);
	resetConnection();
}

std::vector<std::string> UsbGamepad::findUsbDevices() {
	std::vector<std::string> result;
	auto devices = UsbHostManager::enumerateDevices();

	for (auto& device : devices) {
		if (device.vid == 0x0301 && device.pid == 0x1111) {
			result.push_back(device.path);
		}
	}
	return result;
}

UsbGamepad::UsbResponse UsbGamepad::transfer(UsbRequest request) {
	auto txFuture = _txEndpoint->transferAsync(&request, sizeof(request));
	txFuture->wait();

	auto rxFuture = _rxEndpoint->transferAsync(_rxBuffer, sizeof(_rxBuffer));
	auto rxSize = rxFuture->wait();

	if (rxSize != sizeof(UsbResponse)) {
		return { 0xff };
	} else {
		return *((UsbResponse*)(&_rxBuffer[0]));
	}
}

uint8_t UsbGamepad::beginPoll() {
	return transfer({ UsbRequestType::PollBegin, 0x01 }).data;
}
uint8_t UsbGamepad::poll(uint8_t data) {
	return transfer({ UsbRequestType::Poll, data }).data;
}

void UsbGamepad::resetConnection() {
	auto txFuture = _txEndpoint->transferAsync(nullptr, 0);

	uint8_t rxBuffer[64];

	while (true) {
		auto rxFuture = _rxEndpoint->transferAsync(rxBuffer, sizeof(rxBuffer));
		auto rxSize = rxFuture->wait();
		if (rxSize == 0) {
			break;
		}
	}
	txFuture->wait();
}