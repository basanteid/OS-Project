#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include <kern/conc/sleeplock.h>
#include <kern/proc/user_environment.h>
#include <kern/mem/memory_manager.h>
#include "../conc/kspinlock.h"
#include "../conc/ksemaphore.h"
#include <kern/cpu/cpu.h>

// User added
LIST_HEAD(FreeAreaList, FreeArea);

static struct FreeArea area_pool[MAX_FREE_AREAS];
static struct FreeAreaList free_areas, unused_nodes;
static uint16 size_in_pages[NUM_OF_KHEAP_PAGES];

/* 	mochi while testing:
 * 		since no need for extra memory usage to know if the virtual address
 * 		goes to a buddy page or not - we can use given functions to know from
 * 		Page Table Directly
 * 	fixes:
 * 		- removes is_buddy_page array
 * 		- added is_buddy_va macro
 */

static inline int is_buddy_va(uint32 va)
{
	uint32 *pt = NULL;
	get_page_table(ptr_page_directory, va, &pt);
	if (pt == NULL)
		return 0;
	return (pt[PTX(va)] & (PERM_AVAILABLE & ~(PERM_BUFFERED | PERM_UHPAGE))) != 0;
}

static struct kspinlock kheap_lock; // lock to protect critical sections here

/************************************************************************************/
//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
// [PROJECT'26.GM#2] KERNEL HEAP - [0] GIVEN FUNCTIONS
//==============================================
// [1] INITIALIZE KERNEL HEAP:
//==============================================
void kheap_init()
{
	//==================================================================================
	// DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		buddy_init();
		set_kheap_strategy(KHP_PLACE_CUSTOMFIT);
		kheapPageAllocStart = KERNEL_HEAP_START;
		kheapPageAllocBreak = KERNEL_HEAP_START;
	}
	//==================================================================================
	//==================================================================================

	LIST_INIT(&free_areas);
	LIST_INIT(&unused_nodes);
	for (int i = 0; i < MAX_FREE_AREAS; i++)
		LIST_INSERT_HEAD(&unused_nodes, &node_pool[i]);
}
/************************************************************************************/

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
/**************************************************/
/* BLOCK ALLOCATOR */
/**************************************************/
void *create_buddy_space()
{
	// [PROJECT'26.GM#2] KERNEL HEAP - [4] create_buddy_space
	// Your code is here

	// hagib page wahda (4KB) men el page allocator
	// PAGE_SIZE akbar men 2048 f kmalloc hatroh lel page allocator msh el buddy,fa mfesh loop
	/* 	mochi while testing:
	 *  	- hastaa5dem page_alloc_internal msh kmalloc 3lashan el recursive lock
	 *   	- hatit lock
	 */
	acquire_kspinlock(&kheap_lock);
	void *virtual_add_Marioma = page_alloc_internal(PAGE_SIZE);
	release_kspinlock(&kheap_lock);
	if (virtual_add_Marioma == NULL)
		return NULL; // el heap kheles

	// hageeb el page table beta3 el page di ashan aamel aleha el mark
	uint32 *Basbosa_ptr_page_table = NULL;
	get_page_table(ptr_page_directory, (uint32)virtual_add_Marioma, &Basbosa_ptr_page_table);

	// a3mel mark en di buddy page
	// bst3mel |= 3ashan a5ali el present w el writeable zay ma homa
	Basbosa_ptr_page_table[PTX((uint32)virtual_add_Marioma)] |= (PERM_AVAILABLE & ~(PERM_BUFFERED | PERM_UHPAGE));

	return virtual_add_Marioma;

	// Comment the following line
	// panic("create_buddy_space() not implemented");
}

void delete_buddy_space(void *virtual_add_basbosa)
{
	// [PROJECT'26.GM#2] KERNEL HEAP - [5] delete_buddy_space
	// Your code is here

	// hageeb el page table el awel 3ashan ashil el mark
	uint32 *Mariam_ptr_page_table = NULL;
	get_page_table(ptr_page_directory, (uint32)virtual_add_basbosa, &Mariam_ptr_page_table);

	// law fi table ashil el mark, law mfesh yb2a el address ghalat
	if (Mariam_ptr_page_table != NULL)
	{
		// lazem ashil el mark abl kfree
		// law 3aksht el tartib, kfree hatshoof el mark w tfakar el page buddy w tendh el free_block tani = infinite loop
		Mariam_ptr_page_table[PTX((uint32)virtual_add_basbosa)] &= ~(PERM_AVAILABLE & ~(PERM_BUFFERED | PERM_UHPAGE));
	}

	// dlwa2ty el page b2a page allocator adeya
	// kfree heya el mas2ola tpanic law el address ghalat
	kfree(virtual_add_basbosa);

	// Comment the following line
	// panic("delete_buddy_space() not implemented");
}

