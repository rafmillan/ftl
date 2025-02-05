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
    nand_t->blk_idx = 0;
}

int nand_write_bytes(const uint8_t* wbuf, uint32_t size)
{
    uint32_t free_idx;
    uint32_t start_page;
    uint32_t curr_page;
    uint32_t whole_pages;
    uint32_t rem_bytes;
    uint32_t index;
    uint32_t windex;
    
    free_idx = find_next_free_block();
    if (free_idx < 0) {
        return RET_FAILURE;
    }

    Block* blk = &(nand_t->blocks[free_idx]);
    start_page = blk->next_wpage;
    curr_page = start_page;

    whole_pages = size / PAGE_SIZE;
    rem_bytes = size % PAGE_SIZE;

    DBG_MSG("nand write size: %08x bytes\n", size);
    DBG_MSG("total write pages: %08x\n", whole_pages);
    DBG_MSG("total rem bytes: %08x\n", rem_bytes);

    if (size != ((whole_pages * PAGE_SIZE) + rem_bytes)) {
        DBG_MSG("value error");
        return RET_FAILURE;
    }
    
    index = 0;
    uint32_t page = curr_page;
    if (whole_pages > 0) {
        for (int p = 0; p < whole_pages; p++) {
            nand_page_write(wbuf + index, free_idx, page++);
            index += PAGE_SIZE;
            blk->next_wpage++;
            curr_page++;
            if (blk->next_wpage == PAGES_PER_BLOCK) {
                DBG_MSG("write read end of block\n");
                page = 0;
                free_idx = find_next_free_block();
                if (free_idx < 0)
                    return RET_FAILURE;
                blk = &(nand_t->blocks[free_idx]);
            }
        }
    }

    windex = 0;
    index = 0;
    if (rem_bytes > 0) {
        while (index < rem_bytes) {
            uint32_t p_idx = (index + 1) % PAGE_SIZE;

            Page* pg = &(blk->pages[curr_page]);
            pg->data[index] = wbuf[windex];
            DBG_MSG("nand write data %d:%d:%d > 0x%02X\n", free_idx, curr_page, index, pg->data[index]);
            index++;
            windex++;

            if (((p_idx + 1) % PAGE_SIZE) == 0) {
                curr_page++;
                blk->next_wpage = curr_page;
                windex = 0;
                if (curr_page == PAGES_PER_BLOCK) {
                    blk->next_wpage = curr_page;
                    curr_page = 0;

                    free_idx = find_next_free_block();
                    if (free_idx < 0)
                        return RET_FAILURE;
                    blk = &(nand_t->blocks[free_idx]);
                }
            }
        }
    }

    return RET_SUCCESS;
}

int nand_page_read(uint8_t* rbuf, uint32_t block, uint32_t page)
{
    if (page >= PAGES_PER_BLOCK || block >= NAND_SIZE)
        return RET_FAILURE;

    DBG_MSG("page read to blk: %d, page: %d\n", block, page);
    Block* blk = &(nand_t->blocks[block]);
    Page* pg = &(blk->pages[page]);

    // if (!pg->valid)
    //     return RET_FAILURE;

    uint32_t buff_i;
    for (buff_i = 0; buff_i < PAGE_SIZE; buff_i++) {
        rbuf[buff_i] = pg->data[buff_i];
        DBG_MSG("nand read data %d:%d:%d > 0x%02X\n", block, page, buff_i, rbuf[buff_i]);
    }
    
    return RET_SUCCESS;
}

int nand_page_write(const uint8_t* wbuf, uint32_t block, uint32_t page)
{
    if (page >= PAGES_PER_BLOCK || block >= NAND_SIZE)
        return RET_FAILURE;

    DBG_MSG("page write to blk: %d, page: %d\n", block, page);
    Block* blk = &(nand_t->blocks[block]);
    Page* pg = &(blk->pages[page]);

    // if (pg->valid) // page has to be erased first
    //     return RET_FAILURE;

    uint32_t i;
    for (i = 0; i < PAGE_SIZE; i++) {
        pg->data[i] = wbuf[i];
        DBG_MSG("nand write data %d:%d:%d > 0x%02X\n", block, page, i, pg->data[i]);
    }

    // pg->valid = 1;

    return RET_SUCCESS;
}

int nand_block_erase(uint32_t block)
{
    if (block >= NAND_SIZE)
        return RET_FAILURE;

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

    // pg->valid = 0;

    return RET_SUCCESS;
}

static int find_next_free_block(void) {
    for (uint32_t i = nand_t->blk_idx; i < NAND_SIZE; i++) {
        if (nand_t->blocks[i].next_wpage == 0) {
            return i;
        }
    }
    return -1;  // No free block available
}
