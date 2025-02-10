#include "nand.h"
#include "string.h"

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
        nand_t->blocks[block].erase_count = 0;
        nand_t->blocks[block].marked_bad = 0;
        nand_t->blocks[block].next_wpage = 0;
    }

    for (block = 0; block < NAND_SIZE; block++) {
        nand_block_erase(block);
    }
    nand_t->next_lba = 0;
    nand_t->max_lba = PAGES_PER_BLOCK*NAND_SIZE-1;
    nand_t->free_block_count = NAND_SIZE;

    l2p_init();

    get_nand_info();
}

void get_nand_info(void)
{
    DBG_MSG("BLOCKS\t\t\t\t\t%d\t(0x%08x)\n", NAND_SIZE, NAND_SIZE);
    DBG_MSG("PAGES PER BLOCK\t\t\t%d\t(0x%08x)\n", PAGES_PER_BLOCK, PAGES_PER_BLOCK);
    DBG_MSG("TOTAL PAGES\t\t\t\t%d\t(0x%08x)\n", PAGES_PER_BLOCK*NAND_SIZE, PAGES_PER_BLOCK*NAND_SIZE);
    DBG_MSG("PAGE SIZE\t\t\t\t%d\t(0x%08x)\n", PAGE_SIZE, PAGE_SIZE);
    DBG_MSG("TOTAL BYTES\t\t\t\t%d\t(0x%08x)\n", PAGE_SIZE*PAGES_PER_BLOCK*NAND_SIZE, PAGE_SIZE*PAGES_PER_BLOCK*NAND_SIZE);
    DBG_MSG("TOTAL LBA\t\t\t\t%d\t(0x%08x)\n", PAGES_PER_BLOCK*NAND_SIZE, PAGES_PER_BLOCK*NAND_SIZE);
    DBG_MSG("MAX LBA\t\t\t\t\t%d\t(0x%08x)\n", nand_t->max_lba, nand_t->max_lba);
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

    overflow_free_head = 0;
}

static int l2p_insert(uint32_t lba, uint32_t block, uint32_t page)
{
    if (lba > get_max_lba())
        return RET_FAILURE;

    if (overflow_free_head == -1) {
        DBG_MSG("fatal error: out of l2p mapping space\n");
        return RET_FAILURE;
    }

    uint32_t hash;
    L2PEntry* entry;
    Page* _page;

    hash = lba_hash(lba);
    entry = &l2p_primary[hash];
    _page = &nand_t->blocks[block].pages[page];

    /* Empty primary entry: use it directly */
    if (entry->lba == UINT32_MAX) {
        DBG_MSG("free l2p entry\n");
        entry->lba = lba;
        entry->pba_block = block;
        entry->pba_page = page;
        entry->next = -1;
        _page->p2l = entry;
        return RET_SUCCESS;
    }

    /* Update primary entry if LBA already exists */
    if (entry->lba == lba) {
        DBG_MSG("update l2p primary entry\n");
        entry->pba_block = block;
        entry->pba_page = page;
        _page->p2l = entry;
        return RET_SUCCESS;
    }

    /* Collision: traverse the overflow chain */
    DBG_MSG("l2p map collision\n");
    int16_t prev_index = -1;
    int16_t current_index = entry->next;
    while (current_index != -1) {
        L2PEntry* overflow_entry = &l2p_overflow[current_index];
        if (overflow_entry->lba == lba) {
            // Found an existing mapping in the overflow: update it.
            overflow_entry->pba_block = block;
            overflow_entry->pba_page = page;
            _page->p2l = overflow_entry;
            return RET_SUCCESS;
        }
        prev_index = current_index;
        current_index = overflow_entry->next;
    }

    /* Allocate a new overflow entry from the free list */
    if (overflow_free_head == -1) {
        // No free overflow slots available.
        return RET_FAILURE;
    }

    // Pop the free list: take the free index from the head.
    int16_t new_index = overflow_free_head;
    L2PEntry *new_entry = &l2p_overflow[new_index];
    overflow_free_head = new_entry->next;  // Advance the free list head.

    // Populate the new overflow entry.
    new_entry->lba = lba;
    new_entry->pba_block = block;
    new_entry->pba_page = page;
    new_entry->next = -1;  // This new entry is added at the end of the chain.
    _page->p2l = new_entry;

    /* Append the new entry to the overflow chain */
    if (prev_index == -1) {
        // There was no overflow chain before; attach new entry as first overflow.
        entry->next = new_index;
    } else {
        l2p_overflow[prev_index].next = new_index;
    }

    return RET_SUCCESS;
}

static int get_next_lba(void)
{
    uint32_t next_lba = nand_t->next_lba;

    // Case 1: next_lba is unassigned
    if (next_lba == UINT32_MAX) {
        nand_t->next_lba = 0;  // Start assigning from LBA 0
        return 0;
    }

    // Case 2: next_lba is assigned - find the next available LBA
    for (uint32_t lba = next_lba; lba <= get_max_lba(); lba++) {
        uint32_t block, page;
        // Check if the LBA is already mapped
        if (l2p_lookup(lba, &block, &page) != RET_SUCCESS) {
            // LBA is available
            return lba;
        }
    }

    // No available LBAs
    return RET_FAILURE;
}

