#include <fltKernel.h>
#include "system.h"

#define LINK_NAME L"\\DosDevices\\UserApp"
#define DEVICE_NAME L"\\Device\\DriverApp"

#define TARGET_NAME L"\\Driver\\Null"
#define TARGET_DEVICE_NAME L"\\Device\\Null"

PDEVICE_OBJECT MyDevice = NULL;
UNICODE_STRING DeviceLink;
UNICODE_STRING DeviceName;
PDRIVER_OBJECT pNullDriverObject;

HANDLE nullHandle, fileObjectHandle;
PDRIVER_OBJECT pNullObject;
PFILE_OBJECT pFileObject; 

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath) {
	NTSTATUS returnStatus = STATUS_SUCCESS;
	BOOLEAN bFindNull;

	RtlInitUnicodeString(&DeviceLink, LINK_NAME);
	RtlInitUnicodeString(&DeviceName, DEVICE_NAME);

	returnStatus = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN, 0, TRUE, &MyDevice);

	if (!NT_SUCCESS(returnStatus)) {
		DbgPrint("IoCreateDevice Fail! \n");
		return returnStatus;
	}
	DbgPrint("Success IoCreateDevice \n");

	returnStatus = IoCreateSymbolicLink(&DeviceLink, &DeviceName);
	if (!NT_SUCCESS(returnStatus)) {
		DbgPrint("IoCreateSymbolicLink Fail! \n");
		return returnStatus;
	}
	DbgPrint("Success IoCreateSymbolicLink \n");

	// 메모리에 로드된 드라이버 이름을 통해 DRIVER_OBJECT 구하기
	bFindNull = HookNullDriver();
	if (bFindNull == FALSE)
		DbgPrint("Cannot find null.sys \n");
	else
		DbgPrint("Success fine null.sys \n");

	// 샘플 파일 생성
	Sampling();

	// Null_DriverObject 참조 확인
	//PsLoadedModuleList(pNullObject);

	DriverObject->DriverUnload = OnUnload;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MyIOControl;		
	DriverObject->MajorFunction[IRP_MJ_CREATE] = Create_Handler;

	return returnStatus;
}

void PsLoadedModuleList(PDRIVER_OBJECT DriverObject)
{
	PLDR_DATA_TABLE_ENTRY pCur, EntryHeader;
	int i = 0;

	if ( DriverObject == NULL ) 
		return;

	pCur = *((PLDR_DATA_TABLE_ENTRY*)(DriverObject->DriverSection));
	if ( pCur == NULL )
		return;

	EntryHeader = pCur;
	do{
		if ( !MmIsAddressValid((PVOID)pCur) )
			break;
		if ( MmIsAddressValid((PVOID)pCur->EntryPoint) 
			&& MmIsAddressValid(pCur->FullImageName.Buffer) 
			&& MmIsAddressValid(pCur->BaseImageName.Buffer) )
		{
			DbgPrint("%02d 드라이버: %ws", i++, pCur->BaseImageName.Buffer);
		}
		pCur = (PLDR_DATA_TABLE_ENTRY)pCur->InLoadOrderLinks.Flink; 
	}
	while ( (PLDR_DATA_TABLE_ENTRY)pCur != EntryHeader );
}

