#include <winsock2.h>
#include <Windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <stdio.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(linker, "/INCREMENTAL:NO")

#define ProcessThreadStackAllocation 0x29
struct STACK_ALLOCATION_COMPACT
{
	unsigned long dwReserveSize;
	unsigned long ZeroBits;
	unsigned long ReturnedBase;
};

typedef  DWORD(__stdcall *pZwSetInformationProcess)(HANDLE, unsigned long, void*, unsigned long);
typedef  DWORD(__stdcall *pfZwDelayExecution)(BOOLEAN, __int64*);
pfZwDelayExecution			ZwDelayExecution;
pZwSetInformationProcess	ZwSetInformationProcess;

void ApisInit()
{
	HINSTANCE hinstLib, hinstLib2;

	// Get a handle to the DLL module.
	hinstLib = LoadLibrary(TEXT("ntdll"));

	// Get the pointer to the function
	ZwDelayExecution		= (pfZwDelayExecution)		GetProcAddress(hinstLib, "ZwDelayExecution");
	ZwSetInformationProcess = (pZwSetInformationProcess)GetProcAddress(hinstLib, "ZwSetInformationProcess");
}

void showMessage()
{
	int num = 0;
	
	printf("%d- Delay using Sleep...\n", ++num);
	printf("%d- Delay using SleepEx...\n", ++num);
	printf("%d- Delay using ZwDelayExecution...\n", ++num);
	printf("%d- Delay using ping...\n", ++num);
	printf("%d- Delay using custom ping...\n", ++num);
	printf("%d- Delay using Loopless Repeatition...\n", ++num);
	printf("\n");
	printf("Please enter a choice: ");
}

/* perform ping using IcmpSendEcho*/
void customPing(int Timeout)
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

	//========= begin dummy code ======== //
	// dummy code here can be assembly instructions or C language. However, it must consider naked functions restrictions.
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
	//========= end dummy code ========== //

	static LPVOID new_addr = NULL;
	static LPVOID prev_addr = NULL;		// Address of the previously allocated function to delete.
	static int rep = 200000;			// How many times to repeat.
	static STACK_ALLOCATION_COMPACT Stk;
	static int func_size = 0;			// The size of this function is bytes.

	// free previous region
	if (prev_addr)	VirtualFree(prev_addr, 0, MEM_RELEASE);
	
	// is it the end of loop?
	if (--rep <= 0)
		__asm ret;
	
	// save previoius address to be freed in next iteration
	prev_addr = new_addr;

	// get function size
	__asm {
		mov eax, end
		sub eax, dword ptr [looplessRepeatition]
		mov func_size, eax
	}

	// allocate a new instance of this function. To get random addresses, credits go to: http://waleedassar.blogspot.com/2013/01/a-real-random-virtualalloc.html
	Stk.ReturnedBase = Stk.ZeroBits = 0;
	Stk.dwReserveSize = func_size;
	if (ZwSetInformationProcess((HANDLE)-1, ProcessThreadStackAllocation, &Stk, sizeof(Stk)) < 0) {
		__asm ret;	// error
	}
	new_addr = VirtualAlloc((LPVOID)Stk.ReturnedBase, func_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!new_addr) {
		__asm ret; // error
	}

	// copy the function to the new location
	__asm {
		mov esi, dword ptr[looplessRepeatition]
		mov edi, new_addr
		mov ecx, func_size
		rep movsb
		jmp new_addr
	}
end:
	__asm nop; // dummy line to get the compiler not to error on the label "end"
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
		// delay time can be controlled by the option -n [number]
		printf("=== Delay using ping ...\n");
		system("ping 1.1.1.1 -n 1 >null 2>&1");
		break; 

	case 5:
		printf("=== Delay using Custom ping via IcmpSendEcho() ...\n");
		customPing(1000);
		break;
		
	case 6:
		printf("=== Delay using Loopless Repeation...\n");
		looplessRepeatition();
		break;

	default:
		printf("No choice was selected.\n");
	}

	return 0;

}