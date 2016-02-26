#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <ntdef.h>
#include <ntddk.h>
#include <WINDEF.H>

#define IOCTL_TEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x4000, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_VERSION_BUFFERED CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS) 

VOID OnUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS MyIOControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS Create_Handler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS MyClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);

BOOLEAN HookNullDriver();
PDRIVER_OBJECT SearchDriverObject(PUNICODE_STRING pUni );
PDRIVER_OBJECT SearchFileObject(PUNICODE_STRING pUni );

void Sampling();
void PsLoadedModuleList(PDRIVER_OBJECT DriverObject);

NTSTATUS
ObOpenObjectByName (
					IN POBJECT_ATTRIBUTES ObjectAttributes,
					IN ULONG  ObjectType OPTIONAL,
					IN ULONG AccessMode,
					IN ULONG PassedAccessState,
					IN ULONG DesiredAccess OPTIONAL,
					IN OUT PVOID ParseContext OPTIONAL,
					OUT PHANDLE Handle
					);

typedef struct _LDR_DATA_TABLE_ENTRY { 
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	ULONG_PTR ImageBase;
	ULONG_PTR EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullImageName;
	UNICODE_STRING BaseImageName;
	ULONG Flags;
	SHORT LoadCount;
	WORD Fill;
	LIST_ENTRY HashLinks;
	ULONG_PTR SectionPointer;
	ULONG CheckSum;
	ULONG TimeDateStamp;
	ULONG_PTR LoadedImports;
	ULONG_PTR EntryPointActivationContext;
	ULONG_PTR PatchInformation;
#ifdef WINVER >= 0x600
	LIST_ENTRY ForwarderLinks;
	LIST_ENTRY ServiceTagLinks;
	LIST_ENTRY StaticLinks;
#endif
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

#endif 
