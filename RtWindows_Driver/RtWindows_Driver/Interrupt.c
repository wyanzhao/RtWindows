/***********************************头文件**************************************/
#include <NTDDK.h>

#include "../Include/Interrupt.h"
#include "../Include/Scheduler.h"
#include "../Include/SysManager.h"

/***********************************系统变量定义***********************************************/
//ISR中所需函数地址
typedef void(*Pfunc)();
#pragma LOCKEDDATA
static Pfunc HalBeginSystemInterrupt = NULL;
#pragma LOCKEDDATA
static Pfunc HalEndSystemInterrupt = NULL;
#pragma LOCKEDDATA
static Pfunc PsChargeProcessCpuCycles = NULL;
#pragma LOCKEDDATA
static Pfunc Kei386EoiHelper = NULL;

//中断描述符表地址
#pragma LOCKEDDATA
static PP2C_IDTENTRY idt_addr;

//记录APIC timer原有状态
#pragma LOCKEDDATA
ULONG OldInt = 0;

//apic timer初始计数
#pragma LOCKEDDATA
unsigned long apic_init_count = 0;

//计数器周期
#pragma LOCKEDDATA
unsigned long apic_period = 0;

//记录有多少个进程设定过时钟，只有第一个可以设定，0时恢复原有时钟设置
#pragma LOCKEDDATA
unsigned long cInterruptCount = 0;

/***********************************函数定义***********************************************/
//返回中断描述符表IDT的基址
#pragma LOCKEDCODE
static PVOID GetIdt()
{
	P2C_IDTR idtr;
	_asm{
		sidt idtr
	}
	return (PVOID)idtr.base;
}

//设置APIC 中断
#pragma LOCKEDCODE
void SetInterrupt()
{
	PHYSICAL_ADDRESS  phys;
	PVOID pAddr;

	InitAddress();
	AddInterruptHandle();
	
	RtlZeroMemory(&phys, sizeof(PHYSICAL_ADDRESS));
	
	phys.u.LowPart = 0xFEE003E0;
	pAddr = MmMapIoSpace(phys, 0x4, MmNonCached);//设置为1时为sys bus频率
	*(unsigned char*)pAddr = 0x0b;

	phys.u.LowPart = 0xFEE00380;
	pAddr = MmMapIoSpace(phys, 0x4, MmNonCached);
	*(unsigned long*)pAddr = apic_init_count;//设置Local APIC定时器的的初始计数寄存器	apic_init_count
	KdPrint(("Current APIC Count is %ld\n", *(unsigned long*)pAddr));

	phys.u.LowPart = 0xFEE00320;
	pAddr = MmMapIoSpace(phys, 0x4, MmNonCached);
	OldInt = *(unsigned long *)pAddr & (0x000000ff);
	*(unsigned long *)pAddr = (*(unsigned long *)pAddr) & (0xffffff00);
	*(unsigned long *)pAddr = (*(unsigned long *)pAddr) | (0x000000e2);
	*(unsigned long *)pAddr = (*(unsigned long *)pAddr) & (0xfffeffff);
	MmUnmapIoSpace(pAddr, 0x4);
}

//卸载APIC 中断
#pragma LOCKEDCODE
BOOLEAN UnsetInterrupt()
{
	PHYSICAL_ADDRESS  phys;
	PVOID pAddr;
	KIRQL					oldIrql;

	KeRaiseIrql(HIGH_LEVEL, &oldIrql);

	RtlZeroMemory(&phys, sizeof(PHYSICAL_ADDRESS));

	if (!OldInt) return FALSE;

	phys.u.LowPart = 0xFEE00320;
	pAddr = MmMapIoSpace(phys, 0x4, MmNonCached);

	*(unsigned long *)pAddr = (*(unsigned long *)pAddr) | (0x10000);
	*(unsigned long *)pAddr = (*(unsigned long *)pAddr) | (0x200e2);
	*(unsigned long *)pAddr = (*(unsigned long *)pAddr) & (0xffffff00);
	*(unsigned long *)pAddr = (*(unsigned long *)pAddr) | (OldInt);

	MmUnmapIoSpace(pAddr, 0x4);
	_asm cli
	idt_addr[0xe2] = idt_addr[0xe4];
	_asm sti
	KeLowerIrql(oldIrql);

	return TRUE;
}

