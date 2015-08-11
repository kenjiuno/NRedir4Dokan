
#include "NRedir4Dokan.h"
#include <wdmsec.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, NRedUnload)
#pragma alloc_text (PAGE, NRedCreate)
#pragma alloc_text (PAGE, NRedDeviceControl)
#pragma alloc_text (PAGE, NRedDispatchFileSystemControl)
#pragma alloc_text (PAGE, SimRepAllocateUnicodeString)
#pragma alloc_text (PAGE, SimRepFreeUnicodeString)
#pragma alloc_text (PAGE, NRedMapPath)
#pragma alloc_text (PAGE, NRedIsIPC)
#pragma alloc_text (PAGE, NRedSwapUnicodeString)
#pragma alloc_text (PAGE, NRedUnicodeStringContains)

#endif

// ############################## SimRepAllocateUnicodeString

NTSTATUS
SimRepAllocateUnicodeString (
    PUNICODE_STRING String
    )
{

	PAGED_CODE();

	String->Buffer = reinterpret_cast<PWCH>(
		ExAllocatePoolWithTag(
			PagedPool,
			String->MaximumLength,
			SIMREP_STRING_TAG
			)
		);

	if (String->Buffer == NULL) {

		DDbgPrint("[SimRep]: Failed to allocate unicode string of size 0x%x\n",
			String->MaximumLength);

		return STATUS_INSUFFICIENT_RESOURCES;
	}

	String->Length = 0;

	return STATUS_SUCCESS;
}

// ############################## SimRepFreeUnicodeString

VOID
SimRepFreeUnicodeString (
    PUNICODE_STRING String
    )
{
	PAGED_CODE();

	if (String->Buffer) {
		ExFreePoolWithTag( String->Buffer, SIMREP_STRING_TAG );
		String->Buffer = NULL;
	}

	String->Length = String->MaximumLength = 0;
	String->Buffer = NULL;
}

// ############################## NRedUnicodeStringContains

BOOLEAN
NRedUnicodeStringContains(
	__in PUNICODE_STRING pFirst,
	LPCWSTR pSrch
	)
{
	int x = 0, cx = pFirst->Length;
	int t;
	LPCWSTR pStr = pFirst->Buffer;

	PAGED_CODE();

	if (pFirst == NULL || pSrch == NULL)
		return FALSE;

	for (; x < cx; x++) {
		if (RtlUpcaseUnicodeChar(pStr[x]) == RtlUpcaseUnicodeChar(pSrch[0])) {
			if (pSrch[0] == 0) {
				return TRUE;
			}
			t = 1;
			for (; x +t < cx; t++) {
				if (pSrch[t] == 0) {
					return TRUE;
				}
				if (RtlUpcaseUnicodeChar(pStr[x +t]) != RtlUpcaseUnicodeChar(pSrch[t])) {
					break;
				}
			}
		}
	}
	return FALSE;
}

// ############################## NRedSwapUnicodeString

VOID
NRedSwapUnicodeString(
	__in __out PUNICODE_STRING p1,
	__in __out PUNICODE_STRING p2
	)
{
	UNICODE_STRING tmp;
	
	PAGED_CODE();

	tmp.Length        = p1->Length;
	tmp.MaximumLength = p1->MaximumLength;
	tmp.Buffer        = p1->Buffer;

	p1->Length        = p2->Length;
	p1->MaximumLength = p2->MaximumLength;
	p1->Buffer        = p2->Buffer;

	p2->Length        = tmp.Length;
	p2->MaximumLength = tmp.MaximumLength;
	p2->Buffer        = tmp.Buffer;
}

// ############################## NRedMapPath

