/*
 * krembo.c, Demonstration of inline hooking (aka. Detouring) within driver/kernel.
 * - ik@ikotler.org
 */

#include <ntddk.h>

// Assembly JMP opcode value

#ifndef X86_JMP
#define X86_JMP 0xE9
#endif

// Number of bytes to overwrite

#ifndef JMP_LENGTH
#define JMP_LENGTH sizeof(X86_JMP) + sizeof(int)
#endif

// Size of a page (4k)

#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif

// Pointers macros

#define PAGE_MASK(ptr) (ptr & 0xFFFFF000)

#define OFFSET_IN_PAGE(ptr) (ptr - (ptr & 0xFFFFF000))

// RtlRandom pointer prototype

typedef unsigned long (*RTLRANDOM)(
	unsigned long *seed
);

// Pointers to duplicated (original) and current versions of RtlRandom

RTLRANDOM pDupRtlRandom;
RTLRANDOM pCurRtlRandom;

/*
 * DetourFunction, Detour a function (Install Detour)
 * * pFcnAddr, Pointer to the soon-to-be-detoured function
 * * iHookAddr, Address of the hook function
 * * iPoolType, Pool type of allocated memory
 */

void *DetourFunction(char *pFcnAddr, int iHookAddr, int iPoolType) {
	void *pOrigPage;
		
	// Allocate a `PAGE_SIZE` for the original function (duplicate)
	
	pOrigPage = ExAllocatePoolWithTag(iPoolType, PAGE_SIZE, 0xdeadbeef);
	
	// Is there enough resources?
	
	if (pOrigPage == NULL) {
		return NULL;
	}
	
	// Duplicate the entire page
	
	RtlCopyMemory(pOrigPage, (char *)PAGE_MASK((int)pFcnAddr), PAGE_SIZE);
	
	// Calculate the relocation to `iHookAddr`
	
	iHookAddr -= ((int)pFcnAddr + 5);
	
	_asm
	{
		CLI					// disable interrupt
		MOV EAX, CR0		// move CR0 register into EAX
		AND EAX, NOT 10000H // disable WP bit 
		MOV CR0, EAX		// write register back 
	}
	
	// Overwrite the first `JMP_LENGTH` bytes of pFcnAddr (Detour it) 
	
	*(pFcnAddr) = X86_JMP;
	*(pFcnAddr+1) = (iHookAddr & 0xFF);
	*(pFcnAddr+2) = (iHookAddr >> 8) & 0xFF;
	*(pFcnAddr+3) = (iHookAddr >> 16) & 0xFF;
	*(pFcnAddr+4) = (iHookAddr >> 24) & 0xFF;
	
	_asm 
	{
		MOV EAX, CR0		// move CR0 register into EAX
		OR EAX, 10000H		// enable WP bit 
		MOV CR0, EAX		// write register back 
		STI					// enable interrupt
	}
	
	// Return pointer to the duplicate function (within the duplicate page)
	
	return (void *)((int)pOrigPage + OFFSET_IN_PAGE((int)pFcnAddr));
}

/*
 * MyRtlRandom, RtlRandom hook
 * * seed, given seed
 */

unsigned long MyRtlRandom(unsigned long *seed) {
	unsigned long retval;

	KdPrint(("MyRtlRandom invoked with seed = %d\n", *seed));
	
	retval = pDupRtlRandom(seed);
	
	KdPrint(("Returning value = %d, from OldRtlRandom(%d)\n", retval, *seed));
	
	return retval;
}

/*
 * RestoreDetouredFunction, Restore a detoured function (Remove detour)
 * * pDupFcn, Pointer to the duplicated function
 * * pOrigFcn, Pointer to the original function
 */

void RestoreDetouredFunction(char *pOrigFcn, char *pDupFcn) {
	int offset;
	
	_asm
	{
		CLI					// disable interrupt
		MOV EAX, CR0		// move CR0 register into EAX
		AND EAX, NOT 10000H // disable WP bit 
		MOV CR0, EAX		// write register back 
	}

	// Uninstall the detour, Restore `JMP_LENGHT` bytes from the duplicate function.
	
	for (offset = 0; offset < JMP_LENGTH; offset++) {
		pOrigFcn[offset] = pDupFcn[offset];
	}
	
	_asm 
	{
		MOV EAX, CR0		// move CR0 register into EAX
		OR EAX, 10000H		// enable WP bit 
		MOV CR0, EAX		// write register back 
		STI					// enable interrupt
	}
	
	// Deallocate the duplicate page
	
	ExFreePoolWithTag((void *)((int)pDupFcn - OFFSET_IN_PAGE((int)pOrigFcn)), 0xdeadbeef);
	
	return ;
}

/*
 * DriverUnload, Driver unload point
 * * DriverObject, self (Driver)
 */

void DriverUnload(IN PDRIVER_OBJECT DriverObject) {
	unsigned long seed;

	if (pDupRtlRandom != NULL) {

		KdPrint(("Removeing detour from RtlRandom\n"));

		// Remove detour

		RestoreDetouredFunction((char *)pCurRtlRandom, (char *)pDupRtlRandom);	

		// Do another self-test

		seed = 31337;

		pCurRtlRandom(&seed);
		
		// No bugcheck? ;-)
	}
	
	KdPrint(("Krembo unloaded!\n"));
	
	return ;
}

/*
 * DriverEntry, Driver Single Entry Point
 * * DriverObject, self (Driver)
 * * RegistryPath, given RegistryPath
 */

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath) {
	UNICODE_STRING pFcnName;
	unsigned long seed;

	KdPrint(("Krembo loaded!\n"));
	
	// Register unload routine
	
	DriverObject->DriverUnload = DriverUnload;
	
	// Lookup RtlRandom address
	
	RtlInitUnicodeString(&pFcnName, L"RtlRandom");
	
	pCurRtlRandom = MmGetSystemRoutineAddress(&pFcnName);
	
	KdPrint(("Found RtlRandom @ 0x%08x\n", pCurRtlRandom));
	
	// Detour RtlRandom

	pDupRtlRandom = DetourFunction((char *)pCurRtlRandom, (int)MyRtlRandom, (int)NonPagedPool);

	// Detour status?

	if (pDupRtlRandom == NULL) {
		
		// Unable to detour

		KdPrint(("Unable to detour RtlRandom! (Not enough resources?)\n"));
	
	} else {
	
		KdPrint(("Duplicate Function @ 0x%08x\n", pDupRtlRandom));

		// Detour installed, do a self-test
	
		KdPrint(("Detour installed, going into a self-test ...\n"));
		
		// Dummy seed for the self-test
		
		seed = 31337;

		pCurRtlRandom(&seed);
		
	}
	
	return STATUS_SUCCESS;
}
