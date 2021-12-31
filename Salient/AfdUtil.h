#pragma once

#include "dbgmsg.h"
#include "ntdef.h"
#include "common.h"

#include <ntddk.h>
#include <ntstrsafe.h>
#include <stdlib.h>
#include <wdf.h>

#define AFD_DEVICE L"\\Device\\Afd"

typedef NTSTATUS HOOK_DISPATCH(_In_ DRIVER_DISPATCH OriginalFunction, _In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp);
typedef HOOK_DISPATCH* PHOOK_DISPATCH;

typedef struct HookedDeviceObject
{
	PFILE_OBJECT FileObject;
	PDEVICE_OBJECT OldDeviceObject;
} HOOKED_DEVOBJ, *PHOOKED_DEVOBJ;

typedef class AfdUtil
{
	static ULONG HookCount;
	static PDEVICE_OBJECT OriginalDeviceObject;
	static PDEVICE_OBJECT FakeDeviceObject;
	static PDRIVER_DISPATCH OriginalDispatch[IRP_MJ_MAXIMUM_FUNCTION + 1];
	static PFILE_OBJECT HookedFileObjects[0x1000];

	static NTSTATUS AfdDispatchHook(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);
	static NTSTATUS HookAfdIoctl(_In_ DRIVER_DISPATCH OriginalFunction, _In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp);

	BOOLEAN AfdUtil::IsFileObject(_In_ PVOID Object);
	BOOLEAN CreateFakeDriverAndDeviceObject(_In_ PDEVICE_OBJECT BaseDeviceObject);
	BOOLEAN HookFileObject(_In_ PFILE_OBJECT FileObject);
	void AddHookedFileObject(_In_ PFILE_OBJECT FileObject);

public:
	static void RestoreHookedFileObjects();

	AfdUtil();
	~AfdUtil();
	void FindAndHookFileObjects();
} AFD_UTIL, *PAFD_UTIL;