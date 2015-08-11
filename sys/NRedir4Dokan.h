
#ifndef _NREDIR4DOKAN_H_
#define _NREDIR4DOKAN_H_

#include <ntifs.h>
#include <ntdddisk.h>
#include <ntstrsafe.h>

//
// DEFINES
//

#define NRED_GLOBAL_DEVICE_NAME        L"\\Device\\NRedir4Dokan"
#define NRED_GLOBAL_SYMBOLIC_LINK_NAME L"\\DosDevices\\Global\\NRedir4Dokan"

#if _WIN32_WINNT > 0x501
	#define DDbgPrint(...) \
		{ KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "[NRedir4Dokan] " __VA_ARGS__ )); }
#else
	#define DDbgPrint(...) \
		{ DbgPrint("[NRedir4Dokan] " __VA_ARGS__); }
#endif

#define SIMREP_STRING_TAG  'NRed'
#define NREG_REG_TAG       'NR_r'

//
// FSD_IDENTIFIER_TYPE
//
// Identifiers used to mark the structures
//
typedef enum _FSD_IDENTIFIER_TYPE {
    DGL = ':DGL', // Dokan Global
} FSD_IDENTIFIER_TYPE;

//
// FSD_IDENTIFIER
//
// Header put in the beginning of every structure
//
typedef struct _FSD_IDENTIFIER {
    FSD_IDENTIFIER_TYPE     Type;
    ULONG                   Size;
} FSD_IDENTIFIER, *PFSD_IDENTIFIER;


#define GetIdentifierType(Obj) (((PFSD_IDENTIFIER)Obj)->Type)


//
// DATA
//

typedef struct _NRED_GLOBAL {
	FSD_IDENTIFIER	Identifier;
	PDEVICE_OBJECT	DeviceObject;
	HANDLE					MupHandle;
	UNICODE_STRING  RPMap; // registry mapping path
} NRED_GLOBAL, *PNRED_GLOBAL;

extern "C" DRIVER_INITIALIZE DriverEntry;

extern "C" __drv_dispatchType(IRP_MJ_CREATE)          DRIVER_DISPATCH NRedCreate;
extern "C" __drv_dispatchType(IRP_MJ_DEVICE_CONTROL)  DRIVER_DISPATCH NRedDeviceControl;
extern "C" __drv_dispatchType(IRP_MJ_FILE_SYSTEM_CONTROL) DRIVER_DISPATCH NRedDispatchFileSystemControl;

extern "C" DRIVER_UNLOAD NRedUnload;

extern "C" NTSTATUS
SimRepAllocateUnicodeString (
    PUNICODE_STRING String
    );

extern "C" VOID
SimRepFreeUnicodeString (
    PUNICODE_STRING String
    );

extern "C" NTSTATUS
NRedMapPath(
	__in PNRED_GLOBAL pGlobal,
	__in PCUNICODE_STRING pSIn,
	__in __out PUNICODE_STRING pSOut,
	__out PBOOLEAN pbMapOk
	);

extern "C" NTSTATUS
NRedIsIPC(
	__in PNRED_GLOBAL pGlobal,
	__in PCUNICODE_STRING pSIn,
	__out PBOOLEAN pbIsIPC
	);

extern "C" VOID
NRedSwapUnicodeString(
	__in __out PUNICODE_STRING p1,
	__in __out PUNICODE_STRING p2
	);

extern "C" BOOLEAN
NRedUnicodeStringContains(
	__in PUNICODE_STRING pFirst,
	LPCWSTR pSrch
	);

VOID
DokanPrintNTStatus(NTSTATUS Status);

#endif // _NREDIR4DOKAN_H_
