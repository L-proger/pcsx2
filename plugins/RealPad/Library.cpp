#include "PS2Edefs.h"
#include "Log/Log.h"
#include <cinttypes>
#define PLUGIN_EXPORT(type) extern "C" __declspec(dllexport) type CALLBACK
#define REALPAD_VERSION ((0 << 8) | (1 << 0))

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
    return 0;
}
PLUGIN_EXPORT(u8) PADstartPoll(int pad){
	RP_LOGI("%s %i", __FUNCTION__, pad);
    //Return first response byte (0xFF)
    return 0xff;
}
PLUGIN_EXPORT(u8) PADpoll(u8 value){
	RP_LOGI("%s (%" PRIu8 ")", __FUNCTION__, value);
	//retrurn response byte. TODO: implement
    return 0xff;
}
PLUGIN_EXPORT(u32) PADquery(){
	RP_LOGI("%s", __FUNCTION__);
    return 1;
}
PLUGIN_EXPORT(keyEvent *) PADkeyEvent(){
	RP_LOGI("%s", __FUNCTION__);
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