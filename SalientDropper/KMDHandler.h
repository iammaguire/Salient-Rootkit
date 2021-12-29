#pragma once
#include <windows.h>
#include <stdio.h>
#include <wchar.h>

#define FILE_DEVICE_RK 0x00008001
#define CTL_CODE(DeviceType, Function, Method, Access) ( \
	((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)
#define IOCTL_PING_CMD \
CTL_CODE(FILE_DEVICE_RK, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PHIDE_CMD \
CTL_CODE(FILE_DEVICE_RK, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

class KMDHandler
{
private:
	HANDLE hDriver;

public:
	bool connectDriver(wchar_t* name);
	bool pingDriver();
	bool hideProcess(int pID);
};