#include "AfdHandler.h"
#include "AfdUtil.h"

// Just a wrapper in case I want to expand upon the class based design later
PAFD_UTIL afdUtil;
extern "C" void HookAfd()
{
	// Create new AfdUtil to open Afd handle, then copy over to non paged pool
	AfdUtil afdUtilOrig = AfdUtil::AfdUtil();
	afdUtil = reinterpret_cast<PAFD_UTIL>(ExAllocatePool(NonPagedPoolNx, sizeof(AfdUtil)));

	if (afdUtil == NULL)
	{
		DBG_TRACE("HookAfd", "Couldn't allocate memory for AfdUtil");
		return;
	}

	memcpy(afdUtil, &afdUtilOrig, sizeof(AfdUtil));
	afdUtil->FindAndHookFileObjects();
}

extern "C" void FreeAfdHooks()
{
	AfdUtil::RestoreHookedFileObjects();
	ExFreePool(afdUtil);
}