int read_lba(uint32_t lba, uint8_t* rbuf)
{
    if (rbuf == NULL)
        return RET_FAILURE;
    
    uint32_t block;
    uint32_t page;
    
    DBG_MSG("read lba %d\n", lba);
    if (l2p_lookup(lba, &block, &page) == RET_FAILURE) {
        DBG_MSG("error: invalid lba\n");
        return RET_FAILURE;
    }

    nand_page_read(rbuf, block, page);
    return RET_SUCCESS;
}

int write_lba(uint32_t lba, uint8_t* wbuf)
{
    if (wbuf == NULL || lba > get_max_lba())
        return RET_FAILURE;
    
    uint32_t block;
    int free_idx;

    DBG_MSG("write lba %d\n", lba);
    free_idx = find_next_free_block();
    if (free_idx < 0) {
        return RET_FAILURE;
    }

    uint32_t old_block;
    uint32_t old_page;
    if (l2p_lookup(lba, &old_block, &old_page) == RET_SUCCESS) {
        DBG_MSG("LBA %d is already mapped, invalidating old page\n", lba);
        trim_lba(lba);  // Invalidate old mapping
    }

    Block* blk = &(nand_t->blocks[free_idx]);
    if (l2p_insert(lba, free_idx, blk->next_wpage) == RET_FAILURE) {
        return RET_FAILURE;
    }

    if (nand_page_write(wbuf, free_idx, blk->next_wpage++) == RET_FAILURE) {
        return RET_FAILURE;
    }

    return RET_SUCCESS;
}

