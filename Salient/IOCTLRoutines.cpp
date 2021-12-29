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

void PHideCommand(PVOID inputBuffer, PVOID outputBuffer, ULONG inBufferLen, ULONG outBufferLen)
{
	UNREFERENCED_PARAMETER(inputBuffer);
	UNREFERENCED_PARAMETER(inBufferLen);
	DBG_PRINT2("[PHideCommand] Asked to hide PID %s\n", (CHAR*) inputBuffer);
	
	CHAR* output = "Hiding process....";
	RtlCopyMemory(outputBuffer, output, outBufferLen);
	return;
}