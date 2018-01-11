/***********************************头文件**************************************/
#include "../Include/RTWindowsScheduler.h"
#include "../Include/Queue.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//												队    列
//
/////////////////////////////////////////////////////////////////////////////////////////////////
/***********************************系统变量定义***********************************************/
//共享内存偏移地址
#pragma LOCKEDDATA
ULONG	s_AddressOffset;

/***********************************函数定义***************************************************/
#pragma LOCKEDCODE
static void ListAdd(LIST_HEAD *New,LIST_HEAD *Prev,LIST_HEAD *Next)
{
	Next->Prev = PTR_OFFSET(LIST_HEAD,New);
	New->Next =  PTR_OFFSET(LIST_HEAD,Next);
	New->Prev =  PTR_OFFSET(LIST_HEAD,Prev);
	Prev->Next = PTR_OFFSET(LIST_HEAD,New);
}

#pragma LOCKEDCODE
void ListAddHead(LIST_HEAD *New, LIST_HEAD *Head)
{
	ListAdd(New, PTR(LIST_HEAD,Head->Next), Head);
}

#pragma LOCKEDCODE
void ListAddTail(LIST_HEAD *New, LIST_HEAD *Head)
{
	ListAdd(New, PTR(LIST_HEAD,Head->Prev), Head);
}

#pragma LOCKEDCODE
void ListInsert(LIST_HEAD *New, LIST_HEAD *Pos)
{
	PTR(LIST_HEAD,Pos->Prev)->Next = PTR_OFFSET(LIST_HEAD,New);
	New->Next = PTR_OFFSET(LIST_HEAD,Pos);
	New->Prev = Pos->Prev;
	Pos->Prev = PTR_OFFSET(LIST_HEAD,New);
}

#pragma LOCKEDCODE
static void __ListDel(LIST_HEAD * Prev, LIST_HEAD * Next)
{
	Next->Prev = PTR_OFFSET(LIST_HEAD,Prev);
	Prev->Next = PTR_OFFSET(LIST_HEAD,Next);
}

#pragma LOCKEDCODE
void ListDel(LIST_HEAD *entry)
{
	if (entry->Prev)
	{
		__ListDel(PTR(LIST_HEAD,entry->Prev), PTR(LIST_HEAD,entry->Next));
		entry->Prev = 0;
		entry->Next = 0;
	}
}

#pragma LOCKEDCODE
int ListEmpty(const LIST_HEAD *Head)
{
	return PTR(LIST_HEAD,Head->Next) == Head;
}