void Sampling()
{
	HANDLE hReadHandle;
	HANDLE hWriteHandle;
	CHAR pFileBuffer[1024];
	NTSTATUS ntStatus;
	IO_STATUS_BLOCK IoStatusBlock;
	OBJECT_ATTRIBUTES ObjectAttributes;
	LARGE_INTEGER Offset;
	UNICODE_STRING uniWritFileName, FullImageName, mNullDirectory;
	PFILE_OBJECT mfileObject; 
	
	// L"\\\\.\\NULL"	=>	동작중인 드라이버의 인스턴스 핸들을 얻는다.	
	// L"\\??\\C:\\WINDOWS\\SYSTEM32\\DRIVERS\\NULL.Sys"	=>	null.sys 파일을 열어서 핸들을 얻는다.
	RtlInitUnicodeString (&FullImageName, L"\\??\\C:\\WINDOWS\\SYSTEM32\\DRIVERS\\NULL.Sys");
	InitializeObjectAttributes (&ObjectAttributes, &FullImageName, OBJ_CASE_INSENSITIVE, NULL, NULL);

	ntStatus = ZwCreateFile( &hReadHandle, 
		GENERIC_READ | SYNCHRONIZE,
		&ObjectAttributes,
		&IoStatusBlock, NULL,
		FILE_ATTRIBUTE_NORMAL, 
		FILE_SHARE_READ | FILE_SHARE_WRITE, 
		FILE_OPEN, 
		FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, 
		NULL, 
		0 );

	Offset.LowPart = 0;
	Offset.HighPart = 0;

	// 메모리에 로드되는 Null.sys를 C:\Sample.sys 파일로 백업 받는다.
	RtlInitUnicodeString (&uniWritFileName, L"\\??\\C:\\Sample.Sys");
	InitializeObjectAttributes (&ObjectAttributes, &uniWritFileName, OBJ_CASE_INSENSITIVE, NULL, NULL);

	ntStatus = ZwCreateFile( &hWriteHandle, 
		GENERIC_WRITE | SYNCHRONIZE | FILE_APPEND_DATA, 
		&ObjectAttributes, 
		&IoStatusBlock, 
		NULL,
		FILE_ATTRIBUTE_NORMAL, 
		FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_CREATE, 
		FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, 
		NULL, 
		0 );

	if (NT_SUCCESS(ntStatus) == FALSE)
	{
		DbgPrint("쓰기 파일을 생성 할 수 없음 %#x\n", ntStatus);
		ZwClose(hReadHandle);
		return ;
	}

	while (TRUE)
	{		
		ntStatus = ZwReadFile( hReadHandle,	NULL, NULL, NULL, &IoStatusBlock, pFileBuffer, 1024, &Offset, NULL );

		if ((NT_SUCCESS(ntStatus) == FALSE) || (IoStatusBlock.Information != 1024))
		{
			ntStatus = ZwWriteFile( hWriteHandle, NULL, NULL, NULL, &IoStatusBlock, pFileBuffer, IoStatusBlock.Information, NULL, NULL );

			if (NT_SUCCESS(ntStatus) == FALSE)
			{
				ZwClose (hWriteHandle);
				ZwClose (hReadHandle);
				return ;
			}

			DbgPrint("루프를 빠져나갑니다.\n");

			// 파일을 모두 작성하였으니 루프를 빠져나간다.
			ZwClose (hWriteHandle);
			ZwClose (hReadHandle);
			break;
		}
		else
		{
			ntStatus = ZwWriteFile( hWriteHandle, NULL, NULL, NULL, IoStatusBlock, pFileBuffer, 1024, NULL, NULL );

			if (NT_SUCCESS(ntStatus) == FALSE)
			{
				DbgPrint ("쓰기 실패...\n");
				ZwClose (hWriteHandle);
				ZwClose (hReadHandle);
				return ;
			}
		}

		Offset.LowPart += 1024;
	}
	DbgPrint("완료...\n");
}

BOOLEAN HookNullDriver()
{
	UNICODE_STRING TargetName, FileTargetName;
	NTSTATUS ntStatus;
	
	RtlInitUnicodeString(&TargetName, L"\\Driver\\NULL");
	RtlInitUnicodeString(&FileTargetName, L"\\Driver\\NULL");

	pNullDriverObject = SearchDriverObject(&TargetName);
	pFileObject = SearchFileObject(&FileTargetName); 

	DbgPrint("pNullDriverObject->DeviceObject = %X \n\n", pNullDriverObject->DeviceObject);
	DbgPrint("pNullDriverObject = %X \n", pNullDriverObject);

	DbgPrint("pFileObject = %X \n", pFileObject);
	DbgPrint("pFileObject->FileName = %ws \n\n", pFileObject->FileName);

	if( pNullDriverObject )
		return TRUE;
	else
		return FALSE;
}

