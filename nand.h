#ifndef NAND_H
#define NAND_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define DBG_MSG(format, ...) \
    do { \
        fprintf(stdout, "[%s:%d] " format, __func__, __LINE__, ##__VA_ARGS__); \
        fflush(stdout); \
    } while (0)

#define RET_SUCCESS         0
#define RET_FAILURE         -1
#define PAGE_SIZE           16      // 512
#define ECC_SIZE            8
#define PAGES_PER_BLOCK     8       // 64
#define NAND_SIZE           4       // 4
#define BUFF_SIZE           64
#define HASH_SIZE           16      //1024    // Size of primary hash table (adjust based on LBA range)
#define OVERFLOW_SIZE       16      //256     // Size of overflow pool (adjust based on collision expectations)
#define MAX_ERASE_CYCLES    64

#define DATA_ERASED         0xFF
#define BAD_BLOCK           0x01

struct /*__attribute((packed))*/ mappingEntry {
    uint32_t lba;
    uint32_t pba_block;
    uint32_t pba_page;
    uint32_t next;
};
typedef struct mappingEntry L2PEntry;

struct /*__attribute((packed))*/ page {
    uint8_t data[PAGE_SIZE];
    uint32_t ecc;
    L2PEntry* p2l;
    uint8_t valid;
};
typedef struct page Page;

struct /*__attribute((packed))*/ block {
    Page pages[PAGES_PER_BLOCK];
    uint32_t erase_count;
    uint32_t next_wpage;
    uint8_t marked_bad;
};
typedef struct block Block;

struct /*__attribute((packed))*/ nand {
    uint32_t max_lba;
    uint32_t next_lba;
    uint32_t free_block_count;  // Use a priority queue (min-heap) or sorted list for erase_count
    Block blocks[NAND_SIZE];
};
typedef struct nand NAND;

void nand_init(void);

void get_nand_info(void);
uint32_t get_max_lba(void);

void l2p_init(void);

int write_lba(uint32_t lba, uint8_t* wbuf);
int read_lba(uint32_t lba, uint8_t* rbuf);
int trim_lba(uint32_t lba);
int get_next_lba(void);

int l2p_lookup(uint32_t lba, uint32_t* block, uint32_t* page);

int nand_page_read(uint8_t* rbuf, uint32_t block, uint32_t page);
int nand_page_write(const uint8_t* wbuf, uint32_t block, uint32_t page);
int nand_block_erase(uint32_t block);
int nand_mark_bad(uint32_t block);

static int nand_page_erase(uint32_t block, uint32_t page);
static int find_next_free_block(void);
static int l2p_delete(uint32_t lba);
static int l2p_insert(uint32_t lba, uint32_t block, uint32_t page);

static inline uint32_t lba_hash(uint32_t lba);
#endif