/**************************************************/
/* PAGE ALLOCATOR */
/**************************************************/
void *kmalloc(unsigned int size)
{
	// [PROJECT'26.GM#2] KERNEL HEAP - [6] kmalloc
	// Your code is here

	if (size == 0)
		return NULL;

	/* mochi while testing:
	 * fixed: if size <= DYN_ALLOC_MAX_BLOCK_SIZE, use block allocator, else use page allocator
	 * it was making a recursive lock:
	 * 		kmalloc(samll) -> alloc_block(size) -> create_buddy_space() -> kmalloc(PAGE_SIZE)
	 * fixe:
	 * 		call alloc_block(size) out side of the critical section of kmalloc()
	 */

	if (size <= DYN_ALLOC_MAX_BLOCK_SIZE) // Block Allocator
		return alloc_block(size);

	acquire_kspinlock(&kheap_lock);
	// Page Allocator
	void *ptr = page_alloc_internal(size);
	release_kspinlock(&kheap_lock);
	return ptr;

	// Comment the following line
	//  kpanic_into_prompt("kmalloc() is not implemented yet...!!");

	/* EFFICIENT IMPLEMENTATION ~O(#FREE BLOCKS + #ALLOCATED PAGES) IS REQUIRED */
}

void kfree(void *virtual_address)
{
	// [PROJECT'26.GM#2] KERNEL HEAP - [7] kfree
	// Your code is here

	uint32 va = (uint32)virtual_address;

	/* mochi while testing:
	 * avoid recursive lock with the same way in kalloc()
	 */

	if (va < KERNEL_HEAP_START || va >= kheapPageAllocBreak)
		panic("kfree: invalid address %x", va);

	if (is_buddy_va(va))
	{
		free_block(virtual_address); // may call delete_buddy_space -> kfree(page)
		return;
	}

	acquire_kspinlock(&kheap_lock);
	int ret = page_free_internal(va);
	release_kspinlock(&kheap_lock);

	if (ret != 0)
		panic("kfree: invalid address %x", va);

	// Comment the following line
	//  panic("kfree() not implemented");

	/* EFFICIENT IMPLEMENTATION ~O(#FREE BLOCKS + #FREED PAGES) IS REQUIRED */
}

unsigned int kheap_physical_address(unsigned int Mariam_virtual_address)
{
	// [PROJECT'26.GM#2] KERNEL HEAP - [8] kheap_physical_address
	// Your code is here

	// law el address bara el heap yb2a mfesh mapping leh, nraga3 0
	if (Mariam_virtual_address < KERNEL_HEAP_START || Mariam_virtual_address >= KERNEL_HEAP_MAX)
		return 0;

	// hageeb el page table beta3 el address da men el kernel directory
	uint32 *Marioma_ptr_page_table = NULL;
	get_page_table(ptr_page_directory, Mariam_virtual_address, &Marioma_ptr_page_table);
	if (Marioma_ptr_page_table == NULL)
		return 0; // mfesh table yb2a mfesh mapping

	// a2ra el entry beta3 el page w ashoof hia mapped wala la
	uint32 Basbosa_page_table_entry = Marioma_ptr_page_table[PTX(Mariam_virtual_address)];
	if ((Basbosa_page_table_entry & PERM_PRESENT) == 0)
		return 0; // el page msh present f el ram

	// 3enwan el frame + el offset gowa el page
	return EXTRACT_ADDRESS(Basbosa_page_table_entry) + PGOFF(Mariam_virtual_address);

	// Comment the following line
	// panic("kheap_physical_address() is not implemented yet...!!");

	/*EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED */
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	// [PROJECT'26.GM#2] KERNEL HEAP - [9] kheap_virtual_address
	// Your code is here

	// lazem nshoof rakam el frame abl to_frame_info 3ashan hia btpanic law el rakam akbar mn el number_of_frames
	if (PPN(physical_address) >= number_of_frames)
		return 0;

	// hageeb el frame beta3 el physical address da
	struct FrameInfo *ptr_frame_info = to_frame_info(physical_address);

	// law el proc fady yb2a mfesh VA mt2ayed leha, y3ni msh mapped f el kernel heap
	/*	mochi while testing:
	 *		virtual address of frame usually stored in kheap_va not in process

	 */
	if (ptr_frame_info->kheap_va == 0)
		return 0;

	// el process shayel VA awel el page (kmalloc hia eli btktbo), bzawed 3aleh el offset
	return (uint32)ptr_frame_info->kheap_va + PGOFF(physical_address);

	// Comment the following line
	// panic("kheap_virtual_address() is not implemented yet...!!");

	/*EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED */
}

