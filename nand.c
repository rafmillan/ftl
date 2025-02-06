#include "nand.h"

// Compile-time allocation
static NAND nand_die;  
static NAND* nand_t = &nand_die;

int16_t overflow_free_head;
L2PEntry l2p_primary[HASH_SIZE];
L2PEntry l2p_overflow[OVERFLOW_SIZE];

void nand_init(void)
{
    uint32_t block, page, lba;
    for (block = 0; block < NAND_SIZE; block++) {
        nand_block_erase(block);
    }
    nand_t->blk_idx = 0;
    nand_t->next_lba = 0;
    nand_t->max_lba = PAGES_PER_BLOCK*NAND_SIZE-1;

    l2p_init();

    get_nand_info();
}

void get_nand_info(void)
{
    DBG_MSG("BLOCKS\t\t\t%d\t(0x%08x)\n", NAND_SIZE, NAND_SIZE);
    DBG_MSG("PAGES PER BLOCK\t\t\t%d\t(0x%08x)\n", PAGES_PER_BLOCK, PAGES_PER_BLOCK);
    DBG_MSG("TOTAL PAGES\t\t\t%d\t(0x%08x)\n", PAGES_PER_BLOCK*NAND_SIZE, PAGES_PER_BLOCK*NAND_SIZE);
    DBG_MSG("PAGE SIZE\t\t\t%d\t(0x%08x)\n", PAGE_SIZE, PAGE_SIZE);
    DBG_MSG("TOTAL BYTES\t\t\t%d\t(0x%08x)\n", PAGE_SIZE*PAGES_PER_BLOCK*NAND_SIZE, PAGE_SIZE*PAGES_PER_BLOCK*NAND_SIZE);
    DBG_MSG("TOTAL LBA\t\t\t%d\t(0x%08x)\n", PAGES_PER_BLOCK*NAND_SIZE, PAGES_PER_BLOCK*NAND_SIZE);
    DBG_MSG("MAX LBA\t\t\t%d\t(0x%08x)\n", nand_t->max_lba, nand_t->max_lba);
}

uint32_t get_max_lba(void)
{
    return nand_t->max_lba;
}

void l2p_init(void)
{
    for (int i = 0; i < HASH_SIZE; i++) {
        l2p_primary[i].lba = UINT32_MAX; // Mark as empty
        l2p_primary[i].next = -1;
    }

    for (int i = 0; i < OVERFLOW_SIZE; i++) {
        l2p_overflow[i].lba = UINT32_MAX;
        l2p_overflow[i].next = (i == OVERFLOW_SIZE - 1) ? -1 : i + 1;
    }
}

int insert_lba(uint32_t lba, uint32_t block, uint32_t page)
{
    uint32_t hash;
    L2PEntry* entry;
    Page* _page;



    hash = lba_hash(lba);
    entry = &(l2p_primary[hash]);
    _page = &nand_t->blocks[block].pages[page];

    /* Empty LBA */
    if (entry->lba == UINT32_MAX) {
        entry->lba = lba;
        entry->pba_block = block;
        entry->pba_page = page;
        entry->next = -1;
        _page->p2l = entry;
        return RET_SUCCESS;
    }

    /* Update LBA */
    if (entry->lba == lba) {
        entry->pba_block = block;
        entry->pba_page = page;
        _page->p2l = entry;
        return RET_SUCCESS;
    }

    /* Collision */
    int16_t prev_index = -1;
    int16_t current_index = entry->next;

    while (current_index != -1) {
        L2PEntry *overflow_entry = &l2p_overflow[current_index];
        if (overflow_entry->lba == lba) {
            // Update existing overflow entry
            overflow_entry->pba_block = block;
            overflow_entry->pba_page = page;
            _page->p2l = overflow_entry;
            return RET_SUCCESS;
        }
        prev_index = current_index;
        current_index = overflow_entry->next;
    }

    /* New overflow entry */
    if (overflow_free_head == OVERFLOW_SIZE) {
        return RET_FAILURE; // No free overflow slots
    }

    /* Take available overflow index */
    int16_t new_index = overflow_free_head;
    L2PEntry *new_entry = &l2p_overflow[new_index];
    overflow_free_head++;

    new_entry->lba = lba;
    new_entry->pba_block = block;
    new_entry->pba_page = page;
    new_entry->next = -1;
    _page->p2l = new_entry;
    

    if (prev_index == -1) {
        entry->next = new_index;
    } else {
        l2p_overflow[prev_index].next = new_index;
    }

    return RET_SUCCESS;
}