#pragma LOCKEDCODE
static void InitAddress()
{
	UNICODE_STRING FunName;

	RtlInitUnicodeString(&FunName, L"Kei386EoiHelper");
	Kei386EoiHelper = (Pfunc)MmGetSystemRoutineAddress(&FunName);

	RtlInitUnicodeString(&FunName, L"HalEndSystemInterrupt");
	HalEndSystemInterrupt = (Pfunc)MmGetSystemRoutineAddress(&FunName);

	RtlInitUnicodeString(&FunName, L"HalBeginSystemInterrupt");
	HalBeginSystemInterrupt = (Pfunc)MmGetSystemRoutineAddress(&FunName);

	RtlInitUnicodeString(&FunName, L"PsChargeProcessCpuCycles");
	PsChargeProcessCpuCycles = (Pfunc)MmGetSystemRoutineAddress(&FunName);

	KdPrint(("Kei386EoiHelper:0x%x\n", Kei386EoiHelper));
	KdPrint(("HalEndSystemInterrupt:0x%x\n", HalEndSystemInterrupt));
	KdPrint(("HalBeginSystemInterrupt:0x%x\n", HalBeginSystemInterrupt));
	KdPrint(("PsChargeProcessCpuCycles:0x%x\n", PsChargeProcessCpuCycles));
}

#pragma LOCKEDCODE
static void AddInterruptHandle()
{
	idt_addr = (PP2C_IDTENTRY)GetIdt();
	idt_addr[0xe2] = idt_addr[0xD1];
	idt_addr[0xe2].offset_low = P2C_LOW16_OF_32(Interrupt);
	idt_addr[0xe2].offset_high = P2C_HIGH16_OF_32(Interrupt);
}

#pragma LOCKEDCODE
static void ServiceRoutine()
{
	InitScheduler();
}

