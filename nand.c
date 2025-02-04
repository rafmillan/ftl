#include "nand.h"

// Compile-time allocation
static NAND nand_die;  
static NAND* nand_t = &nand_die;

void nand_init(void)
{
    uint32_t block, page;
    for (block = 0; block < NAND_SIZE; block++) {
        Block* blk = &(nand_t->blocks[block]);
        for (page = 0; page < PAGES_PER_BLOCK; page++) {
            Page* pg = &(blk->pages[page]);
            nand_page_erase(block, page);
        }
        blk->erase_count = 0;
        blk->next_wpage = 0;
    }
}

int nand_page_read(char* rbuf, uint32_t block, uint32_t page)
{
    if (page >= PAGES_PER_BLOCK || block >= NAND_SIZE)
        return RET_FAILURE;

    Block* blk = &(nand_t->blocks[block]);
    Page* pg = &(blk->pages[page]);

    if (!pg->valid)
        return RET_FAILURE;

    uint32_t buff_i;
    for (buff_i = 0; buff_i < PAGE_SIZE; buff_i++) {
        rbuf[buff_i] = pg->data[buff_i];
    }
    
    return RET_SUCCESS;
}

int nand_page_write(char* wbuf, uint32_t block, uint32_t page)
{
    if (page >= PAGES_PER_BLOCK || block >= NAND_SIZE)
        return RET_FAILURE;

    Block* blk = &(nand_t->blocks[block]);
    Page* pg = &(blk->pages[page]);

    if (pg->valid) // page has to be erased first
        return RET_FAILURE;

    uint32_t i;
    for (i = 0; i < PAGE_SIZE; i++) {
        pg->data[i] = wbuf[i];
    }

    pg->valid = 1;

    return RET_SUCCESS;
}

int nand_block_erase(uint32_t block)
{
    uint32_t page;
    Block* blk = &(nand_t->blocks[block]);

    // Erase all pages
    for (page = 0; page < PAGES_PER_BLOCK; page++) {
        Page* pg = &(blk->pages[page]);
        nand_page_erase(block, page);
    }

    blk->erase_count++;
    blk->next_wpage = 0;
    return RET_SUCCESS;
}

static int nand_page_erase(uint32_t block, uint32_t page)
{
    uint32_t i;
    Block* blk = &(nand_t->blocks[block]);
    Page* pg = &(blk->pages[page]);
    
    // Erase page data
    for (i = 0; i < PAGE_SIZE; i++) {
        pg->data[i] = DATA_ERASED;
    }

    // Erase page ECC
    for (i = 0; i < ECC_SIZE; i++) {
        pg->ecc[i] = DATA_ERASED;
    }

    pg->valid = 0;

    return RET_SUCCESS;
}
