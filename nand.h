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
#define HASH_SIZE           64      //1024    // Size of primary hash table (adjust based on LBA range)
#define OVERFLOW_SIZE       16      //256     // Size of overflow pool (adjust based on collision expectations)


#define DATA_ERASED         0xFF

struct page {
    uint8_t data[PAGE_SIZE];
    uint8_t valid;
    uint8_t ecc[ECC_SIZE];
};
typedef struct page Page;

struct block {
    Page pages[PAGES_PER_BLOCK];
    uint32_t erase_count;
    uint32_t next_wpage;
};
typedef struct block Block;

struct nand {
    Block blocks[NAND_SIZE];
    uint32_t blk_idx;
    uint32_t max_lba;
    uint32_t next_lba;
};
typedef struct nand NAND;

struct mappingEntry {
    uint32_t lba;
    uint32_t pba_block;
    uint32_t pba_page;
    uint32_t next;
};
typedef struct mappingEntry L2PEntry;

void nand_init(void);
void get_nand_info(void);

void l2p_init(void);

int insert_lba(uint32_t lba, uint32_t block, uint32_t page);
int l2p_lookup(uint32_t lba, uint32_t* block, uint32_t* page);
int l2p_delete(uint32_t lba);

int nand_write_bytes(const uint8_t* wbuf, uint32_t size);
int nand_page_read(uint8_t* rbuf, uint32_t block, uint32_t page);
int nand_page_write(const uint8_t* wbuf, uint32_t block, uint32_t page);
int nand_block_erase(uint32_t block);

static int nand_page_erase(uint32_t block, uint32_t page);
static int find_next_free_block(void);
static inline uint32_t lba_hash(uint32_t lba);
#endif