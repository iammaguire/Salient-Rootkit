#include "dbgmsg.h"
#include "salient.h"
#include "IRPHandler.h"
#include "AfdHandler.h"

const WCHAR DeviceNameBuffer[] = L"\\Device\\salient";
const WCHAR DeviceLinkBuffer[] = L"\\DosDevices\\salient";
PDEVICE_OBJECT SalientDeviceObject;

NTSTATUS RegisterDriverDeviceName(_In_ PDRIVER_OBJECT DriverObject)
{
	NTSTATUS ntStatus;
	UNICODE_STRING unicodeString;

	RtlInitUnicodeString(&unicodeString, DeviceNameBuffer);
	ntStatus = IoCreateDevice(DriverObject, 0, &unicodeString, FILE_DEVICE_RK, 0, TRUE, &SalientDeviceObject);
	
	return ntStatus;
}

NTSTATUS RegisterDriverDeviceLink()
{
	NTSTATUS ntStatus;
	UNICODE_STRING unicodeString;
	UNICODE_STRING unicodeLinkString;

	RtlInitUnicodeString(&unicodeString, DeviceNameBuffer);
	RtlInitUnicodeString(&unicodeLinkString, DeviceLinkBuffer);
	ntStatus = IoCreateSymbolicLink(&unicodeLinkString, &unicodeString);
	
	return ntStatus;
}

VOID UnloadSalient(_In_ PDRIVER_OBJECT DriverObject) 
{
	UNREFERENCED_PARAMETER(DriverObject);
	DBG_TRACE("Driver Entry", "Unloading salient driver.");

	FreeAfdHooks();

	UNICODE_STRING unicodeString;

	if (DriverObject->DeviceObject != NULL)
	{
		DBG_TRACE("Unload Salient", "Unregistering symbolic link.");
		RtlInitUnicodeString(&unicodeString, DeviceLinkBuffer);
		IoDeleteSymbolicLink(&unicodeString);
		
		DBG_TRACE("Unload Salient", "Unregistering device name.");
		IoDeleteDevice(DriverObject->DeviceObject);
	}

	return;
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING regPath)
{	
	UNREFERENCED_PARAMETER(regPath);
	
	NTSTATUS ntStatus;

	DBG_TRACE("Driver Entry", "Registering driver device name");
	ntStatus = RegisterDriverDeviceName(DriverObject);
	if (!NT_SUCCESS(ntStatus))
	{
		DBG_TRACE("Driver Entry", "Failed to create driver device.");
		return ntStatus;
	}

	DBG_TRACE("Driver Entry", "Registering driver symbolic link.");
	ntStatus = RegisterDriverDeviceLink();
	if (!NT_SUCCESS(ntStatus))
	{
		DBG_TRACE("Driver Entry", "Failed to register driver symbolic link.");
		return ntStatus;
	}
	
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		DriverObject->MajorFunction[i] = defaultDispatch;
	}

	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = dispatchIOControl;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = dispatchOpen;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = dispatchClose;

	DBG_TRACE("Driver Entry", "Salient driver has been loaded.");
	DriverObject->DriverUnload = UnloadSalient;

	HookAfd();

	return STATUS_SUCCESS;
}

// pg. 244
// pg. 475 call table modification (IAT, SSDT, IRP, GDT)