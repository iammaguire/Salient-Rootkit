#include "IrpHandler.h"
#include "AfdUtil.h"
#include "AfdHandler.h"


ULONG AfdUtil::HookCount;
PDEVICE_OBJECT AfdUtil::OriginalDeviceObject;
PDEVICE_OBJECT AfdUtil::FakeDeviceObject;
PDRIVER_DISPATCH AfdUtil::OriginalDispatch[IRP_MJ_MAXIMUM_FUNCTION + 1];
PFILE_OBJECT AfdUtil::HookedFileObjects[0x1000];

AfdUtil::~AfdUtil() {}

AfdUtil::AfdUtil()
{
	NTSTATUS status;
	HANDLE afdDeviceHandle;
	IO_STATUS_BLOCK statusBlock;
	UNICODE_STRING afdDeviceName;
	OBJECT_ATTRIBUTES afdAttributes;
	PFILE_OBJECT afdDeviceHandleObject = NULL;

	RtlInitUnicodeString(&afdDeviceName, AFD_DEVICE);
	InitializeObjectAttributes(&afdAttributes, &afdDeviceName, OBJ_KERNEL_HANDLE, NULL, NULL);

	status = ZwCreateFile(&afdDeviceHandle, STANDARD_RIGHTS_ALL, &afdAttributes, &statusBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_OPEN, FILE_NON_DIRECTORY_FILE, NULL, 0);
	if (NT_SUCCESS(status) == FALSE)
	{
		DBG_TRACE("AfdUtil", "Failed to open handle to Afd device");
		goto exit;
	}

	status = ObReferenceObjectByHandle(afdDeviceHandle, 0, *IoFileObjectType, KernelMode, reinterpret_cast<PVOID*>(&afdDeviceHandleObject), NULL);
	if (NT_SUCCESS(status) == FALSE)
	{
		DBG_TRACE("AfdUtil", "Failed to open Afd device by handle");
		goto exit;
	}

	NT_ASSERTMSG("[AfdUtil] WARNING! AFD DEVICE OBJECT IS NULL!", afdDeviceHandle->DeviceObject);
	DBG_TRACE("AfdUtil", "Found Afd device");

	AfdUtil::OriginalDeviceObject = afdDeviceHandleObject->DeviceObject;
	DBG_PRINT2("[AfdUtil] Original Afd device object: 0x%p\n", AfdUtil::OriginalDeviceObject);

	exit:
	//if (afdDeviceHandleObject)
	//	ObDereferenceObject(afdDeviceHandleObject);
	if (afdDeviceHandle)
		ZwClose(afdDeviceHandle);
}

NTSTATUS AfdUtil::HookAfdIoctl(_In_ DRIVER_DISPATCH OriginalFunction, _In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp)
{
	DBG_TRACE("HookAfdIoctl", "Hello from HookAfdIoctl!");
	return OriginalFunction(DeviceObject, Irp);
}

NTSTATUS AfdUtil::AfdDispatchHook(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
	PIO_STACK_LOCATION irpStackLocation;
	PKMUTEX fileObjectLock;
	
	UNREFERENCED_PARAMETER(DeviceObject);
	DBG_TRACE("AfdDispatchHook", "Got a hooked Afd IRP!");

	irpStackLocation = IoGetCurrentIrpStackLocation(Irp);
	if (irpStackLocation->MajorFunction == IRP_MJ_CLEANUP)
	{
		irpStackLocation->FileObject->DeviceObject = AfdUtil::OriginalDeviceObject;
		fileObjectLock = reinterpret_cast<PKMUTEX>(irpStackLocation->FileObject->FsContext2);
		ExFreePool(fileObjectLock);
		DBG_TRACE("AfdDispatchHook", "Unhooked a process");
	}

	NT_ASSERT(AfdUtil::OriginalDispatch[irpStackLocation->MajorFunction] != AfdUtil::AfdDispatchHook);

	return AfdUtil::HookAfdIoctl(AfdUtil::OriginalDispatch[irpStackLocation->MajorFunction], AfdUtil::OriginalDeviceObject, Irp);
}

