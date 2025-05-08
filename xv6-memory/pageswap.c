#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"
#include "pageswap.h"

// #ifndef ALPHA
// #define ALPHA 25
// #endif

// #ifndef BETA
// #define BETA 10
// #endif

int Th = 100;          // Initial threshold
int Npg = 4;           // Initial pages to swap
const int LIMIT = 100; // Maximum pages to swap

struct swap_slot swap_slots[SWAP_SLOTS];

void
swap_init(void)
{
  int i;

  for (i = 0; i < SWAP_SLOTS; i++)
  {
    swap_slots[i].page_perm = 0;
    swap_slots[i].is_free = 1;
  }
}

int
get_free_swap_slot(void)
{
  int i;

  for (i = 0; i < SWAP_SLOTS; i++)
  {
    if (swap_slots[i].is_free)
    {
      swap_slots[i].is_free = 0;
      return i;
    }
  }

  return -1;
}

void
free_swap_slot(int slot_index)
{
  if (slot_index < 0 || slot_index >= SWAP_SLOTS)
  {
    return;
  }

  swap_slots[slot_index].page_perm = 0;
  swap_slots[slot_index].is_free = 1;
}

int
swap_out_page(char *page, int perm)
{
  int slot_index;
  int i;
  struct buf *bp;
  uint block_no;

  slot_index = get_free_swap_slot();
  if (slot_index < 0)
    return -1;

  
  swap_slots[slot_index].page_perm = perm;

  block_no = FSSIZE - SWAP_BLOCKS_TOTAL + slot_index*8;

  for (i = 0; i< 8; i++)
  {
    bp = bread(1, block_no + i);

    memmove(bp->data, page + i*512, 512);

    bwrite(bp);
    brelse(bp);
  }

  return slot_index;
}

int
swap_in_page(char *page, int slot_index)
{
  int i;
  struct buf *bp;
  uint block_no;

  if (slot_index < 0 || slot_index >= SWAP_SLOTS || swap_slots[slot_index].is_free)
  {
    return -1;
  }

  block_no = FSSIZE - SWAP_BLOCKS_TOTAL + slot_index*8;

  for (i = 0; i < 8; i++)
  {
    bp = bread(1, block_no + i);

    memmove(page + i*512, bp->data, 512);

    brelse(bp);
  }

  return 0;
}

int
get_swap_page_perm(int slot_index)
{
  int perm;

  if (slot_index < 0 || slot_index >= SWAP_SLOTS || swap_slots[slot_index].is_free)
  {
    return 0;
  }

  perm = swap_slots[slot_index].page_perm;

  return perm;
}

struct proc*
find_victim_process(void)
{
  return find_max_page_no_proc();
}

uint 
find_victim_page(struct proc *p)
{
  return find_max_page_no_page(p);
}

int
do_swap_out(struct proc *p, uint va)
{
  return proc__do_swap_out(p, va);
}

int
handle_page_fault(uint va)
{
  struct proc *p = myproc();
  pde_t *pgdir;
  pte_t *pte;
  uint slot_index;
  char *mem;
  uint sz;
  
  if(!p)
    return -1;
    
  va = PGROUNDDOWN(va);
  
  sz = proc_get_size(p);
  if(va >= sz)
    return -1;  
  
  pgdir = proc_get_pgdir(p);
  if(!pgdir)
    return -1;  
  
  pte = walkpgdir(pgdir, (void*)va, 0);
  if(!pte)
    return -1;  
    
  if(*pte & PTE_P)
    return -1;  
    
  // If high bits contain slot index + 1
  slot_index = ((*pte) >> 12) - 1;
  if(slot_index < 0 || slot_index >= SWAP_SLOTS || swap_slots[slot_index].is_free)
    return -1;  
    
  // Allocate a new physical page
  mem = kalloc();
  if(!mem) {
    if(swap_page_on_demand() < 0)
      return -1;  

    // Try allocation again
    mem = kalloc();
    if(!mem)
      return -1;  
  }
    
  // Read the page from swap
  if(swap_in_page(mem, slot_index) < 0) {
    kfree(mem);
    return -1;
  }
  
  // Get original permissions
  int perm = get_swap_page_perm(slot_index);
  
  // Map the page into the process's address space
  if(mappages(pgdir, (void*)va, PGSIZE, V2P(mem), perm) < 0) {
    kfree(mem);
    return -1;
  }
  
  // Free the swap slot
  free_swap_slot(slot_index);
  
  update_page_no(p, 1);
  
  return 0;
}

int 
swap_page_on_demand(void)
{
  return proc__swap_page_on_demand();
}

void
clean_process_swap(struct proc *p)
{
  pde_t *pgdir;
  pte_t *pte;
  uint i, slot_index;
  uint sz;
  
  if(!p)
    return;
    
  // Get process size and page directory safely
  sz = proc_get_size(p);
  pgdir = proc_get_pgdir(p);
  
  if(!pgdir || sz == 0)
    return;
  
  // Free any swap slots used by this process
  for(i = 0; i < sz; i += PGSIZE) {
    pte = walkpgdir(pgdir, (void*)i, 0);
    if(!pte || (*pte & PTE_P))
      continue;  
      
    // If high bits contain slot index + 1
    slot_index = ((*pte) >> 12) - 1;
    if(slot_index >= 0 && slot_index < SWAP_SLOTS && !swap_slots[slot_index].is_free) {
      free_swap_slot(slot_index);
    }
  }
}

void
check_memory_threshold(void)
{
  int free_pages = count_free_pages();
  if (free_pages <= Th)
  {
    cprintf("Current Threshold = %d, Swapping %d pages\n", Th, Npg);

    for (int i = 0; i < Npg; i++)
    {
      struct proc *victim = find_victim_process();

      if (!victim)
        break;       

      uint victim_va = find_victim_page(victim);
      if (victim_va == 0) {
          continue;
      }
      
      // Get the PTE for this virtual address
      pte_t *pte = walkpgdir(victim->pgdir, (void*)victim_va, 0);

      if (pte && (*pte & PTE_P))
      {
        uint pa = PTE_ADDR(*pte);
        int perm = PTE_FLAGS(*pte);

        int slot = swap_out_page((char*)P2V(pa), perm);
        if (slot >= 0)
        {
          *pte = ((slot + 1) << 12) | (perm & ~PTE_P);
          kfree((char*) P2V(pa));
          victim->page_no--;
        }
      }
    }

    Th -= (Th * BETA) / 100;
    Npg += (Npg * ALPHA) / 100;

    if (Npg >= LIMIT)
    {
      Npg = LIMIT;
    }
  }
}