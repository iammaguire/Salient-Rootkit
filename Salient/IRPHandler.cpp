#include "dbgmsg.h"
#include "IOCTLRoutines.h"
#include "IRPHandler.h"

NTSTATUS defaultDispatch(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS dispatchOpen(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	DBG_TRACE("Dispatch Open", "Device opened.");
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS dispatchClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	DBG_TRACE("Dispatch Close", "Device closed.");
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	
	return STATUS_SUCCESS;
}

NTSTATUS dispatchIOControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	PIO_STACK_LOCATION irpStack;
	PVOID inputBuffer;
	PVOID outputBuffer;
	ULONG inBufferLen;
	ULONG outBufferLen;
	ULONG ioctrlcode;
	NTSTATUS ntStatus;

	ntStatus = STATUS_SUCCESS;
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	inputBuffer = Irp->AssociatedIrp.SystemBuffer;
	outputBuffer = Irp->AssociatedIrp.SystemBuffer;

	// Grabs ptr to caller's stack location in the IRP for function codes and params

	irpStack = IoGetCurrentIrpStackLocation(Irp);
	inBufferLen = irpStack->Parameters.DeviceIoControl.InputBufferLength;
	outBufferLen = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
	ioctrlcode = irpStack->Parameters.DeviceIoControl.IoControlCode;

	DBG_TRACE("dispatchIOControl", "Received an IOCTL.");

	switch (ioctrlcode)
	{
	case IOCTL_PING_CMD:
		PingCommand(inputBuffer, outputBuffer, inBufferLen, outBufferLen);
		Irp->IoStatus.Information = outBufferLen;
		break;	
	case IOCTL_PHIDE_CMD:
		PHideCommand(inputBuffer, outputBuffer, inBufferLen, outBufferLen);
		Irp->IoStatus.Information = outBufferLen;
		break;
	default:
		DBG_TRACE("dispatchIOControl", "Received unknown control code.");
	}

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return ntStatus;
}