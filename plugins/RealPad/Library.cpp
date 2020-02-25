#include "PS2Edefs.h"
#include "Log/Log.h"
#include "UsbGamepad.h"
#include <memory>
#include <cinttypes>

#define PLUGIN_EXPORT(type) extern "C" __declspec(dllexport) type CALLBACK
#define REALPAD_VERSION ((0 << 8) | (1 << 0))

std::vector<std::shared_ptr<UsbGamepad>> gamepads;
int activePollingPad = 0;


PLUGIN_EXPORT(u32) PS2EgetLibType(){
	RP_LOGI("%s", __FUNCTION__);
    return PS2E_LT_PAD;
}
PLUGIN_EXPORT(u32) PS2EgetLibVersion2(u32 type){
	RP_LOGI("%s (%" PRIu32 ")", __FUNCTION__, type);
    return type == PS2E_LT_PAD ? (PS2E_PAD_VERSION << 16) | REALPAD_VERSION : 0;
}
PLUGIN_EXPORT(const char *)PS2EgetLibName(){
	RP_LOGI("%s", __FUNCTION__);
    return "L-proger RealPad plugin";
}

PLUGIN_EXPORT(void)PADsetSettingsDir(const char* dir) {
	
}

PLUGIN_EXPORT(void)PADsetLogDir(const char* dir) {
	logSetDir(dir);
}

PLUGIN_EXPORT(s32) PADinit(u32 flags) {
	RP_LOGI("%s (%" PRIi32 ")", __FUNCTION__, flags);
	return 0;
}

PLUGIN_EXPORT(void) PADshutdown() {
	RP_LOGI("%s", __FUNCTION__);
}

PLUGIN_EXPORT(void) PADclose() {
	RP_LOGI("%s", __FUNCTION__);
}

PLUGIN_EXPORT(s32) PADopen(void *pDsp){
	RP_LOGI("%s %i", __FUNCTION__, (int)pDsp);
	auto gamepadList = UsbGamepad::findUsbDevices();
	for (const auto& gamepadPath : gamepadList) {
		try {
			gamepads.push_back(std::make_shared<UsbGamepad>(gamepadPath));
			RP_LOGE("Open USB gamepad: %s", gamepadPath.c_str());
		} catch (std::exception & ex) {
			RP_LOGE("Failed to open USB gamepad with error: %s", ex.what());
		}
	}

    return 0;
}

static bool justFreezed = false;
PLUGIN_EXPORT(u8) PADstartPoll(int pad){
	
	//
	u8 result = 0xff;
	--pad;  //Make ID zero based
	activePollingPad = pad;
	if (gamepads.size() > pad) {
		if (justFreezed) {
			auto state = gamepads[pad]->getState();
			gamepads[pad]->setState(state);
			RP_LOGI("%s: state reload after freeze ", __FUNCTION__);
			justFreezed = false;
		}
		result = gamepads[pad]->beginPoll();
	}
	RP_LOGI("%s (%i) -> %" PRIu8, __FUNCTION__, pad, result);
    return result;
}
PLUGIN_EXPORT(u8) PADpoll(u8 value){
	//
	u8 result = 0xff;
	if (gamepads.size() > activePollingPad) {
		result = gamepads[activePollingPad]->poll(value);
	}
	RP_LOGI("%s (%" PRIu8 ") -> %" PRIu8 ";", __FUNCTION__, value, result);
    return result;
}
PLUGIN_EXPORT(u32) PADquery(){
	RP_LOGI("%s", __FUNCTION__);
    return 1;
}
PLUGIN_EXPORT(keyEvent *) PADkeyEvent(){
	//RP_LOGI("%s", __FUNCTION__);
    return nullptr;
}
PLUGIN_EXPORT(s32) PADsetSlot(u8 port, u8 slot){
	RP_LOGI("%s (%" PRIu8 ", %" PRIu8 ")", __FUNCTION__, port, slot);
    return 1;
}
PLUGIN_EXPORT(s32) PADqueryMtap(u8 port){
	RP_LOGI("%s (%" PRIu8 ")", __FUNCTION__, port);
    return 0;
}

s32 CALLBACK PADfreeze(int mode, freezeData* data) {
	
	if (mode == FREEZE_SIZE) {
		if (!gamepads.empty()) {
			SaveStream stream;
			stream << gamepads[0]->getState();
			data->size = stream.getBuffer().size();
			RP_LOGI("%s (Query freeze data): %i", __FUNCTION__, (int)(data->size));
		} else {
			RP_LOGI("%s (Query freeze data): %i", __FUNCTION__, 0);
			data->size = 0;
		}
	} else if (mode == FREEZE_SAVE) {

		if (!gamepads.empty()) {
			SaveStream stream;
			stream << gamepads[0]->getState();
			data->size = stream.getBuffer().size();
			RP_LOGI("%s (Freeze save): %i", __FUNCTION__, (int)(data->size));
			if (data->size == stream.getBuffer().size()) {
				memcpy(data->data, stream.getBuffer().data(), data->size);
				RP_LOGI("%s (Freeze save): OK!", __FUNCTION__);
				justFreezed = true;
			} else {
				RP_LOGI("%s (Freeze save): FAIL! Invalid buffer size!", __FUNCTION__);
				return -1;
			}
		}
	} else if (mode == FREEZE_LOAD) {
		if (!gamepads.empty()) {
			LoadStream stream(data->data, data->size);

			State state;
			stream << state;

			gamepads[0]->setState(state);
			
			RP_LOGI("%s (Freeze load): %i", __FUNCTION__, (int)(data->size));
		}
	}

	return 0;
}