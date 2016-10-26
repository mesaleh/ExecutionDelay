#include <winsock2.h>
#include <Windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <stdio.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(linker, "/INCREMENTAL:NO")
// /INCREMENTAL[:NO]

typedef  DWORD(__stdcall *pfZwDelayExecution)(BOOLEAN, __int64*);
pfZwDelayExecution ZwDelayExecution;

void ApisInit()
{
	HINSTANCE hinstLib, hinstLib2;

	// Get a handle to the DLL module.
	hinstLib = LoadLibrary(TEXT("ntdll"));

	// Get the pointer to the function
	ZwDelayExecution = (pfZwDelayExecution)GetProcAddress(hinstLib, "ZwDelayExecution");
}

void showMessage()
{
	int num = 0;
	
	printf("%d- Delay using Sleep...\n", ++num);
	printf("%d- Delay using SleepEx...\n", ++num);
	printf("%d- Delay using ZwDelayExecution...\n", ++num);
	printf("%d- Delay using ping...\n", ++num);
	printf("\n");
	printf("Please enter a choice: ");
}

/* perform ping using WinAPIs*/
void ping(int Timeout)
{
	HANDLE hIcmpFile;
	unsigned long ipaddr;
	DWORD dwRetVal = 0;
	LPVOID ReplyBuffer = NULL;
	DWORD ReplySize = 0;

	// Use invalid IP to get the maximum timeout
	ipaddr = inet_addr("1.1.1.1");	

	hIcmpFile = IcmpCreateFile();
	if (hIcmpFile == INVALID_HANDLE_VALUE) {
		printf("\tUnable to open handle.\n");
		printf("IcmpCreatefile returned error: %ld\n", GetLastError());
		return;
	}

	ReplySize = sizeof(ICMP_ECHO_REPLY) + 8;
	ReplyBuffer = (VOID*)malloc(ReplySize);
	dwRetVal = IcmpSendEcho(hIcmpFile, ipaddr, NULL, 0, NULL, ReplyBuffer, ReplySize, Timeout);
}

__declspec(naked) void __fastcall looplessRepeatition() 
{
	static LPVOID new_addr = NULL;
	static LPVOID prev_addr = NULL;		// Address of the previously allocated function to delete.
	static int rep = 1000000;			// How many times to repeat.

	if (prev_addr)	VirtualFree(prev_addr, 0, MEM_RELEASE);

	//dummy code
	__asm {
		nop
		nop
		nop
		mov eax, eax
		mov ebx, ebx
		nop
		nop
		nop
	}
		
	if (--rep <= 0)
		__asm ret;
	
	prev_addr = new_addr;
	// allocate a new instance of this function, check this to enhance and get random addresses: http://waleedassar.blogspot.com/2013/01/a-real-random-virtualalloc.html
	new_addr = VirtualAlloc(NULL, 0x1000, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	// copy the function to the new location
	__asm {
		mov esi, dword ptr [looplessRepeatition]
		mov edi, new_addr
		mov ecx, 0x1000
		rep movsb
	}	
	__asm jmp new_addr;
}

int main()
{
	int num = 0;
	int choice = 0;
	__int64 x = -10000000;	// sleep for 1 seconds (unit is 100ns)
	ApisInit();

	showMessage();
	scanf("%d", &choice);

	switch (choice)
	{
	case 1:
		printf("=== Delay using Sleep...\n");
		Sleep(1000);
		break;

	case 2:
		printf("=== Delay using SleepEx...\n");
		SleepEx(1000, FALSE);
		break;

	case 3:
		printf("=== Delay using ZwDelayExecution...\n");
		ZwDelayExecution(FALSE, &x);
		break;

	case 4:
		printf("=== Delay using ping...\n");
		ping(1000);
		break;

	case 5:
		printf("=== Delay using Loopless repeation...\n");
		looplessRepeatition();
		break;

	default:
		printf("No choice was selected.\n");
	}

	return 0;

}