#include "UsbGamepad.h"

#pragma comment(lib, "User32")
#pragma comment(lib, "Advapi32")
#pragma comment(lib, "Ole32")
#pragma comment(lib, "Winusb")
#pragma comment(lib, "SetupAPI")

#include <LFramework/USB/Host/UsbHostManager.h>
#include <LFramework/USB/Host/UsbHDevice.h>

static uint8_t cmdEnterConfigMode[] = { 0x01,0x43,0x00,0x01,0x00 };
static uint8_t cmdExitConfigMode[] = { 0x01, 0x43, 0x00, 0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A };

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
	_state.finalizeCurrentCommand();
	auto result = transfer({ UsbRequestType::PollBegin, 0x01 }).data;
	_state.currentCommand.append(0x01);
	return result;
}
uint8_t UsbGamepad::poll(uint8_t data) {
	auto result = transfer({ UsbRequestType::Poll, data }).data;
	_state.currentCommand.append(data);
	return result;
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

void UsbGamepad::setState(const State& state) {
	_state = state;

	executeCommand(cmdExitConfigMode, sizeof(cmdExitConfigMode));

	if (!_state.configHistory.empty()) {
		executeCommand(cmdEnterConfigMode, sizeof(cmdEnterConfigMode));
		for (std::uint32_t i = 0; i < _state.configHistory.size(); ++i) {
			auto& cmd = _state.configHistory[i];
			executeCommand(cmd.txBuffer, cmd.length);
		}
		
		if (!_state.isInConfigMode) {
			executeCommand(cmdExitConfigMode, sizeof(cmdExitConfigMode));
		}
	} else {
		if (_state.isInConfigMode) {
			executeCommand(cmdEnterConfigMode, sizeof(cmdEnterConfigMode));
		}
	}

	if (_state.currentCommand.length != 0) {
		executeCommand(_state.currentCommand.txBuffer, _state.currentCommand.length);
	}
}

void UsbGamepad::executeCommand(const uint8_t* data, size_t size) {
	transfer({ UsbRequestType::PollBegin, data[0] });
	for (std::size_t i = 1; i < size; ++i) {
		transfer({ UsbRequestType::Poll, data[i] });
	}
}