//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().
void *krealloc(void *virtual_address, uint32 new_size)
{
	// [PROJECT'26.BONUS#1] KERNEL REALLOC - krealloc
	// Your code is here
	if (virtual_address == NULL)
		return kmalloc(new_size);
	if (new_size == 0)
	{
		kfree(virtual_address);
		return NULL;
	}

	int invalid;

	acquire_kspinlock(&kheap_lock);
	void *ret = krealloc_internal((uint32)virtual_address, new_size, &invalid);
	release_kspinlock(&kheap_lock);

	if (invalid)
		panic("krealloc: invalid address %x", (uint32)virtual_address);

	return ret;

	// Comment the following line
	//  panic("krealloc() is not implemented yet...!!");
}

/* ------------------------------------------------------------------------- *
 *                             Helper Functions                              *
 * ------------------------------------------------------------------------- */
static void *krealloc_internal(uint32 va, uint32 new_size, int *invalid)
{
	*invalid = 0;
	if (va < kheapPageAllocStart || va >= kheapPageAllocBreak)
	{
		*invalid = 1;
		return NULL;
	}

	// old was a buddy space
	uint32 idx = KH_IDX(va);
	if (is_buddy_va(va))
	{
		uint32 blk = get_block_size((void *)va);
		uint32 usable_size = blk - sizeof(struct BlockMetaData);
		// option 1: fits in the same buddy and same level
		if (new_size <= DYN_ALLOC_MAX_BLOCK_SIZE)
		{
			uint32 need = new_size + sizeof(struct BlockMetaData);
			if (need <= blk && need > blk / 2)
				return (void *)va;
		}

		// option 2: kalloc 'buddy or page'
		void *nva;
		if (new_size <= DYN_ALLOC_MAX_BLOCK_SIZE) // Block Allocator
			nva = alloc_block(new_size);
		else // Page Allocator
			nva = page_alloc_internal(new_size);

		if (nva == NULL)
			return NULL;
		memcpy(
			nva,
			(void *)va,
			usable_size < new_size ? usable_size : new_size);
		free_block((void *)va);
		return nva;
	}

	// old was a page
	if (va % PAGE_SIZE != 0 || size_in_pages[idx] == 0)
	{
		*invalid = 1;
		return NULL;
	}

	uint32 old_pages = size_in_pages[idx];
	// option 1: from page to block
	if (new_size <= DYN_ALLOC_MAX_BLOCK_SIZE)
	{
		void *nva = alloc_block(new_size);
		if (nva == NULL)
			return NULL;
		memcpy(nva, (void *)va, new_size);
		page_free_internal(va);
		return nva;
	}

	// option 2: from page to page
	if (new_size > KERNEL_HEAP_MAX - KERNEL_HEAP_START)
		return NULL;

	uint32 new_pages = ROUNDUP(new_size, PAGE_SIZE) / PAGE_SIZE;
	// op 2.1: same number of pages required
	if (new_pages == old_pages)
		return (void *)va;

	// op 2.2: smaller number of pages required
	if (new_pages < old_pages)
	{
		uint32 tail = va + new_pages * PAGE_SIZE;
		undo_mappings(tail, old_pages - new_pages);
		size_in_pages[idx] = new_pages;
		add_free_area(tail, old_pages - new_pages);
		return (void *)va;
	}

	// op 2.3: bigger number of pages required
	uint32 end = va + old_pages * PAGE_SIZE;
	uint32 extra = new_pages - old_pages;
	struct FreeArea *x = NULL, *last = NULL;
	LIST_FOREACH(x, &free_areas, FreeArea)
	{
		if (x->start_va == end)
		{
			last = x;
			break;
		}
		if (x->start_va > end)
			break;
	}

	// op 2.3.1: there is enough space to grow in kheap
	if (last != NULL && last->num_pages >= extra)
	{
		if (map_heap_pages(end, extra) != 0)
			return NULL;
		if (last->num_pages == extra)
		{
			LIST_REMOVE(&free_areas, last);
			LIST_INSERT_HEAD(&unused_nodes, last);
		}
		else
		{
			last->start_va += extra * PAGE_SIZE;
			last->num_pages -= extra;
		}
		size_in_pages[idx] = new_pages;
		return (void *)va;
	}

	// op 2.3.2: need to move kheap_break_ptr
	if (end == kheapPageAllocBreak && extra <= (KERNEL_HEAP_MAX - kheapPageAllocBreak) / PAGE_SIZE)
	{
		if (map_heap_pages(end, extra) != 0)
			return NULL;
		kheapPageAllocBreak += extra * PAGE_SIZE;
		size_in_pages[idx] = new_pages;
		return (void *)va;
	}

	// op 2.3.3: need to move it to another location
	void *nva = page_alloc_internal(new_size);
	if (nva == NULL)
		return NULL;
	memcmp(nva, (void *)va, old_pages * PAGE_SIZE);
	page_free_internal(va);
	return nva;
}

