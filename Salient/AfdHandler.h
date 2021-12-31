#pragma once

#ifdef __cplusplus
extern "C" void HookAfd();
extern "C" void FreeAfdHooks();
#else
void HookAfd();
void FreeAfdHooks();
#endif