NTSTATUS
NRedMapPath(
	__in PNRED_GLOBAL pGlobal,
	__in PCUNICODE_STRING pSIn,
	__in __out PUNICODE_STRING pSOut,
	__out PBOOLEAN pbMapOk
	)
{
	// STATUS_SUCCESS
	// STATUS_OBJECT_PATH_NOT_FOUND
	
	HANDLE hkey = NULL;
	OBJECT_ATTRIBUTES objectAttribs;
	NTSTATUS status;
	PKEY_VALUE_FULL_INFORMATION pInfo = NULL;
	
	PAGED_CODE();
	
	if (pGlobal == NULL)
		return STATUS_INVALID_PARAMETER;
	if (pSIn == NULL)
		return STATUS_INVALID_PARAMETER;
	//if (pSOut == NULL)
	//	return STATUS_INVALID_PARAMETER;
	if (pbMapOk == NULL)
		return STATUS_INVALID_PARAMETER;
	
	*pbMapOk = FALSE;
	
	do {
		ULONG y = 0;
		const SIZE_T cbInfo = 4096;
		
		pInfo = (PKEY_VALUE_FULL_INFORMATION)ExAllocatePoolWithTag(PagedPool, cbInfo, NREG_REG_TAG);
		if (pInfo == NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
		
		InitializeObjectAttributes(&objectAttribs, &(pGlobal->RPMap), OBJ_KERNEL_HANDLE, NULL, NULL);
		status = ZwOpenKey(&hkey, GENERIC_READ, &objectAttribs);
		if (!NT_SUCCESS(status)) {
			break;
		}
		
		for (; y < 50; y++) {
			ULONG cb = 0;
			status = ZwEnumerateValueKey(hkey, y, KeyValueFullInformation, pInfo, cbInfo, &cb);
			if (!NT_SUCCESS(status))
				break;
			
			if (pInfo->Type == REG_SZ && (pInfo->NameLength) < 2*2048 && (pInfo->DataLength) < 2*2048) {
				UNICODE_STRING RName, RData, SInTmp;
				RName.Length = RName.MaximumLength = (USHORT)pInfo->NameLength; // bytes
				RName.Buffer = pInfo->Name;
				while (RName.Length >= 2 && RName.Buffer[(RName.Length / 2) -1] == 0) {
					RName.Length -= 2;
				}
				if (RName.Length != 0 && RName.Buffer[(RName.Length / 2) -1] == L'\\') {
					RName.Length -= 2;
				}
				RData.Length = RData.MaximumLength = (USHORT)pInfo->DataLength;
				RData.Buffer = (WCHAR *)(((UCHAR *)pInfo) + pInfo->DataOffset);
				while (RData.Length >= 2 && RData.Buffer[(RData.Length / 2) -1] == 0) {
					RData.Length -= 2;
				}
				if (RData.Length >= 2 && RData.Buffer[(RData.Length / 2) -1] == L'\\') {
					RData.Length -= 2;
				}
				
				if (!NRedUnicodeStringContains(&RData, L"Dokan")) {
					goto _Skip;
				}
				
				// RName: \dokanworld\server
				// SIn:   \dokanworld\server
				if (RtlEqualUnicodeString(&RName, pSIn, TRUE)) {
					*pbMapOk = TRUE;
					if (pSOut == NULL) {
						status = STATUS_INVALID_PARAMETER;
						goto _Done;
					}
					if (pSOut->MaximumLength < RData.Length) {
						status = STATUS_BUFFER_TOO_SMALL;
						goto _Done;
					}

					RtlCopyUnicodeString(pSOut, &RData);
					status = RtlUnicodeStringCatString(pSOut, L"\\");
					goto _Done;
				}

				// RName: \dokanworld\server
				// SIn:   \dokanworld\server\ 
				SInTmp.Length = pSIn->Length;
				SInTmp.MaximumLength = pSIn->MaximumLength;
				SInTmp.Buffer = pSIn->Buffer;
				if (RName.Length +2 == SInTmp.Length && SInTmp.Buffer[(SInTmp.Length / 2) -1] == L'\\') {
					SInTmp.Length -= 2;
					if (RtlEqualUnicodeString(&RName, &SInTmp, TRUE)) {
						*pbMapOk = TRUE;
						if (pSOut == NULL) {
							status = STATUS_INVALID_PARAMETER;
							goto _Done;
						}
						
						RtlCopyUnicodeString(pSOut, &RData);
						status = RtlUnicodeStringCatString(pSOut, L"\\");
						goto _Done;
					}
				}

				// RName: \dokanworld\server
				// SIn:   \dokanworld\server\123.txt
				if (RName.Length < (pSIn->Length) && pSIn->Buffer[(RName.Length / 2)] == L'\\') {
					SInTmp.Length = RName.Length;
					SInTmp.MaximumLength = pSIn->MaximumLength;
					SInTmp.Buffer = pSIn->Buffer;
					if (RtlEqualUnicodeString(&RName, &SInTmp, TRUE)) {
						*pbMapOk = TRUE;
						if (pSOut == NULL) {
							status = STATUS_INVALID_PARAMETER;
							goto _Done;
						}
						RtlCopyUnicodeString(pSOut, &RData);
						SInTmp.Buffer += RName.Length / 2;
						SInTmp.Length = pSIn->Length -(RName.Length);
						status = RtlUnicodeStringCat(pSOut, &SInTmp);
						goto _Done;
					}
				}
			}
		_Skip:
			;
		}
		status = STATUS_OBJECT_PATH_NOT_FOUND;
	} while (0);

_Done:
	if (hkey != NULL)
		ZwClose(hkey);
	if (pInfo != NULL)
		ExFreePoolWithTag(pInfo, NREG_REG_TAG);

	return status;
}

// ############################## NRedIsIPC

NTSTATUS
NRedIsIPC(
	__in PNRED_GLOBAL pGlobal,
	__in PCUNICODE_STRING pSIn,
	__out PBOOLEAN pbIsIPC
	)
{
	// STATUS_SUCCESS
	// STATUS_OBJECT_PATH_NOT_FOUND
	
	HANDLE hkey = NULL;
	OBJECT_ATTRIBUTES objectAttribs;
	NTSTATUS status;
	PKEY_VALUE_FULL_INFORMATION pInfo = NULL;
	
	PAGED_CODE();
	
	if (pGlobal == NULL)
		return STATUS_INVALID_PARAMETER;
	if (pSIn == NULL)
		return STATUS_INVALID_PARAMETER;
	if (pbIsIPC == NULL)
		return STATUS_INVALID_PARAMETER;
	
	*pbIsIPC = FALSE;
	
	do {
		ULONG y = 0;
		const SIZE_T cbInfo = 4096;
		
		pInfo = (PKEY_VALUE_FULL_INFORMATION)ExAllocatePoolWithTag(PagedPool, cbInfo, NREG_REG_TAG);
		if (pInfo == NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
		
		InitializeObjectAttributes(&objectAttribs, &(pGlobal->RPMap), OBJ_KERNEL_HANDLE, NULL, NULL);
		status = ZwOpenKey(&hkey, GENERIC_READ, &objectAttribs);
		if (!NT_SUCCESS(status)) {
			break;
		}
		
		UNICODE_STRING IpcName;
		RtlInitUnicodeString(&IpcName, L"ipc");
		
		for (; y < 50; y++) {
			ULONG cb = 0;
			status = ZwEnumerateValueKey(hkey, y, KeyValueFullInformation, pInfo, cbInfo, &cb);
			if (!NT_SUCCESS(status))
				break;
			
			if (pInfo->Type == REG_SZ && (pInfo->NameLength) < 2*2048 && (pInfo->DataLength) < 2*2048) {
				UNICODE_STRING RName, RData;
				RName.Length = RName.MaximumLength = (USHORT)pInfo->NameLength; // bytes
				RName.Buffer = pInfo->Name;
				while (RName.Length >= 2 && RName.Buffer[(RName.Length / 2) -1] == 0) {
					RName.Length -= 2;
				}
				if (RName.Length != 0 && RName.Buffer[(RName.Length / 2) -1] == L'\\') {
					RName.Length -= 2;
				}
				RData.Length = RData.MaximumLength = (USHORT)pInfo->DataLength;
				RData.Buffer = (WCHAR *)(((UCHAR *)pInfo) + pInfo->DataOffset);
				while (RData.Length >= 2 && RData.Buffer[(RData.Length / 2) -1] == 0) {
					RData.Length -= 2;
				}
				if (RData.Length >= 2 && RData.Buffer[(RData.Length / 2) -1] == L'\\') {
					RData.Length -= 2;
				}
				
				// RName: \dokanworld\server
				// SIn:   \dokanworld\server
				if (RtlEqualUnicodeString(&RName, pSIn, TRUE)) {
					if (RtlEqualUnicodeString(&RData, &IpcName, TRUE)) {
						*pbIsIPC = TRUE;
						status = STATUS_SUCCESS;
						goto _Done;
					}
				}
			}
		_Skip:
			;
		}
		status = STATUS_OBJECT_PATH_NOT_FOUND;
	} while (0);

_Done:
	if (hkey != NULL)
		ZwClose(hkey);
	if (pInfo != NULL)
		ExFreePoolWithTag(pInfo, NREG_REG_TAG);

	return status;
}

// ############################## NRedCreate

NTSTATUS
NRedCreate(
	__in PDEVICE_OBJECT fsDevice,
	__in PIRP Irp
	)
{
	PIO_STACK_LOCATION irpSp;
	NTSTATUS status = STATUS_INVALID_PARAMETER;
	PFILE_OBJECT fileObject;
	ULONG	 info = 0;
	PUNICODE_STRING pSIn;
	UNICODE_STRING SOut = {0};
	BOOLEAN bMapOk;
	PNRED_GLOBAL pGlobal;
	BOOLEAN bIsIPC;

	PAGED_CODE();

	__try {
		FsRtlEnterFileSystem();

		DDbgPrint("==> NRedCreate\n");

		irpSp = IoGetCurrentIrpStackLocation(Irp);
		fileObject = irpSp->FileObject;

		if (fileObject == NULL) {
			DDbgPrint("  fileObject == NULL\n");
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}

		pGlobal = reinterpret_cast<PNRED_GLOBAL>(fsDevice->DeviceExtension);
		
		DDbgPrint("  ProcessId %lu\n", IoGetRequestorProcessId(Irp));
		DDbgPrint("  FileName:%wZ\n", &fileObject->FileName);
		
		pSIn = &(fileObject->FileName);

		if (pSIn->Length == 0) {
			status = STATUS_SUCCESS;
			__leave;
		}

		status = NRedIsIPC(pGlobal, pSIn, &bIsIPC);
		if (NT_SUCCESS(status) && bIsIPC) {
			status = STATUS_SUCCESS;
			_leave;
		}
		
		SOut.MaximumLength = 2*2048; // bytes
		
		status = SimRepAllocateUnicodeString(&SOut);
		if (!NT_SUCCESS(status)) {
			__leave;
		}

		status = NRedMapPath(pGlobal, pSIn, &SOut, &bMapOk);
		if (!NT_SUCCESS(status)) {
			status = STATUS_ACCESS_DENIED;
			__leave;
		}
		if (!bMapOk) {
			status = STATUS_ACCESS_DENIED;
			__leave;
		}
		
		DDbgPrint("  Mapped:%wZ\n", &SOut);
		NRedSwapUnicodeString(pSIn, &SOut); // pSIn¨(free), SOut¨pSIn
		
		status = STATUS_REPARSE;

		__leave;
	}
	__finally {

		if (status != STATUS_PENDING) {
			Irp->IoStatus.Status = status;
			Irp->IoStatus.Information = info;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			DokanPrintNTStatus(status);
		}

		SimRepFreeUnicodeString(&SOut);

		DDbgPrint("<== NRedCreate\n");
		FsRtlExitFileSystem();
	}

	return status;
}

// ############################## PrintUnknownDeviceIoctlCode

VOID
PrintUnknownDeviceIoctlCode(
	__in ULONG	IoctlCode
	)
{
	PCHAR baseCodeStr = "unknown";
	ULONG baseCode = DEVICE_TYPE_FROM_CTL_CODE(IoctlCode);
	ULONG functionCode = (IoctlCode & (~0xffffc003)) >> 2;

	DDbgPrint("   Unknown Code 0x%x\n", IoctlCode);

	DDbgPrint("   BaseCode: 0x%x(%s) FunctionCode 0x%x(%d)\n",
		baseCode, baseCodeStr, functionCode, functionCode);
}

// ############################## NRedDeviceControl

NTSTATUS
NRedDeviceControl(
	__in PDEVICE_OBJECT fsDevice,
	__in PIRP Irp
	)
{
	PIO_STACK_LOCATION irpSp;
	NTSTATUS status = STATUS_NOT_IMPLEMENTED;
	PNRED_GLOBAL pGlobal;

	PAGED_CODE();

	__try {
		FsRtlEnterFileSystem();

		Irp->IoStatus.Information = 0;

		irpSp = IoGetCurrentIrpStackLocation(Irp);

		DDbgPrint("==> DokanDispatchIoControl\n");
		DDbgPrint("  ProcessId %lu\n", IoGetRequestorProcessId(Irp));

		pGlobal = reinterpret_cast<PNRED_GLOBAL>(fsDevice->DeviceExtension);

		switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
		case IOCTL_REDIR_QUERY_PATH:
			{
				UNICODE_STRING SIn;
				BOOLEAN bMapOk;
				BOOLEAN bIsIPC;
				
				PQUERY_PATH_REQUEST pReq = (PQUERY_PATH_REQUEST)irpSp->Parameters.DeviceIoControl.Type3InputBuffer;
				
				if (pReq->PathNameLength > 2*2048) { // bytes
					status = STATUS_ACCESS_DENIED;
					break;
				}
				
				SIn.Length = SIn.MaximumLength = (USHORT)pReq->PathNameLength; // bytes
				SIn.Buffer = pReq->FilePathName;
				
				DDbgPrint("  IOCTL_REDIR_QUERY_PATH vcb \n");
				DDbgPrint("    PathNameLength = %lu \n", pReq->PathNameLength);
				DDbgPrint("    FilePathName = %wZ \n", &SIn);
				
				status = NRedIsIPC(pGlobal, &SIn, &bIsIPC);
				DDbgPrint("    IsIPC = %lu \n", 0UL + bIsIPC);
				if (NT_SUCCESS(status) && bIsIPC) {

				}
				else {
					status = NRedMapPath(pGlobal, &SIn, NULL, &bMapOk); // shall return STATUS_INVALID_PARAMETER
					if (!bMapOk) {
						status = STATUS_BAD_NETWORK_PATH;
						break;
					}
				}
				
				PQUERY_PATH_RESPONSE pRes = (PQUERY_PATH_RESPONSE)Irp->UserBuffer;
				pRes->LengthAccepted = SIn.Length; // bytes
				
				status = STATUS_SUCCESS;
			}
			break;

		default:
			{
				PrintUnknownDeviceIoctlCode(irpSp->Parameters.DeviceIoControl.IoControlCode);
				status = STATUS_NOT_IMPLEMENTED;
			}
			break;
		} // switch IoControlCode
	
	} __finally {

		if (status != STATUS_PENDING) {
			//
			// complete the Irp
			//
			Irp->IoStatus.Status = status;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
		}

		DokanPrintNTStatus(status);
		DDbgPrint("<== DokanDispatchIoControl\n");

		FsRtlExitFileSystem();
	}

	return status;
}

// ############################## NRedDispatchFileSystemControl

NTSTATUS
NRedDispatchFileSystemControl(
	__in PDEVICE_OBJECT fsDevice,
	__in PIRP Irp
	)
{
	NTSTATUS			status = STATUS_INVALID_PARAMETER;
	PIO_STACK_LOCATION	irpSp;
	ULONG_PTR			info = 0;

	PAGED_CODE();

	__try {
		FsRtlEnterFileSystem();

		DDbgPrint("==> NRedFileSystemControl\n");
		DDbgPrint("  ProcessId %lu\n", IoGetRequestorProcessId(Irp));

		irpSp = IoGetCurrentIrpStackLocation(Irp);

		DWORD CtlCode;

		switch(irpSp->MinorFunction) {
		case IRP_MN_KERNEL_CALL:
			DDbgPrint("	 IRP_MN_KERNEL_CALL\n");
			break;

		case IRP_MN_LOAD_FILE_SYSTEM:
			DDbgPrint("	 IRP_MN_LOAD_FILE_SYSTEM\n");
			break;

		case IRP_MN_MOUNT_VOLUME:
			DDbgPrint("	 IRP_MN_MOUNT_VOLUME\n");
			break;

		case IRP_MN_USER_FS_REQUEST:
			CtlCode = irpSp->Parameters.FileSystemControl.FsControlCode;
			DDbgPrint("	 IRP_MN_USER_FS_REQUEST %08lX \n", CtlCode);
			
			switch (CtlCode) {
			case 0x00060194 : //FSCTL_DFS_GET_REFERRALS:
				status = STATUS_DFS_UNAVAILABLE;
				break;
			}
			break;

		case IRP_MN_VERIFY_VOLUME:
			DDbgPrint("	 IRP_MN_VERIFY_VOLUME\n");
			break;

		default:
			DDbgPrint("  unknown %d\n", irpSp->MinorFunction);
			break;

		}

	} __finally {
		
		if (status != STATUS_PENDING) {
			Irp->IoStatus.Status = status;
			Irp->IoStatus.Information = info;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
		}

		DokanPrintNTStatus(status);
		DDbgPrint("<== NRedFileSystemControl\n");

		FsRtlExitFileSystem();
	}

	return status;
}

// ############################## DriverEntry

NTSTATUS
DriverEntry(
	__in PDRIVER_OBJECT  DriverObject,
	__in PUNICODE_STRING RegistryPath
	)
{
	WCHAR deviceNameBuf[] = NRED_GLOBAL_DEVICE_NAME; 
	WCHAR symbolicLinkBuf[] = NRED_GLOBAL_SYMBOLIC_LINK_NAME;
	UNICODE_STRING fsDeviceName;
	UNICODE_STRING symbolicLinkName;
	PDEVICE_OBJECT fsDevice = NULL;
	NTSTATUS status;
	PNRED_GLOBAL pGlobal = NULL;

	RtlInitUnicodeString(&fsDeviceName, deviceNameBuf);
	RtlInitUnicodeString(&symbolicLinkName, symbolicLinkBuf);

	do {
		status = IoCreateDeviceSecure(
					DriverObject,         // DriverObject
					sizeof(NRED_GLOBAL), // DeviceExtensionSize
					&fsDeviceName,        // fsDeviceName
					FILE_DEVICE_NETWORK_FILE_SYSTEM, // DeviceType
					0,                    // DeviceCharacteristics
					FALSE,                // Not Exclusive
					&SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R, // Default SDDL String
					NULL,                // Device Class GUID
					&fsDevice);	          // fsDevice

		if (!NT_SUCCESS(status)) {
			DDbgPrint("  IoCreateDevice returned 0x%x\n", status);
			break;
		}
		
		DDbgPrint("myGlobalDevice: %wZ created\n", &fsDeviceName);
		
		status = IoCreateSymbolicLink(&symbolicLinkName, &fsDeviceName);
		if (!NT_SUCCESS(status)) {
			DDbgPrint("  IoCreateSymbolicLink returned 0x%x\n", status);
			break;
		}
		
		DDbgPrint("SymbolicLink: %wZ -> %wZ created\n", &fsDeviceName, &symbolicLinkName);

		pGlobal = reinterpret_cast<PNRED_GLOBAL>(fsDevice->DeviceExtension);
		RtlZeroMemory(pGlobal, sizeof(NRED_GLOBAL));

		pGlobal->DeviceObject = fsDevice;

		pGlobal->Identifier.Type = DGL;
		pGlobal->Identifier.Size = sizeof(NRED_GLOBAL);
			
		pGlobal->RPMap.MaximumLength = RegistryPath->MaximumLength +2*1 +2*3 +2*1; // bytes
			
		status = SimRepAllocateUnicodeString(&(pGlobal->RPMap));
		if (!NT_SUCCESS(status)) {
			DDbgPrint("  SimRepAllocateUnicodeString returned 0x%x\n", status);
			break;
		}
		
		RtlCopyUnicodeString(&(pGlobal->RPMap), RegistryPath);
		
		status = RtlUnicodeStringCatString(&(pGlobal->RPMap), L"\\Map");
		if (!NT_SUCCESS(status)) {
			break;
		}
		
		//
		// Set up dispatch entry points for the driver.
		//
		DriverObject->DriverUnload = NRedUnload;

		DriverObject->MajorFunction[IRP_MJ_CREATE] = NRedCreate;

		DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = NRedDeviceControl;
		DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL]	= NRedDispatchFileSystemControl;

		fsDevice->Flags |= DO_DIRECT_IO;
		fsDevice->Flags &= ~DO_DEVICE_INITIALIZING;

		status = FsRtlRegisterUncProvider(&(pGlobal->MupHandle), &(fsDeviceName), FALSE);
		if (!NT_SUCCESS(status)) {
			DDbgPrint("FsRtlRegisterUncProvider failed: 0x%X\n", status);
			break;
		}

		DDbgPrint("<== DriverEntry\n");
		
		status = STATUS_SUCCESS;
	} while (0);
	
	if (status != STATUS_SUCCESS) {
		IoDeleteSymbolicLink(&symbolicLinkName);
		if (fsDevice != NULL) {
			IoDeleteDevice(fsDevice);
		}
		if (pGlobal != NULL) {
			SimRepFreeUnicodeString(&(pGlobal->RPMap));
		}
	}
	
	return( status );
}

