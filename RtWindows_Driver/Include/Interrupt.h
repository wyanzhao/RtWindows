#pragma once
/************************************************************************
* �ļ�˵��������APIC�������жϴ�����
* ���ߣ����Ʒ�
* ���ڣ�2011��11��15��
* �޸ģ����
* ʱ�䣺2013��6��3��
* �޸ģ�������
* ʱ�䣺2016��4��1��
*************************************************************************/

/***********************************ͷ�ļ�**************************************/
#include <NTDDK.h>
#include "Driver.h"

/***********************************�궨��**************************************/
//��ȡ��ַ�е�16λ����
#define P2C_LOW16_OF_32(data) \
	((P2C_U16)(((P2C_U32)data) & 0xffff))
//��ȡ��ַ�и�16λ����
#define P2C_HIGH16_OF_32(data) \
	((P2C_U16)(((P2C_U32)data) >> 16))

/***********************************ϵͳ���ݽṹ��������**************************************/
//���������ݽṹ
typedef UCHAR		 P2C_U8;
typedef USHORT		 P2C_U16;
typedef ULONG		 P2C_U32;

#pragma once
//IDT���ݽṹ
#pragma pack(push,1)
typedef struct P2C_IDTR_
{
	USHORT limit;
	ULONG base;
}P2C_IDTR, *PP2C_IDTR;
#pragma pack(pop)

//IDT�������ṹ
#pragma pack(push,1)
typedef struct P2C_IDT_ENTRY_ {
	P2C_U16 offset_low;
	P2C_U16 selector;
	P2C_U8 reserved;
	P2C_U8 type : 4;
	P2C_U8 always0 : 1;
	P2C_U8 dpl : 2;
	P2C_U8 present : 1;
	P2C_U16 offset_high;
} P2C_IDTENTRY, *PP2C_IDTENTRY;
#pragma pack(pop)

/***********************************ϵͳ��������***********************************************/
//apic timer��ʼ����
extern unsigned long apic_init_count;

//ͨ���û�̬���õ�APIC ����
extern unsigned long apic_period;

//��¼APIC timerԭ��״̬
extern unsigned long OldInt;

//����APIC�жϼ���
#pragma LOCKEDDATA
extern unsigned long cInterruptCount;

/***********************************��������***************************************************/
/*
* �����ж���������IDT�Ļ�ַ
* PVOID ����ֵ IDT��ַ
*/
static PVOID GetIdt();

/*
* ���ʱ���жϴ�������
*/
static void AddInterruptHandle();

/*
* �жϴ�������
*/
 static void Interrupt();

 /*
 * ����APICʱ���ж�
 */
void SetInterrupt();

/*
* ж��APIC�ж�
*/
BOOLEAN UnsetInterrupt();

/*
* ��ʼ��������ַ
*/
static void InitAddress();

/*
* �жϷ�������
*/
static void ServiceRoutine();