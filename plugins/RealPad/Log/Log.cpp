#include "Log.h"
#include <cstdarg>
#include <stdio.h>

#include <mutex>
#include <chrono>

#include <cinttypes>
std::mutex logMutex;

class LogState {
public:
	FILE* file = nullptr;

	LogState() : _timeBuffer{} {
		_startTime = std::chrono::high_resolution_clock::now();
	}
	~LogState() {
		closeLog();
	}

	bool openLog() {
		if (file != nullptr) {
			return true;
		}
		file = _fsopen((_logDir + "RealPadLog.txt").c_str(), "w", _SH_DENYWR);
		if (file != nullptr) {
			return true;
		}
		return false;
	}
	std::chrono::microseconds getTimeElapsedUseconds() {
		return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - _startTime);
	}

	void setLogDir(const std::string& dir) {
		_logDir = dir;
		closeLog();
		openLog();
	}
private:
	void closeLog() {
		if (file != nullptr) {
			fclose(file);
			file = nullptr;
		}
	}
	
	std::chrono::high_resolution_clock::time_point _startTime;
	char _timeBuffer[80];
	std::string _logDir = "logs/";
};

static LogState logState;

void logWrite(const char* tag, const char* format, ...) {
	std::lock_guard<std::mutex> lock(logMutex);
	if (!logState.openLog()) {
		return;
	}

	va_list arg;
	va_start(arg, format);
	fprintf(logState.file, "[%" PRIu64 "] ", logState.getTimeElapsedUseconds().count());
	fprintf(logState.file, tag);
	vfprintf(logState.file, format, arg);
	fprintf(logState.file, "\n");
	fflush(logState.file);
	va_end(arg);
}

void logSetDir(const char* dir) {
	std::lock_guard<std::mutex> lock(logMutex);
	logState.setLogDir(dir);
}