int read_lba(uint32_t lba, uint8_t* rbuf)
{
    if (rbuf == NULL)
        return RET_FAILURE;
    
    uint32_t block;
    uint32_t page;
    
    DBG_MSG("read lba %d\n", lba);
    if (l2p_lookup(lba, &block, &page) == RET_FAILURE) {
        DBG_MSG("error: invalid lba");
        return RET_FAILURE;
    }

    nand_page_read(rbuf, block, page);
    return RET_SUCCESS;
}


int write_lba(uint32_t lba, uint8_t* wbuf)
{
    if (wbuf == NULL)
        return RET_FAILURE;
    
    uint32_t block;
    int free_idx;

    DBG_MSG("write lba %d\n", lba);
    free_idx = find_next_free_block();
    if (free_idx < 0) {
        return RET_FAILURE;
    }

    Block* blk = &(nand_t->blocks[free_idx]);
    nand_page_write(wbuf, free_idx, blk->next_wpage);
    insert_lba(nand_t->next_lba++, free_idx, blk->next_wpage++);

    return RET_SUCCESS;
}

int l2p_lookup(uint32_t lba, uint32_t* block, uint32_t* page)
{
    uint32_t hash;
    L2PEntry* entry;

    hash = lba_hash(lba);
    entry = &(l2p_primary[hash]);

    /* no collision */
    if (entry->lba == lba) {
        *block = entry->pba_block;
        *page = entry->pba_page;
        return RET_SUCCESS;
    }

    uint32_t curr_index = entry->next;
    while (curr_index != -1) {
        L2PEntry* overflow = &(l2p_overflow[hash]);
        if (entry->lba == lba) {
            *block = overflow->pba_block;
            *page = overflow->pba_page;
            return RET_SUCCESS;
        }
        curr_index = overflow->next;
    }

    return RET_FAILURE;
}

int p2l_lookup(uint32_t block, uint32_t page, uint32_t* lba)
{
    if (block >= NAND_SIZE || page >= PAGES_PER_BLOCK)
        return RET_FAILURE;

    Page* _page = &nand_t->blocks[block].pages[page];
    if (_page->p2l == NULL)
        return RET_FAILURE;
    
    if (_page->p2l->lba > nand_t->max_lba)
        return RET_FAILURE;

    *lba = _page->p2l->lba;
    return  RET_SUCCESS;
}