BOOLEAN AfdUtil::CreateFakeDriverAndDeviceObject(_In_ PDEVICE_OBJECT BaseDeviceObject)
{
	NTSTATUS status;
	OBJECT_ATTRIBUTES fakeDriverAttributes;
	OBJECT_ATTRIBUTES fakeDeviceAttributes;
	PDRIVER_OBJECT fakeDriverObject;
	POBJECT_HEADER_NAME_INFO realDeviceNameHeader;
	CSHORT fakeDriverObjectSize;
	CSHORT fakeDeviceObjectSize;

	InitializeObjectAttributes(&fakeDriverAttributes, &BaseDeviceObject->DriverObject->DriverName, OBJ_PERMANENT | OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	fakeDriverObjectSize = sizeof(DRIVER_OBJECT) + sizeof(EXTENDED_DRIVER_EXTENSION);

	NT_ASSERT(*IoDriverObjectType);
	NT_ASSERT(*IoDeviceObjectType);

	status = ObCreateObject(KernelMode, *IoDriverObjectType, &fakeDriverAttributes, KernelMode, NULL, fakeDriverObjectSize, 0, 0, reinterpret_cast<PVOID*>(&fakeDriverObject));
	if (NT_SUCCESS(status) == FALSE)
	{
		DBG_TRACE("CreateFakeDriverAndDeviceObject", "Failed to create fake driver object");
		return FALSE;
	}

	memcpy(fakeDriverObject, BaseDeviceObject->DriverObject, fakeDriverObjectSize);
	realDeviceNameHeader = static_cast<POBJECT_HEADER_NAME_INFO>(ObQueryNameInfo(BaseDeviceObject));

	NT_ASSERT(realDeviceNameHeader);
	if (realDeviceNameHeader == NULL)
	{
		DBG_TRACE("CreateFakeDriverAndDeviceObject", "Failed to query object name info");
		return FALSE;
	}

	InitializeObjectAttributes(&fakeDeviceAttributes, &realDeviceNameHeader->Name, OBJ_KERNEL_HANDLE | OBJ_PERMANENT, NULL, BaseDeviceObject->SecurityDescriptor);
	fakeDeviceObjectSize = sizeof(DEVICE_OBJECT) + sizeof(EXTENDED_DEVOBJ_EXTENSION);

	if (FlagOn(BaseDeviceObject->Flags, DO_EXCLUSIVE))
		fakeDeviceAttributes.Attributes |= DO_EXCLUSIVE;

	status = ObCreateObject(KernelMode, *IoDeviceObjectType, &fakeDeviceAttributes, KernelMode, NULL, fakeDeviceObjectSize, 0, 0, reinterpret_cast<PVOID*>(&AfdUtil::FakeDeviceObject));
	if (NT_SUCCESS(status) == FALSE)
	{
		DBG_TRACE("CreateFakeDriverAndDeviceObject", "Failed to create fake device object");
		return FALSE;
	}

	memcpy(AfdUtil::FakeDeviceObject, BaseDeviceObject, fakeDeviceObjectSize);
	AfdUtil::FakeDeviceObject->DriverObject = fakeDriverObject;
	fakeDriverObject->DeviceObject = AfdUtil::FakeDeviceObject;

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		AfdUtil::OriginalDispatch[i] = fakeDriverObject->MajorFunction[i];
		fakeDriverObject->MajorFunction[i] = AfdDispatchHook;
	}

	// TODO: Implement FastIODispatch hooking

	return TRUE;
}

