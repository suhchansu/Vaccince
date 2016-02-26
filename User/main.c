#define _CRT_SECURE_NO_DEPRECATE
#include "system.h"

int main(int argc, char *argv[])
{
	if ( DriverStart())
		if ( CompareHash(argc, argv[1]) )
			Treatment();

	getchar();
	
	return 0;
}

int DriverStart()
{
	HANDLE dHandle;
	WCHAR DeviceLink[] = L"\\\\.\\UserApp";
	DWORD dwRet, dwKernelRet;
	PCHAR data[128];

	dHandle = CreateFileW(DeviceLink, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (dHandle == INVALID_HANDLE_VALUE) {
		printf("Get Device Handle Fail! : 0x%X \n", GetLastError());
		return 0;
	}else
		printf("handle success : %d \n", dHandle);

	if (!DeviceIoControl(dHandle, IOCTL_TEST, 0, 0, 0, 0, &dwRet, 0)) {
		printf("DeviceIOControl Fail!! \n");
		CloseHandle(dHandle);
		return 0;
	}

	if (DeviceIoControl(dHandle, IOCTL_GET_VERSION_BUFFERED, NULL, 0, (PVOID)data, sizeof(data), &dwKernelRet, NULL)) {
		printf("BytesReturned:[%d]\n", dwKernelRet);
		printf("IOCTL_GET_VERSION_BUFFERED : [%s]\n", data);
	}else
		printf("Error %d in IOCTL_GET_VERSION_BUFFERED call\n", GetLastError());

	CloseHandle(dHandle);
	return 1;
}

int CompareHash(int argc, char *argv)
{
	int i, j;
	SHA1_CTX context;
	unsigned char digest[20], buffer[16384];
	FILE* file;

	if (argc > 2) {
		printf("SHA-1 ½ÇÇà \n");
		exit(0);
	}
	if (argc < 2) {
		file = stdin;
	}
	else {
		if (!(file = fopen(argv[1], "rb"))) {
			printf("Open File Fail !!! \n", stderr);
			return 0;
		}
	}
	SHA1Init(&context);
	while (!feof(file)) { 
		i = fread(buffer, 1, 16384, file);
		SHA1Update(&context, buffer, i);
	}
	SHA1Final(digest, &context);
	fclose(file);
	for (i = 0; i < 5; i++) {
		for (j = 0; j < 4; j++) {
			printf("%02X", digest[i*4+j]);
		}
		putchar(' ');
	}
	putchar('\n');
	exit(0);

	return 1;
}

void Treatment()
{
	CopyFile(L"C:\\windows\\system32\\dllcache\\null.sys",L"C:\\windows\\system32\\drivers\\null.sys", FALSE);
}