int l2p_delete(uint32_t lba)
{
    uint32_t hash = lba_hash(lba);
    L2PEntry* entry = &(l2p_primary[hash]);

    int16_t current_index = entry->next;
    if (entry->lba == lba) {
        if (current_index != -1) {
            // Promote the first overflow entry to primary
            L2PEntry* overflow_entry = &l2p_overflow[current_index];
            entry->lba = overflow_entry->lba;
            entry->pba_block = overflow_entry->pba_block;
            entry->pba_page = overflow_entry->pba_page;
            entry->next = overflow_entry->next;

            // Reclaim the overflow slot
            overflow_entry->next = -1;
            overflow_entry->lba = UINT32_MAX; // Clear the entry
            overflow_entry->pba_block = 0;
            overflow_entry->pba_page = 0;
            overflow_free_head = current_index;
        } else {
            // Mark primary entry as empty
            entry->lba = UINT32_MAX;
            entry->pba_block = 0; // Optionally clear these fields
            entry->pba_page = 0;
        }
        return RET_SUCCESS;
    }

    // Traverse overflow chain
    int16_t prev_index = -1;
    while (current_index != -1) {
        L2PEntry* overflow_entry = &l2p_overflow[current_index];
        if (overflow_entry->lba == lba) {
            // Unlink the entry
            if (prev_index == -1) {
                entry->next = overflow_entry->next;
            } else {
                l2p_overflow[prev_index].next = overflow_entry->next;
            }

            // Reclaim the overflow slot
            overflow_entry->next = overflow_free_head;
            overflow_entry->lba = UINT32_MAX; // Clear the entry
            overflow_entry->pba_block = 0;
            overflow_entry->pba_page = 0;
            overflow_free_head = current_index;
            return RET_SUCCESS;
        }
        prev_index = current_index;
        current_index = overflow_entry->next;
    }

    return RET_FAILURE; // LBA not found
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
    uint32_t start_lba;
    uint32_t curr_lba;
    
    free_idx = find_next_free_block();
    if (free_idx < 0) {
        return RET_FAILURE;
    }

    Block* blk = &(nand_t->blocks[free_idx]);
    start_page = blk->next_wpage;
    curr_page = start_page;

    whole_pages = size / PAGE_SIZE;
    rem_bytes = size % PAGE_SIZE;

    DBG_MSG("nand write size: 0x%08X bytes\n", size);
    DBG_MSG("total write pages: 0x%08X\n", whole_pages);
    DBG_MSG("total rem bytes: 0x%08X\n", rem_bytes);

    if (size != ((whole_pages * PAGE_SIZE) + rem_bytes)) {
        DBG_MSG("value error");
        return RET_FAILURE;
    }

    start_lba = nand_t->next_lba;
    curr_lba = start_lba;
    DBG_MSG("start lba: 0x%08X\n", start_lba);
    
    index = 0;
    if (whole_pages > 0) {
        for (int p = 0; p < whole_pages; p++) {
            nand_page_write(wbuf + index, free_idx, curr_page);
            insert_lba(nand_t->next_lba++, free_idx, curr_page++);
            curr_lba = nand_t->next_lba;
            index += PAGE_SIZE;
            blk->next_wpage++;
            if (blk->next_wpage == PAGES_PER_BLOCK) {
                DBG_MSG("write reached end of block\n");
                curr_page = 0;
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
            DBG_MSG("nand write data %d:%d:%d (0x%02X) -> LBA %d\n", free_idx, curr_page, index, pg->data[index], curr_lba);
            index++;
            windex++;

            if (((p_idx + 1) % PAGE_SIZE) == 0) {
                insert_lba(nand_t->next_lba++, free_idx, curr_page);
                curr_lba = nand_t->next_lba;
                blk->next_wpage = curr_page;
                curr_page++;
                windex = 0;
                if (curr_page == PAGES_PER_BLOCK) {
                    DBG_MSG("write reached end of block\n");
                    curr_page = 0;
                    free_idx = find_next_free_block();
                    if (free_idx < 0)
                        return RET_FAILURE;
                    blk = &(nand_t->blocks[free_idx]);
                }
            }
        }
        blk->next_wpage++;
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

    uint32_t lba;
    if (p2l_lookup(block, page, &lba) == RET_FAILURE) {
        DBG_MSG("read error\n");
        return RET_FAILURE;
    }

    uint32_t buff_i;
    for (buff_i = 0; buff_i < PAGE_SIZE; buff_i++) {
        rbuf[buff_i] = pg->data[buff_i];
        DBG_MSG("nand read data %d:%d:%d (%d) > 0x%02X\n", block, page, buff_i, lba, rbuf[buff_i]);
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

    uint32_t i;
    for (i = 0; i < PAGE_SIZE; i++) {
        pg->data[i] = wbuf[i];
        DBG_MSG("nand write data %d:%d:%d (0x%02X) -> LBA %d\n", block, page, i, pg->data[i], nand_t->next_lba);
    }

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

    // Erase P2L
    pg->p2l = NULL;

    return RET_SUCCESS;
}

static int find_next_free_block(void) {
    for (uint32_t i = nand_t->blk_idx; i < NAND_SIZE; i++) {
        if (nand_t->blocks[i].next_wpage != PAGES_PER_BLOCK) {
            return i;
        }
    }
    return -1;  // No free block available
}

static inline uint32_t lba_hash(uint32_t lba) {
    return lba % HASH_SIZE; // Simple modulo hashing
}