BOOLEAN AfdUtil::HookFileObject(_In_ PFILE_OBJECT FileObject)
{
	PKMUTEX fileObjectLock;
	PDEVICE_OBJECT oldDeviceObject;

	// Already hooked this object?
	if (FileObject->DeviceObject->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] == AfdDispatchHook)
	{
		return TRUE;
	}

	if (AfdUtil::FakeDeviceObject == NULL)
	{
		if (CreateFakeDriverAndDeviceObject(FileObject->DeviceObject) == FALSE)
		{
			DBG_TRACE("HookFileObject", "Failed to create fake device object");
			return FALSE;
		}
		DBG_TRACE("HookFileObject", "Created fake device object");
	}

	fileObjectLock = reinterpret_cast<PKMUTEX>(ExAllocatePool(NonPagedPool, sizeof(KMUTEX)));
	if (fileObjectLock == NULL)
	{
		DBG_TRACE("HookFileObject", "Failed to allocate space for file object mutex lock");
		return FALSE;
	}

	memset(fileObjectLock, 0, sizeof(KMUTEX));
	KeInitializeMutex(fileObjectLock, 0);

	// Sanity check?
	if (FileObject->FsContext2) NT_ASSERT(FALSE);

	FileObject->FsContext2 = fileObjectLock;
	oldDeviceObject = reinterpret_cast<PDEVICE_OBJECT>(InterlockedExchange64(reinterpret_cast<PLONG64>(&FileObject->DeviceObject), reinterpret_cast<LONG64>(AfdUtil::FakeDeviceObject)));
	this->AddHookedFileObject(FileObject);
	//DBG_TRACE("HookFileObject", "Hooked file object!");

	// Hooking different device? Very bad news, shouldn't happen
	NT_ASSERT(AfdUtil::OriginalDeviceObject == oldDeviceObject);

	return TRUE;
}

BOOLEAN AfdUtil::IsFileObject(_In_ PVOID Object)
{
	POBJECT_TYPE objectType = ObGetObjectType(Object);

	if (objectType == NULL)
	{
		DBG_TRACE("IsFileObject", "Failed to get object type");
		return FALSE;
	}

	return objectType == *IoFileObjectType;
}

void AfdUtil::RestoreHookedFileObjects()
{
	DBG_TRACE("RestoreHookedFileObjects", "Restoring original file object device objects");
	for (unsigned int i = 0; i < AfdUtil::HookCount; i++)
	{
		AfdUtil::HookedFileObjects[i]->DeviceObject = AfdUtil::OriginalDeviceObject;
		PKMUTEX fileObjectLock = reinterpret_cast<PKMUTEX>(AfdUtil::HookedFileObjects[i]->FsContext2);
		ExFreePool(fileObjectLock);
	}
}

void AfdUtil::AddHookedFileObject(_In_ PFILE_OBJECT FileObject)
{
	AfdUtil::HookedFileObjects[AfdUtil::HookCount] = FileObject;
}

void AfdUtil::FindAndHookFileObjects()
{
	ULONG systemHandleInformationSize = 0x1000;
	PSYSTEM_HANDLE_INFORMATION systemHandleInformation = NULL;
	PFILE_OBJECT fileObject = NULL;
	NTSTATUS status;
	DBG_TRACE("FindFileObjects", "Finding file objects...");
	
	do 
	{
		if (systemHandleInformation)
		{
			ExFreePool(systemHandleInformation);
			systemHandleInformationSize *= 2;
		}

		systemHandleInformation = reinterpret_cast<PSYSTEM_HANDLE_INFORMATION>(ExAllocatePool(PagedPool, systemHandleInformationSize));
		if (systemHandleInformation == NULL)
		{
			DBG_TRACE("FindFileObjects", "Failed to allocate space for systemHandleInformation");
			return;
		}
	} while ((status = ZwQuerySystemInformation(SystemHandleInformation, systemHandleInformation, systemHandleInformationSize, NULL)) == STATUS_INFO_LENGTH_MISMATCH);

	if (NT_SUCCESS(status) == FALSE) {
		DBG_TRACE("FindFileObjects", "Call to ZwQuerySystemInformation failed");
	}

	for (unsigned int i = 0; i < systemHandleInformation->HandleCount; i++)
	{
		SYSTEM_HANDLE systemHandle = systemHandleInformation->Handles[i];
		if (systemHandle.Object == NULL) continue;
		if (IsFileObject(systemHandle.Object) == FALSE) continue;

		fileObject = static_cast<PFILE_OBJECT>(systemHandle.Object);
		
		// Could result in race condition. Try to wrap with try catch.
		if (fileObject->Size != sizeof(FILE_OBJECT))
		{
			DBG_TRACE("FindFileObjects", "Found FILE_OBJECT has invalid size");
			continue;
		}

		if (fileObject->DeviceObject == AfdUtil::OriginalDeviceObject)
		{
			if (this->HookFileObject(fileObject) == FALSE)
			{
				DBG_TRACE("FindAndHookFileObjects", "Failed to hook a file object");
				continue;
			}

			AfdUtil::HookCount++;
		}
	}

	ExFreePool(systemHandleInformation);
}