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
#define PAGE_SIZE           16  // 512
#define ECC_SIZE            8
#define PAGES_PER_BLOCK     8   // 64
#define NAND_SIZE           2   // 4
#define BUFF_SIZE           64

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
};
typedef struct nand NAND;

void nand_init(void);

int nand_write_bytes(const uint8_t* wbuf, uint32_t size);

int nand_page_read(uint8_t* rbuf, uint32_t block, uint32_t page);
int nand_page_write(const uint8_t* wbuf, uint32_t block, uint32_t page);
int nand_block_erase(uint32_t block);

static int nand_page_erase(uint32_t block, uint32_t page);
static int find_next_free_block(void);
#endif