// ############################## NRedUnload

VOID
NRedUnload(
	__in PDRIVER_OBJECT DriverObject
	)
{
	PDEVICE_OBJECT fsDevice = DriverObject->DeviceObject;
	WCHAR symbolicLinkBuf[] = NRED_GLOBAL_SYMBOLIC_LINK_NAME;
	UNICODE_STRING symbolicLinkName;

	PAGED_CODE();
	DDbgPrint("==> NRedUnload\n");

	if (GetIdentifierType(fsDevice->DeviceExtension) == DGL) {
		PNRED_GLOBAL pGlobal = (PNRED_GLOBAL)fsDevice->DeviceExtension;
		DDbgPrint("  Delete Global fsDevice\n");
		FsRtlDeregisterUncProvider(pGlobal->MupHandle);
		RtlInitUnicodeString(&symbolicLinkName, symbolicLinkBuf);
		IoDeleteSymbolicLink(&symbolicLinkName);
		IoDeleteDevice(fsDevice);
		SimRepFreeUnicodeString(&pGlobal->RPMap);
	}

	DDbgPrint("<== NRedUnload\n");
	return;
}

// ############################## DokanPrintNTStatus

#define PrintStatus(val, flag) if(val == flag) DDbgPrint("  status = " #flag "\n")

VOID
DokanPrintNTStatus(NTSTATUS Status)
{
	PrintStatus(Status, STATUS_SUCCESS);
	PrintStatus(Status, STATUS_NO_MORE_FILES);
	PrintStatus(Status, STATUS_END_OF_FILE);
	PrintStatus(Status, STATUS_NO_SUCH_FILE);
	PrintStatus(Status, STATUS_NOT_IMPLEMENTED);
	PrintStatus(Status, STATUS_BUFFER_OVERFLOW);
	PrintStatus(Status, STATUS_FILE_IS_A_DIRECTORY);
	PrintStatus(Status, STATUS_SHARING_VIOLATION);
	PrintStatus(Status, STATUS_OBJECT_NAME_INVALID);
	PrintStatus(Status, STATUS_OBJECT_NAME_NOT_FOUND);
	PrintStatus(Status, STATUS_OBJECT_NAME_COLLISION);
	PrintStatus(Status, STATUS_OBJECT_PATH_INVALID);
	PrintStatus(Status, STATUS_OBJECT_PATH_NOT_FOUND);
	PrintStatus(Status, STATUS_OBJECT_PATH_SYNTAX_BAD);
	PrintStatus(Status, STATUS_ACCESS_DENIED);
	PrintStatus(Status, STATUS_ACCESS_VIOLATION);
	PrintStatus(Status, STATUS_INVALID_PARAMETER);
	PrintStatus(Status, STATUS_INVALID_USER_BUFFER);
	PrintStatus(Status, STATUS_INVALID_HANDLE);
	PrintStatus(Status, STATUS_REPARSE);
	PrintStatus(Status, STATUS_BAD_NETWORK_PATH);
}
