#pragma once

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//											��    ��
//
/////////////////////////////////////////////////////////////////////////////////////////////////

/***********************************�궨��**************************************/
//�����ҳ���Ƿ�ҳ�ڴ�
#ifdef _RTKERNEL
#ifndef PAGE_OPTION
#define PAGE_OPTION
#define PAGEDCODE code_seg("PAGE")
#define LOCKEDCODE code_seg()
#define INITCODE code_seg("INIT")

#define PAGEDDATA data_seg("PAGE")
#define LOCKEDDATA data_seg()
#define INITDATA data_seg("INIT")
#endif // !PAGE_OPTION
#else
#ifndef PAGE_OPTION
#define PAGE_OPTION
#define PAGEDCODE 
#define LOCKEDCODE 
#define INITCODE 

#define PAGEDDATA 
#define LOCKEDDATA 
#define INITDATA
#endif // !PAGE_OPTION
#endif

#define INIT_LIST_HEAD(ptr) do { \
		(ptr)->Next = PTR_OFFSET(LIST_HEAD,ptr); (ptr)->Prev = PTR_OFFSET(LIST_HEAD,ptr); \
	} while (0)

#define EMPTY_LIST_HEAD(ptr) do { \
		(ptr)->Next = 0; (ptr)->Prev = 0; \
	} while (0)

#define offset_of(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) (type *)(PTR(char,ptr) - offset_of(type, member))

#define list_entry(ptr, type, member)   container_of(ptr, type, member)

#define ListForEachEntry(type,pos, head, member)				\
	for (pos = list_entry((head)->Next, type, member);			\
	     pos->member.Next, &pos->member != (head); 				\
	     pos = list_entry(pos->member.Next, type, member))

#define ListForEachEntrySafe(type,pos, n, head, member)			\
	for (pos = list_entry((head)->Next, type, member),			\
		 n = list_entry(pos->member.Next, type, member);		\
	     &pos->member != (head); 								\
	     pos = n, n = list_entry(n->member.Next, type, member))

/***********************************ϵͳ���ݽṹ��������**************************************/
	typedef struct _ListHead 
	{
		volatile struct _ListHead *Next;
		volatile struct _ListHead *Prev;
	} LIST_HEAD;

/***********************************��������***************************************************/
#pragma LOCKEDCODE
void ListAddHead(LIST_HEAD *New, LIST_HEAD *Head);

#pragma LOCKEDCODE
void ListInsert(LIST_HEAD *New, LIST_HEAD *Pos);

#pragma LOCKEDCODE
void ListAddTail(LIST_HEAD *New, LIST_HEAD *Head);
	
#pragma LOCKEDCODE
void ListDel(LIST_HEAD *entry);

#pragma LOCKEDCODE
int  ListEmpty(const LIST_HEAD *Head);

