#pragma once
#include "common.h"

// For DeviceType developers can define custom values 0x8000-0xFFFF
#define FILE_DEVICE_RK 0x00008001

/*
 * DeviceType: Represents underlying hardware
 * Function: Program specific value defining which action is to be performed
 * Method: Defines how data will pass between user and kernel mode (METHOD_BUFFERED is non paged system buffer)
 * Access: Type of access caller must request when opening file object that represents the device. 
 *		   FILE_READ_DATA allows KMD to transfer from its device to system mem. FILE_WRITE_DATA allows KMD to transfer system mem to its device
 */
#define CTL_CODE(DeviceType, Function, Method, Access) ( \
	((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

#define IOCTL_PING_CMD \
CTL_CODE(FILE_DEVICE_RK, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PHIDE_CMD \
CTL_CODE(FILE_DEVICE_RK, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

#ifdef __cplusplus
extern "C" {
#endif
	NTSTATUS defaultDispatch(_In_ PDEVICE_OBJECT, _In_ PIRP);
	NTSTATUS dispatchIOControl(_In_ PDEVICE_OBJECT, _In_ PIRP);
	NTSTATUS dispatchOpen(_In_ PDEVICE_OBJECT, _In_ PIRP);
	NTSTATUS dispatchClose(_In_ PDEVICE_OBJECT, _In_ PIRP);
#ifdef __cplusplus
}
#endif