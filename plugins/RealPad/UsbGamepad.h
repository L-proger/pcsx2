#pragma once

#include <string>
#include <vector>
#include <LFramework/USB/Host/UsbHDevice.h>
#include <cassert>
#include <array>

class Stream {
public:
	virtual bool isLoadStream() const = 0;
	virtual void handleData(std::size_t size, void* data) = 0;
	const std::vector<std::uint8_t>& getBuffer() const {
		return _buffer;
	}
protected:
	std::vector<std::uint8_t> _buffer;
};

class SaveStream : public Stream {
public:
	bool isLoadStream() const override {
		return false;
	}
	void handleData(std::size_t size, void* data) override {
		_buffer.insert(_buffer.end(), reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + size);
	}
};

class LoadStream : public Stream {
public:
	LoadStream(const void* data, std::size_t size) {
		_buffer.insert(_buffer.begin(), reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + size);
	}
	bool isLoadStream() const override {
		return true;
	}
	void handleData(std::size_t size, void* data) override {
		memcpy(data, &_buffer[_offset], size);
		_offset += size;
	}
private:
	std::size_t _offset = 0;
};

template<typename T>
Stream& operator << (Stream& stream, T& data) {
	auto dataPtr = reinterpret_cast<uint8_t*>(&data);
	stream.handleData(sizeof(T), &data);
	return stream;
}


template<typename T, std::uint32_t Capacity>
class CircularPODBuffer {
public:
	using SizeType = std::uint32_t;

	friend Stream& operator << (Stream& stream, CircularPODBuffer& data) {
		return stream << data._size << data._readPos << data._buffer;
	}

	void add(const T& data) {
		if (_size == Capacity) {
			_buffer[_readPos] = data;
			_readPos = (_readPos + 1) % Capacity;
		} else {
			_buffer[(_readPos + _size) % Capacity] = data;
			++_size;
		}
	}
	T& operator[](SizeType id) {
		return _buffer[(_readPos + id) % Capacity];
	}
	SizeType size() const {
		return _size;
	}
	bool empty() const {
		return size() == 0;
	}
	void clear() {
		_size = 0;
		_readPos = 0;
	}
private:
	SizeType _size = 0;
	SizeType _readPos = 0;
	std::array<T, Capacity> _buffer;
};



struct State {
	static constexpr std::size_t MaxCommandLength = 21;

	class Command {
	public:
		std::uint8_t length = 0;
		std::uint8_t txBuffer[MaxCommandLength] = {};

		void append(uint8_t txValue) {
			assert(length < MaxCommandLength);
			txBuffer[length] = txValue;
			length++;
		}

		bool isConfigEnterCommand() const {
			return (length >= 4) && (txBuffer[1] == 0x43) && (txBuffer[3] == 0x01);
		}

		bool isConfigExitCommand() const {
			return (length >= 4) && (txBuffer[1] == 0x43) && (txBuffer[3] == 0x00);
		}
		bool isMainPollCommand() {
			return (length >= 2) && (txBuffer[1] == 0x42);
		}

		bool isUnknownConfigReadCommand() {
			return (length >= 2) && ((txBuffer[1] == 0x46) || (txBuffer[1] == 0x47) || (txBuffer[1] == 0x4C));
		}

		bool isReadButtonsMaskCommand() {
			return (length >= 2) && (txBuffer[1] == 0x41);
		}

		bool isReadExtendedPadDescriptionCommand() {
			return (length >= 2) && (txBuffer[1] == 0x45);
		}

		void clear() {
			length = 0;
		}
	};

	CircularPODBuffer<Command, 64> configHistory;

	Command currentCommand;

	bool isInConfigMode = false;

	void finalizeCurrentCommand() {
		if (currentCommand.length == 0) {
			return;
		}
		if (currentCommand.isConfigExitCommand()) {
			isInConfigMode = false;
		} else if (currentCommand.isConfigEnterCommand()) {
			isInConfigMode = true;
		} else {
			if (isInConfigMode) {
				if (!currentCommand.isMainPollCommand() && !currentCommand.isUnknownConfigReadCommand() && !currentCommand.isReadButtonsMaskCommand() && !currentCommand.isReadExtendedPadDescriptionCommand()) {
					configHistory.add(currentCommand);
				}
			} else {
				//Not interested in non-config commands
			}
		}
		currentCommand.clear();
	}
};


template<typename T>
Stream& operator << (Stream& stream, std::vector<T>& data) {
	std::uint64_t size = data.size();
	stream << size;
	if (stream.isLoadStream()) {
		data.resize(size);
	}
	for (size_t i = 0; i < data.size(); ++i) {
		stream << data[i];
	}
	return stream;
}


inline Stream& operator << (Stream& stream, State& data) {
	return stream << data.configHistory << data.currentCommand << data.isInConfigMode;
}




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

	void setState(const State& state);
	State& getState() {
		return _state;
	}
private:
	int _pollByte = 0;
	State _state;

	void resetConnection();
	UsbResponse transfer(UsbRequest request);
	void executeCommand(const uint8_t* data, size_t size);

	LFramework::USB::UsbHDevice _usbDevice;
	std::shared_ptr<LFramework::USB::UsbHInterface> _usbInterface;
	LFramework::USB::UsbHEndpoint* _rxEndpoint;
	LFramework::USB::UsbHEndpoint* _txEndpoint;
	uint8_t _rxBuffer[64];
};