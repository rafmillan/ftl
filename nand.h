#include <stdlib.h>
#include <stdint.h>

#define RET_SUCCESS         0
#define RET_FAILURE         -1
#define PAGE_SIZE           16  // 512
#define ECC_SIZE            8
#define PAGES_PER_BLOCK     8   // 64
#define NAND_SIZE           2   // 4
#define BUFF_SIZE           64

#define DATA_ERASED         0xFF

struct page {
    char data[PAGE_SIZE];
    uint8_t valid;
    char ecc[ECC_SIZE];
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
};
typedef struct nand NAND;

void nand_init(void);

int nand_page_read(char* rbuf, uint32_t block, uint32_t page);
int nand_page_write(char* wbuf, uint32_t block, uint32_t page);
int nand_block_erase(uint32_t block);

static int nand_page_erase(uint32_t block, uint32_t page);
