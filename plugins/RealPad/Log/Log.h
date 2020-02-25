#pragma once

#define PR_LOG_ENABLED

#ifdef PR_LOG_ENABLED
#define RP_LOG(prefix, format, ...) logWrite(prefix, format, ##__VA_ARGS__)
#define RP_LOGI(format, ...) logWrite("[Info ] ", format, ##__VA_ARGS__)
#define RP_LOGE(format, ...) logWrite("[Error] ", format, ##__VA_ARGS__)
#else
#define RP_LOG(prefix, format, ...)
#define RP_LOGI(format, ...)
#define RP_LOGE(format, ...) 
#endif

void logWrite(const char* tag, const char* format, ...);
void logSetDir(const char* dir);


