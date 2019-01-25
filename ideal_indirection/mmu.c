/**
 * Ideal Indirection Lab
 * CS 241 - Fall 2016
 */
#include "kernel.h"
#include "mmu.h"
#include <assert.h>
#include <stdio.h>

MMU *MMU_create() {
  MMU *mmu = calloc(1, sizeof(MMU));
  mmu->tlb = TLB_create();
  mmu->curr_pid = 0;
  return mmu;
}

void * maskVirtualAddress(void * virtual_address, int lowestBitsKeep) {
	unsigned long long mask = 1;
	/*for(int i = 0; i < lowestBitsKeep; i++) {
		mask *= 2;
	}*/
	mask = 1 << lowestBitsKeep;
	mask--;
	return (void*) ((unsigned long long) virtual_address & mask);
}

//Mask to keep only the bits between [hiBitKeep,loBitKeep] (0-based, incl.),
//  and shift right s.t. loBitKeep becomes the LSB.
//		(Ex: maskAndShiftVA(11010111, 6, 4): shifted = 11010111>>5 = 00001101)
//	  	ret. maskVA(00001101, 6-4+1=3) = ret. 00000101
void * maskAndShiftVA(void * virtual_address, unsigned long long hiBitKeep, unsigned long long loBitKeep) {
	void* shifted = (void*) (((unsigned long long) virtual_address) >> loBitKeep);
	return maskVirtualAddress(shifted, hiBitKeep-loBitKeep+1);
}

void *MMU_get_physical_address(MMU *mmu, void *virtual_address, size_t pid) {
  assert(pid < MAX_PROCESS_ID);
  void * maskedVA = maskVirtualAddress(virtual_address, VIRTUAL_ADDRESS_LENGTH);
  void * physical = NULL;
  if(pid != mmu->curr_pid) { //different pid than before
  	TLB_flush(&mmu->tlb); //for security, flush TLB
  	mmu->curr_pid = pid;
  }
  else { //check TLB: much faster if it has it
  	physical = TLB_get_physical_address(&mmu->tlb, maskedVA);
  	if(physical) {
  		//printf("==========TLB found match!==============\n");
  		return physical;
  	}
  }
  //Not in TLB; search 3-tiered PageTables:
  MMU_tlb_miss(mmu, virtual_address, pid); //tlb doesn't have it
  if(!mmu->base_pts[pid]) { MMU_raise_page_fault(mmu, virtual_address, pid); return NULL; }
  size_t vpn1 = (size_t)maskAndShiftVA(virtual_address, VIRTUAL_ADDRESS_LENGTH-1, VIRTUAL_ADDRESS_LENGTH-PAGE_NUMBER_LENGTH);
  size_t vpn2 = (size_t)maskAndShiftVA(virtual_address, VIRTUAL_ADDRESS_LENGTH-1-PAGE_NUMBER_LENGTH, VIRTUAL_ADDRESS_LENGTH-2*PAGE_NUMBER_LENGTH);
  size_t vpn3 = (size_t)maskAndShiftVA(virtual_address, VIRTUAL_ADDRESS_LENGTH-1-2*PAGE_NUMBER_LENGTH, VIRTUAL_ADDRESS_LENGTH-3*PAGE_NUMBER_LENGTH);
  size_t offset = (size_t)maskVirtualAddress(virtual_address, OFFSET_LENGTH); //only useful for vpn3
  physical = PageTable_get_entry(mmu->base_pts[pid], vpn1) + vpn2; //get vp2 from deref. vpn1 and offset of vpn2
  if(physical) { //tiered conditionals to prevent throwing multiple page faults when an earlier tier DNE
  	physical = PageTable_get_entry(mmu->base_pts[pid], (size_t) physical) + vpn3; //get vp3 from deref. vp2 and offset of vpn3
  	if(physical) {
  		physical = (char*) PageTable_get_entry(mmu->base_pts[pid], (size_t) physical) + offset; //get physical address from deref. vp3 and actual offset
  		TLB_add_physical_address(&mmu->tlb, maskedVA, physical); //Add to TLB cache when done, if successful
  	} else MMU_raise_page_fault(mmu, virtual_address, pid);
  } else MMU_raise_page_fault(mmu, virtual_address, pid);
  return physical;
}

void MMU_tlb_miss(MMU *mmu, void *address, size_t pid) {
  assert(pid < MAX_PROCESS_ID);
  mmu->num_tlb_misses++;
  printf("Process [%lu] tried to access [%p] and it couldn't find it in the "
         "TLB so the MMU has to check the PAGE TABLES\n",
         pid, address);
}

void MMU_raise_page_fault(MMU *mmu, void *address, size_t pid) {
  assert(pid < MAX_PROCESS_ID);
  mmu->num_page_faults++;
  printf(
      "Process [%lu] tried to access [%p] and the MMU got an invalid entry\n",
      pid, address);
}

void MMU_add_process(MMU *mmu, size_t pid) {
  assert(pid < MAX_PROCESS_ID);
  mmu->base_pts[pid] = PageTable_create();
}

void MMU_free_process_tables(MMU *mmu, size_t pid) {
  assert(pid < MAX_PROCESS_ID);
  PageTable *base_pt = mmu->base_pts[pid];
  Pagetable_delete_tree(base_pt);
}

void MMU_delete(MMU *mmu) {
  for (size_t i = 0; i < MAX_PROCESS_ID; i++) {
    MMU_free_process_tables(mmu, i);
  }
  TLB_delete(mmu->tlb);
  free(mmu);
}
