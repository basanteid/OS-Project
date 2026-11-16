/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"

/************************************************************************************/
//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
// [PROJECT'26.GM#1] DYNAMIC ALLOCATOR - [0] GIVEN FUNCTIONS
//==================================
// [1] INITIALIZE BUDDY FREE LISTS:
//==================================
void buddy_init()
{
	// panic("buddy_init is not implemented yet");

	for (int l = BUDDY_LOWER_LEVEL; l <= BUDDY_UPPER_LEVEL; ++l)
	{
		LIST_INIT(&(freeBlockLists[l]));
	}
}
//====================================
// [2] PRINT SIZE OF FREE LISTS:
//====================================
void PrintFreeBlockLists()
{
	for (int i = BUDDY_LOWER_LEVEL; i <= BUDDY_UPPER_LEVEL; ++i)
	{
		cprintf("Level#%d: %d\n", i, LIST_SIZE(&freeBlockLists[i]));
	}
}
/************************************************************************************/

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
//===========================
// [1] GET Elblockaya SIZE:
//===========================
__inline__ uint32 get_block_size(void *va)
{
	// [PROJECT'26.GM#1] DYNAMIC ALLOCATOR - [1] get_block_size
	// Your code is here

	if (va == NULL)
		return 0;

	struct BlockMetaData *meta = (struct BlockMetaData *)((uint8 *)va - META_DATA_SIZE);
	return BLOCK_SIZE(meta->level);

	// Comment the following line
	// panic("get_block_size() not implemented");
}
//===========================
// [2] ALLOCATE Elblockaya:
//===========================
void *alloc_block(uint32 requiredSize)
{
	// [PROJECT'26.GM#1] DYNAMIC ALLOCATOR - [2] alloc_block
	// Your code is here

	if (requiredSize == 0)
		return NULL;

	uint32 Size_Elblockaya = requiredSize + META_DATA_SIZE;

	int Level_Bta3y;
	for (Level_Bta3y = BUDDY_LOWER_LEVEL; Level_Bta3y <= BUDDY_UPPER_LEVEL; Level_Bta3y++)
	{
		if (BLOCK_SIZE(Level_Bta3y) >= Size_Elblockaya) // la2et level
			break;
	}

	if (Level_Bta3y > BUDDY_UPPER_LEVEL) // check 3ashan elfor loop momken twsal 12!!!
		return NULL;

	int Level_hns7ab_mno;
	for (Level_hns7ab_mno = Level_Bta3y; Level_hns7ab_mno <= BUDDY_UPPER_LEVEL; Level_hns7ab_mno++)
	{
		// hndwar 3la blockaya fe ellevel aw akbar
		if (LIST_SIZE(&freeBlockLists[Level_hns7ab_mno]) != 0)
			break;
	}

	// mafish blockaya fadya a5odha hn3ml page gdeda
	if (Level_hns7ab_mno > BUDDY_UPPER_LEVEL)
	{
		void *newPage = create_buddy_space();
		if (newPage == NULL) // mafish memory
			return NULL;
		// hn2sm elpage 2 half
		struct BlockMetaData *ElHalf_Elso8ayar = (struct BlockMetaData *)newPage;
		struct BlockMetaData *ElHalf_Elkbeer =
			(struct BlockMetaData *)((uint8 *)newPage + BLOCK_SIZE(BUDDY_UPPER_LEVEL));
		// matadata elements
		ElHalf_Elso8ayar->level = BUDDY_UPPER_LEVEL;
		ElHalf_Elso8ayar->is_free = 1;
		ElHalf_Elkbeer->level = BUDDY_UPPER_LEVEL;
		ElHalf_Elkbeer->is_free = 1;
		// hn7ot el 2 half
		LIST_INSERT_HEAD(&freeBlockLists[BUDDY_UPPER_LEVEL], ElHalf_Elkbeer);
		LIST_INSERT_HEAD(&freeBlockLists[BUDDY_UPPER_LEVEL], ElHalf_Elso8ayar);

		Level_hns7ab_mno = BUDDY_UPPER_LEVEL; // elly lsa md5alino hns7ab mno
	}

	struct BlockMetaData *Elblockaya_Bta3ty = LIST_FIRST(&freeBlockLists[Level_hns7ab_mno]); // bna5od elblockaya
	LIST_REMOVE(&freeBlockLists[Level_hns7ab_mno], Elblockaya_Bta3ty);						 // bnms7 elblockaya
	// law elblockaya kbera bn2smha
	for (int lvl = Level_hns7ab_mno - 1; lvl >= Level_Bta3y; lvl--)
	{
		struct BlockMetaData *upperHalf =
			(struct BlockMetaData *)((uint8 *)Elblockaya_Bta3ty + BLOCK_SIZE(lvl));
		upperHalf->level = lvl;
		upperHalf->is_free = 1;
		LIST_INSERT_HEAD(&freeBlockLists[lvl], upperHalf);
	}

	Elblockaya_Bta3ty->level = Level_Bta3y; // bn7or ellevel
	Elblockaya_Bta3ty->is_free = 0;			// bn5leha msh fadya

	return (void *)((uint8 *)Elblockaya_Bta3ty + META_DATA_SIZE);

	// Comment the following line
	// panic("alloc_block() not implemented");
}