PFILE_OBJECT SearchFileObject(PUNICODE_STRING pUni ) 
{   
	NTSTATUS nullStatus;
	OBJECT_ATTRIBUTES ObjectAttributes;	

	// InitializeObjectAttributes - 파일명을 객체(OBJECT_ATTRIBUTES) 구조로 초기화
	InitializeObjectAttributes( &ObjectAttributes, pUni, OBJ_CASE_INSENSITIVE, NULL, NULL );
	nullStatus = ObOpenObjectByName(&ObjectAttributes, 0L, 0L, 0L, 0L, 0L, &fileObjectHandle );

	if( nullStatus != STATUS_SUCCESS )
		return (PFILE_OBJECT)0;

	nullStatus = ObReferenceObjectByHandle(fileObjectHandle, 0x80000000, NULL, KernelMode, &pFileObject, NULL );
	if( nullStatus != STATUS_SUCCESS )
	{
		ZwClose( fileObjectHandle );
		DbgPrint("ObReferenceObjectByName Fail!!! \n");
		return (PFILE_OBJECT)0;
	}		
	return pFileObject;	
}   

PDRIVER_OBJECT SearchDriverObject(PUNICODE_STRING pUni )
{
	NTSTATUS nullStatus;
	OBJECT_ATTRIBUTES ObjectAttributes;	

	// InitializeObjectAttributes - 파일명을 객체(OBJECT_ATTRIBUTES) 구조로 초기화
	InitializeObjectAttributes( &ObjectAttributes, pUni, OBJ_CASE_INSENSITIVE, NULL, NULL );
	nullStatus = ObOpenObjectByName(&ObjectAttributes, 0L, 0L, 0L, 0L, 0L, &nullHandle );

	if( nullStatus != STATUS_SUCCESS )
		return (PDRIVER_OBJECT)0;

	// 0x80000000 = GENERIC_READ
	nullStatus = ObReferenceObjectByHandle(nullHandle, 0x80000000, NULL, 0, &pNullObject, NULL );
	if( nullStatus != STATUS_SUCCESS )
	{
		ZwClose( nullHandle );
		DbgPrint("ObReferenceObjectByName Fail!!! \n");
		return (PDRIVER_OBJECT)0;
	}		
	return pNullObject;	
}

VOID OnUnload(IN PDRIVER_OBJECT DriverObject) 
{
	ZwClose( nullHandle );
	ObDereferenceObject( pNullObject );
	IoDeleteDevice(MyDevice);
	IoDeleteSymbolicLink(&DeviceLink);
	DbgPrint("OnUnload Call!! \n");
}

NTSTATUS MyIOControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) 
{
	PIO_STACK_LOCATION pStack;
	NTSTATUS returnStatus = STATUS_SUCCESS;
	ULONG ControlCode;
	ULONG info = 0;
	ULONG cbin, cbout;
	HANDLE UserHandle;
	PKEVENT SharedEvent	= NULL;
	int i = 0;
	
	pStack = IoGetCurrentIrpStackLocation(Irp);
	ControlCode = pStack->Parameters.DeviceIoControl.IoControlCode;
	cbin    = pStack->Parameters.DeviceIoControl.InputBufferLength;
	cbout  = pStack->Parameters.DeviceIoControl.OutputBufferLength;

	switch (ControlCode)
	{
	case IOCTL_TEST:
		DbgPrint("IOCTL_TEST Call~~");
		break;
	case IOCTL_GET_VERSION_BUFFERED:
		{
			PCHAR pData = "Compare hash";
			PVOID outBuffer;

			if( cbout < strlen(pData) ){
				returnStatus = STATUS_INVALID_BUFFER_SIZE;
				break;
			}
			outBuffer = (PVOID)Irp->AssociatedIrp.SystemBuffer;
			RtlCopyMemory( outBuffer, pData, strlen(pData) );

			info = strlen( pData );
			break;
		}

	default:
		DbgPrint("[!] IOCTL : %d\n", ControlCode);
		break;
	}

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = info;    
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return returnStatus;
}

NTSTATUS Create_Handler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) 
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}
