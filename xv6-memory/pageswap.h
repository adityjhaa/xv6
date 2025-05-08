#ifndef PAGESWAP_H
#define PAGESWAP_H

struct swap_slot {
  int page_perm;
  int is_free;
};

extern struct swap_slot swap_slots[SWAP_SLOTS];

#endif