//===================================================
// [3] FREE Elblockaya WITH COALESCING:
//===================================================
void free_block(void *va)
{
	// [PROJECT'26.GM#1] DYNAMIC ALLOCATOR - [3] free_block
	// Your code is here

	if (va == NULL)
		return;

	// bnshel elinfo bta3 elmatadata
	struct BlockMetaData *Elblockaya = (struct BlockMetaData *)((uint8 *)va - META_DATA_SIZE);
	Elblockaya->is_free = 1;
	LIST_INSERT_HEAD(&freeBlockLists[Elblockaya->level], Elblockaya);

	// bnshof eltwin bta3o 3shan ndmghom
	for (int lvl = Elblockaya->level; lvl <= BUDDY_UPPER_LEVEL; lvl++)
	{
		uint32 Addrres_Elblockaya = (uint32)Elblockaya;
		uint32 buddy_Addrresss = Addrres_Elblockaya ^ BLOCK_SIZE(lvl); // esta5dmna XOR
		struct BlockMetaData *My_Twin_Yaaa = (struct BlockMetaData *)buddy_Addrresss;

		if (My_Twin_Yaaa->is_free == 0 || My_Twin_Yaaa->level != lvl) // twin mahgoz
			return;
		// ndmghom
		LIST_REMOVE(&freeBlockLists[lvl], Elblockaya);
		LIST_REMOVE(&freeBlockLists[lvl], My_Twin_Yaaa);

		if (buddy_Addrresss < Addrres_Elblockaya)
			Elblockaya = My_Twin_Yaaa;
		// law kol elpage fadya bnrg3ha
		if (lvl == BUDDY_UPPER_LEVEL)
		{
			delete_buddy_space((void *)Elblockaya);
			return;
		}
		// ubgrade level
		Elblockaya->level = lvl + 1;
		Elblockaya->is_free = 1;
		LIST_INSERT_HEAD(&freeBlockLists[lvl + 1], Elblockaya);
	}

	// Comment the following line
	// panic("free_block() not implemented");
}

//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//

//===========================
// [1] REALLOCATE Elblockaya:
//===========================
void *realloc_block(void *va, uint32 newSize)
{
	// [PROJECT'26.BONUS#1] KERNEL REALLOC - realloc_block
	// Your code is here

	// law mafish addr aslan hnwadeh alloc Elblock
	if (va == NULL)
		return alloc_block(newSize);

	// law addr = 0 yb2a nfree elblock
	if (newSize == 0)
	{
		free_block(va);
		return NULL;
	}

	// law el size kbeer awy
	if (newSize > DYN_ALLOC_MAX_BLOCK_SIZE - META_DATA_SIZE)
		return NULL;

	// hagz elmakan elgded
	void *Eladdr_Elgdid = alloc_block(newSize);
	
	// law mafish memory
	if (Eladdr_Elgdid == NULL)
		return NULL;

	uint32 Elsize_Elold = get_block_size(va) - META_DATA_SIZE;
	uint32 nos5a_mn_ElnewSize = (Elsize_Elold < newSize) ? Elsize_Elold : newSize;
	memcpy(Eladdr_Elgdid, va, nos5a_mn_ElnewSize);

	free_block(va); // bnfree el2adem
	return Eladdr_Elgdid;

	// Comment the following line
	// panic("realloc_block() Not implemented yet");
}