int trim_lba(uint32_t lba) {
    if (lba > get_max_lba())
        return RET_FAILURE;

    uint32_t block;
    uint32_t page;
    L2PEntry* entry;

    DBG_MSG("trim lba %d\n", lba);
    if (l2p_lookup(lba, &block, &page) == RET_FAILURE) {
        DBG_MSG("error: invalid lba\n");
        return RET_FAILURE;
    }

    if (l2p_delete(lba) == RET_FAILURE) {
        DBG_MSG("error: could not delete L2P entry\n");
        return RET_FAILURE;
    }

    nand_t->blocks[block].pages[page].p2l = NULL;
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
        L2PEntry* overflow = &(l2p_overflow[curr_index]);
        if (overflow->lba == lba) {
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

static int l2p_delete(uint32_t lba)
{
    uint32_t hash = lba_hash(lba);
    L2PEntry* entry = &l2p_primary[hash];

    // Check if the primary entry matches the target LBA.
    if (entry->lba == lba) {
        // If there is an overflow chain, promote the first overflow entry.
        if (entry->next != -1) {
            int16_t promoted_index = entry->next;
            L2PEntry* promoted_entry = &l2p_overflow[promoted_index];

            // Copy the overflow entry into the primary entry.
            entry->lba = promoted_entry->lba;
            entry->pba_block = promoted_entry->pba_block;
            entry->pba_page = promoted_entry->pba_page;
            entry->next = promoted_entry->next;

            // Reclaim the promoted overflow slot by pushing it onto the free list.
            promoted_entry->lba = UINT32_MAX;  // Mark as free.
            promoted_entry->pba_block = 0;
            promoted_entry->pba_page = 0;
            promoted_entry->next = overflow_free_head; // Link the freed slot.
            overflow_free_head = promoted_index;
        } else {
            // No overflow entries: simply clear the primary entry.
            entry->lba = UINT32_MAX;
            entry->pba_block = 0;
            entry->pba_page = 0;
        }
        return RET_SUCCESS;
    }

    // If the primary entry doesn't match, traverse the overflow chain.
    int16_t prev_index = -1;
    int16_t current_index = entry->next;
    while (current_index != -1) {
        L2PEntry* overflow_entry = &l2p_overflow[current_index];
        if (overflow_entry->lba == lba) {
            // Unlink the entry from the chain.
            if (prev_index == -1) {
                entry->next = overflow_entry->next;
            } else {
                l2p_overflow[prev_index].next = overflow_entry->next;
            }

            // Reclaim the freed slot by pushing it onto the free list.
            overflow_entry->lba = UINT32_MAX;  // Mark as free.
            overflow_entry->pba_block = 0;
            overflow_entry->pba_page = 0;
            overflow_entry->next = overflow_free_head;
            overflow_free_head = current_index;
            return RET_SUCCESS;
        }
        prev_index = current_index;
        current_index = overflow_entry->next;
    }

    return RET_FAILURE; // LBA not found.
}

int nand_page_read(uint8_t* rbuf, uint32_t block, uint32_t page)
{
    if (page >= PAGES_PER_BLOCK || block >= NAND_SIZE)
        return RET_FAILURE;

    DBG_MSG("page read to blk: %d, page: %d\n", block, page);
    Block* blk = &(nand_t->blocks[block]);
    Page* pg = &(blk->pages[page]);

    uint32_t buff_i;
    for (buff_i = 0; buff_i < PAGE_SIZE; buff_i++) {
        rbuf[buff_i] = pg->data[buff_i];
        //DBG_MSG("nand read data %d:%d:%d > 0x%02X\n", block, page, buff_i, rbuf[buff_i]);
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
        //DBG_MSG("nand write data %d:%d:%d (0x%02X) -> LBA %d\n", block, page, i, pg->data[i], nand_t->next_lba);
    }

    // If block is full, decrement free block count
    if (blk->next_wpage == PAGES_PER_BLOCK - 1) {
        nand_t->free_block_count--;
        DBG_MSG("free block count = %d\n", nand_t->free_block_count);
    }

    return RET_SUCCESS;
}

int nand_block_erase(uint32_t block)
{
    if (block >= NAND_SIZE)
        return RET_FAILURE;

    uint32_t page;
    Block* blk = &(nand_t->blocks[block]);

    DBG_MSG("block erase to blk: %d\n", block);

    // Erase all pages
    for (page = 0; page < PAGES_PER_BLOCK; page++) {
        Page* pg = &(blk->pages[page]);
        if (nand_page_erase(block, page) == RET_FAILURE) {
            return RET_FAILURE;
        }
    }

    blk->next_wpage = 0;
    if (blk->erase_count < MAX_ERASE_CYCLES) {
        blk->erase_count++;
    } else {
        DBG_MSG("warning: erase count limit reached for block %d\n", block);
        if (nand_mark_bad(block, EXCEED_PE_CYCLES) == RET_FAILURE) {
            DBG_MSG("critical warning: failed to mark bad block %d\n", block);
        }
    }
    return RET_SUCCESS;
}

int manual_markbad(uint32_t block)
{
    return nand_mark_bad(block, MANUAL_MARKBAD);
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
    pg->ecc = 0;

    // Erase P2L
    pg->p2l = NULL;

    return RET_SUCCESS;
}

static int find_next_free_block(void)
{
    uint32_t min_count = MAX_ERASE_CYCLES;
    int next_block = -1;

    for (uint32_t i = 0; i < NAND_SIZE; i++) {
        if (nand_t->blocks[i].next_wpage != PAGES_PER_BLOCK) {
            if (nand_t->blocks[i].erase_count < min_count && 
                nand_t->blocks[i].erase_count != MAX_ERASE_CYCLES) {
                min_count = nand_t->blocks[i].erase_count;
                next_block = i;
            }
        }
    }
    
    if (next_block == -1) {
        DBG_MSG("error: cannot find next free block\n");
        return -1;  // No free block available
    }

    return next_block;
}

static int nand_mark_bad(uint32_t block, uint32_t reason) {
    if (block > NAND_SIZE)
        return RET_FAILURE;

    DBG_MSG("warning: marking %d block bad! (%x)\n", block, reason);

    Block* blk = &(nand_t->blocks[block]);
    blk->marked_bad = BAD_BLOCK;
    uint32_t lba;

    if ((reason & EXCEED_PE_CYCLES) > 0) {
        // If exceed P/E cycles after successful block erase:
        // no need to migrate the data, just trim LBAs in the block
        for (uint32_t page = 0; page < PAGES_PER_BLOCK; page++) {
            if (p2l_lookup(block, page, &lba) == RET_FAILURE) {
                DBG_MSG("error: could not get physical location for lba %d\n", lba);
                continue;
            }   
            
            if (trim_lba(lba) == RET_FAILURE) {
                DBG_MSG("error: could not trim lba %d\n", lba);
                return RET_FAILURE;
            }
        }
        blk->marked_bad |= EXCEED_PE_CYCLES;
        return RET_SUCCESS;
    }

    if ((reason & MANUAL_MARKBAD) > 0) {
        // migrate the data
        uint8_t buf[PAGE_SIZE] = {0};
        for (uint32_t page = 0; page < PAGES_PER_BLOCK; page++) {
            if (p2l_lookup(block, page, &lba) == RET_FAILURE) {
                DBG_MSG("error: could not get physical location for lba %d\n", lba);
                continue;
            }   

            if (nand_page_read(buf, block, page) == RET_FAILURE) {
                DBG_MSG("error: could not read page\n");
                return RET_FAILURE;
            }

            if (write_lba(lba, buf) == RET_FAILURE) {
                DBG_MSG("error: could not remap lba %d\n", lba);
                return RET_FAILURE;
            }

            uint32_t new_block, new_page;
            if (l2p_lookup(lba, &new_block, &new_page)) {
                DBG_MSG("error: l2p fail\n");
                return RET_FAILURE;
            }

            DBG_MSG("markbad block - remap lba %d from %d:%d to %d:%d\n", lba, block, page, new_block, new_page);
        }
    }

    return RET_SUCCESS;
}

static inline uint32_t lba_hash(uint32_t lba) {
    return lba % HASH_SIZE; // Simple modulo hashing
}