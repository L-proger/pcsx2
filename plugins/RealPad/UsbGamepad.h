#pragma once

#include <string>
#include <vector>
#include <LFramework/USB/Host/UsbHDevice.h>

class UsbGamepad {
public:
	enum class UsbRequestType : uint8_t {
		PollBegin,
		Poll
	};

	struct UsbRequest {
		UsbRequestType type;
		uint8_t data;
	};

	struct UsbResponse {
		uint8_t data;
	};

	UsbGamepad(const std::string& usbDevice);
	static std::vector<std::string> findUsbDevices();

	uint8_t beginPoll();
	uint8_t poll(uint8_t data);
private:
	void resetConnection();
	UsbResponse transfer(UsbRequest request);

	LFramework::USB::UsbHDevice _usbDevice;
	std::shared_ptr<LFramework::USB::UsbHInterface> _usbInterface;
	LFramework::USB::UsbHEndpoint* _rxEndpoint;
	LFramework::USB::UsbHEndpoint* _txEndpoint;
	uint8_t _rxBuffer[64];
};