static int map_heap_pages(uint32 va, uint32 n)
{
	for (uint32 i = 0; i < n; i++)
	{
		struct FrameInfo *fi = NULL;
		if (allocate_frame(&fi) != 0)
		{
			undo_mappings(va, i);
			return -1;
		}

		if (map_frame(ptr_page_directory, fi, va + i * PAGE_SIZE, PERM_WRITEABLE | PERM_PRESENT) != 0)
		{
			free_frame(fi);
			undo_mappings(va, i);
			return -1;
		}

		fi->kheap_va = va + i * PAGE_SIZE;
	}
	return 0;
}

static void undo_mappings(uint32 va, uint32 count)
{
	for (uint32 i = 0; i < count; i++)
	{
		uint32 *ptr_page_table = NULL;
		uint32 page_va = va + i * PAGE_SIZE;
		struct FrameInfo *fi = get_frame_info(ptr_page_directory, page_va, &ptr_page_table);
		if (fi != NULL)
			fi->kheap_va = 0;
		unmap_frame(ptr_page_directory, page_va);
	}
}

static void add_free_area(uint32 va, uint32 n)
{
	// make node
	struct FreeArea *x = LIST_FIRST(&unused_nodes);
	if (x == NULL)
		panic("kheap: out of free-area nodes");
	LIST_REMOVE(&unused_nodes, x);
	x->start_va = va;
	x->num_pages = n;

	struct FreeArea *pos = NULL;
	LIST_FOREACH(pos, &free_areas, FreeArea)
	{
		if (pos->start_va > va)
			break;
	}
	if (pos != NULL)
		LIST_INSERT_BEFORE(&free_areas, pos, x);
	else
		LIST_INSERT_TAIL(&free_areas, x);

	// merge next area
	struct FreeArea *nx = LIST_NEXT(x);
	if (nx != NULL && x->start_va + x->num_pages * PAGE_SIZE == nx->start_va)
	{
		x->num_pages += nx->num_pages;
		LIST_REMOVE(&free_areas, nx);
		LIST_INSERT_HEAD(&unused_nodes, nx);
	}

	// merge previous area
	struct FreeArea *pv = LIST_PREV(x);
	if (pv != NULL && pv->start_va + pv->num_pages * PAGE_SIZE == x->start_va)
	{
		pv->num_pages += x->num_pages;
		LIST_REMOVE(&free_areas, x);
		LIST_INSERT_HEAD(&unused_nodes, x);
		x = pv;
	}

	// kheap_break reset
	if (x->start_va + x->num_pages * PAGE_SIZE == kheapPageAllocBreak)
	{
		kheapPageAllocBreak = x->start_va;
		LIST_REMOVE(&free_areas, x);
		LIST_INSERT_HEAD(&unused_nodes, x);
	}
}

static void *page_alloc_internal(uint32 size)
{
	if (size == 0 || size > KERNEL_HEAP_MAX - KERNEL_HEAP_START)
		return NULL;

	// custom fit
	uint32 required_pages = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
	struct FreeArea *node = NULL, *exact = NULL, *worst = NULL;
	LIST_FOREACH(node, &free_areas, FreeArea)
	{
		if (node->num_pages == required_pages)
		{
			exact = node;
			break;
		}
		if (node->num_pages > required_pages && (worst == NULL || node->num_pages > worst->num_pages))
		{
			worst = node;
		}
	}

	uint32 va;
	if (exact != NULL)
		va = exact->start_va;
	else if (worst != NULL)
		va = worst->start_va;
	else
	{
		if (required_pages > (KERNEL_HEAP_MAX - kheapPageAllocBreak) / PAGE_SIZE)
			return NULL;
		va = kheapPageAllocBreak;
	}

	// mapping
	if (map_heap_pages(va, required_pages) != 0)
		return NULL;

	// commit
	if (exact != NULL)
	{
		LIST_REMOVE(&free_areas, exact);
		LIST_INSERT_HEAD(&unused_nodes, exact);
	}
	else if (worst != NULL)
	{
		worst->start_va += required_pages * PAGE_SIZE;
		worst->num_pages -= required_pages;
	}
	else
	{
		kheapPageAllocBreak += required_pages * PAGE_SIZE;
	}

	size_in_pages[KH_IDX(va)] = required_pages;
	return (void *)va;
}

static int page_free_internal(uint32 va)
{
	if (va % PAGE_SIZE != 0)
		return -1;
	uint32 n = size_in_pages[KH_IDX(va)];
	if (n == 0)
		return -1;

	undo_mappings(va, n);
	/* 	mochi while testing
	 * 		changes line 550 from size_in_pages[KH_IDX(va)]; to size_in_pages[KH_IDX(va)] = 0;
	 */
	size_in_pages[KH_IDX(va)] = 0;
	add_free_area(va, n);

	return 0;
}