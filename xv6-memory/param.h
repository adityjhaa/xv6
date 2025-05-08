#define NPROC                64  // maximum number of processes
#define KSTACKSIZE         4096  // size of per-process kernel stack
#define NCPU                  8  // maximum number of CPUs
#define NOFILE               16  // open files per process
#define NFILE               100  // open files per system
#define NINODE               50  // maximum number of active i-nodes
#define NDEV                 10  // maximum major device number
#define ROOTDEV               1  // device number of file system root disk
#define MAXARG               32  // max exec arguments
#define MAXOPBLOCKS          10  // max # of blocks any FS op writes
#define LOGSIZE              (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF                 (MAXOPBLOCKS*3)  // size of disk block cache

#define SECTOR_SIZE          512
#define PAGE_SIZE            4096
#define SWAP_SLOTS           800 // size of swap blocks
#define SWAP_BLOCKS_PER_SLOT (PAGE_SIZE / SECTOR_SIZE)
#define SWAP_BLOCKS_TOTAL    (SWAP_SLOTS * SWAP_BLOCKS_PER_SLOT)

#define FSSIZE               (1000 + SWAP_BLOCKS_TOTAL)  // size of file system in blocks