#pragma LOCKEDCODE
 __declspec (naked) void Interrupt()
{	
_asm
{
	jmp HalpBroadcastCallService

	Dr_HGeneric_a:
	test    dword ptr[ebp + 70h], 20000h
	 jne     Dr_HGeneric_a_0x13

	Dr_HGeneric_a_0x9 :
	test    byte ptr[ebp + 6Ch], 1
	je      HalpBroadcastCallService_0xcb

	Dr_HGeneric_a_0x13 :
	mov     ebx, dr0
	mov     ecx, dr1
	 mov     edi, dr2
	mov     dword ptr[ebp + 18h], ebx
	mov     dword ptr[ebp + 1Ch], ecx
	 mov     dword ptr[ebp + 20h], edi
	mov     ebx, dr3
	mov     ecx, dr6
	mov     edi, dr7
	mov     dword ptr[ebp + 24h], ebx
	 mov     dword ptr[ebp + 28h], ecx
	xor     ebx, ebx
	mov     dword ptr[ebp + 2Ch], edi
	mov     dr7, ebx
	mov     edi, dword ptr fs : [20h]
	mov     ebx, dword ptr[edi + 2F4h]
	 mov     ecx, dword ptr[edi + 2F8h]
	mov     dr0, ebx
	mov     dr1, ecx
	mov     ebx, dword ptr[edi + 2FCh]
	mov     ecx, dword ptr[edi + 300h]
	mov     dr2, ebx
	mov     dr3, ecx
	mov     ebx, dword ptr[edi + 304h]
	mov     ecx, dword ptr[edi + 308h]
	mov     dr6, ebx
	mov     dr7, ecx
	jmp     HalpBroadcastCallService_0xcb

	Abios_HGeneric_a:
	mov     eax, dword ptr[esp + 64h]
	 mov     ecx, 10h
	mov     edx, dword ptr fs : [20h]
	mov     edx, dword ptr[edx + 4]
	mov     edx, dword ptr[edx + 2Ch]
	shl     eax, 10h
	add     edx, esp
	mov     dword ptr[esp + 64h], eax
	mov     ss, cx
	mov     esp, edx
	mov     ebp, edx
	jmp     HalpBroadcastCallService_0x6a 
			   
	V86_HGeneric_a:
	mov     eax, dword ptr[ebp + 84h]
	mov     ebx, dword ptr[ebp + 88h]
	mov     ecx, dword ptr[ebp + 7Ch]
	mov     edx, dword ptr[ebp + 80h]
	mov     word ptr[ebp + 50h], ax
	mov     word ptr[ebp + 30h], bx
	mov     word ptr[ebp + 34h], cx
	mov     word ptr[ebp + 38h], dx
	jmp     HalpBroadcastCallService_0x2f

		HalpBroadcastCallService:
	 push    esp
	 push    ebp
	push    ebx
	push    esi
	 push    edi
	sub     esp, 54h
	mov     ebp, esp
	mov     dword ptr[ebp + 44h], eax
	mov     dword ptr[ebp + 40h], ecx
	mov     dword ptr[ebp + 3Ch], edx
	test    dword ptr[ebp + 70h], 20000h
	 jne     V86_HGeneric_a

	HalpBroadcastCallService_0x1c:
	  cmp     word ptr[ebp + 6Ch], 8
	           je      HalpBroadcastCallService_0x42

		HalpBroadcastCallService_0x23 :
	          mov     word ptr[ebp + 50h], fs
	      mov     word ptr[ebp + 38h], ds
	     mov     word ptr[ebp + 34h], es
	       mov     word ptr[ebp + 30h], gs

		HalpBroadcastCallService_0x2f:
   mov     ebx, 30h
   mov     eax, 23h
	mov     fs, bx
	mov     ds, ax
	mov     es, ax

		HalpBroadcastCallService_0x42:
	 mov  ebx, dword ptr fs : [0]
	 mov dword ptr fs : [0], 0FFFFFFFFh
	  mov     dword ptr[ebp + 4Ch], ebx
	cmp     esp, 10000h
	    jb      Abios_HGeneric_a

		HalpBroadcastCallService_0x63 :
		mov     dword ptr[ebp + 64h], 0

		HalpBroadcastCallService_0x6a :
		 mov     ecx, dword ptr fs : [124h]
		 cld
		 mov     edi, dword ptr fs : [20h]
		     inc     byte ptr[edi + 11h]
		       cmp     byte ptr[edi + 11h], 1
		          jne     HalpBroadcastCallService_0xbd

		HalpBroadcastCallService_0x82 :
		          rdtsc
		  sub     eax, dword ptr[edi + 31F8h]
		   sbb     edx, dword ptr[edi + 31FCh]
		      mov     esi, dword ptr[ecx + 10h]
		         add     esi, eax
		        adc     dword ptr[ecx + 18h], edx
		       add     dword ptr[ecx + 10h], eax
		    adc     dword ptr[ecx + 14h], edx
		  add     dword ptr[edi + 31F8h], eax
		    adc     dword ptr[edi + 31FCh], edx
		    test    byte ptr[ecx + 2], 1
		        je      HalpBroadcastCallService_0xbd 

		HalpBroadcastCallService_0xb0:
	       mov     esi, ecx
	          push    edx
	         push    eax
	         push    edi
	  call    dword ptr[PsChargeProcessCpuCycles]
	     mov     ecx, esi

		HalpBroadcastCallService_0xbd:
	     and     dword ptr[ebp + 2Ch], 0
	    test    byte ptr[ecx + 3], 0DFh
	  jne     Dr_HGeneric_a

		HalpBroadcastCallService_0xcb :
		       mov     ebx, dword ptr[ebp + 60h]
		        mov     edi, dword ptr[ebp + 68h]
		        mov     dword ptr[ebp + 0Ch], edx
		 mov     dword ptr[ebp + 8], 0BADB0D00h
		        mov     dword ptr[ebp], ebx
		        mov     dword ptr[ebp + 4], edi
		      push    0E2h
		      sub     esp, 4
		     push    esp
	    push    0E2h
		        push    1Ch
		   call    HalBeginSystemInterrupt
		 or al, al
		 jne     HalpBroadcastCallService_0x146

		HalpBroadcastCallService_0xfa:
        add     esp, 8
	  mov     ecx, dword ptr fs : [1Ch]
		   dec     byte ptr[ecx + 131h]
		           jne     HalpBroadcastCallService_0x140

		HalpBroadcastCallService_0x10c :
	      rdtsc
		    sub     eax, dword ptr[ecx + 3318h]
		 sbb     edx, dword ptr[ecx + 331Ch]
		   mov     edi, dword ptr[ecx + 3320h]
		        add     edi, eax
		    adc     dword ptr[ecx + 3328h], edx
		  add     dword ptr[ecx + 3320h], eax
		   adc     dword ptr[ecx + 3324h], edx
	   add     dword ptr[ecx + 3318h], eax
		   adc     dword ptr[ecx + 331Ch], edx

		HalpBroadcastCallService_0x140:
	   jmp     dword ptr[Kei386EoiHelper]

		HalpBroadcastCallService_0x146 :
		     call    InitScheduler
		        cli
		 mov     ecx, dword ptr fs : [1Ch]
		dec     byte ptr[ecx + 131h]
		      jne     HalpBroadcastCallService_0x18f

		HalpBroadcastCallService_0x15b :
		        rdtsc
		   sub     eax, dword ptr[ecx + 3318h]
		   sbb     edx, dword ptr[ecx + 331Ch]
		  mov     edi, dword ptr[ecx + 3320h]
		    add     edi, eax
		  adc     dword ptr[ecx + 3328h], edx
		  add     dword ptr[ecx + 3320h], eax
	 adc     dword ptr[ecx + 3324h], edx
		add     dword ptr[ecx + 3318h], eax
	 adc     dword ptr[ecx + 331Ch], edx

		HalpBroadcastCallService_0x18f:
	    call    HalEndSystemInterrupt
		jmp     dword ptr[Kei386EoiHelper] 
}
}
