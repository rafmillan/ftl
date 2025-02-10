#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "nand.h"

int main(void)
{
    uint32_t lba;
    uint32_t block, new_block;
    uint32_t page, new_page;

    uint8_t rbuf[PAGE_SIZE];
    uint8_t wbuf[PAGE_SIZE];

    memset(wbuf, 0xAA, PAGE_SIZE);
    memset(wbuf, 0x00, PAGE_SIZE);

    nand_init();

    for (lba = 0; lba < PAGES_PER_BLOCK; lba++) {
        write_lba(lba, wbuf);

        if (read_lba(lba, rbuf) != 0) {
            printf("%s:[FAIL]\n", __FILE__);
            printf("\tRead to lba %d failed!\n", lba);
            return 1;
        }

        if (memcmp(rbuf, wbuf, PAGE_SIZE) != 0) {
            printf("%s:[FAIL]\n", __FILE__);
            printf("\tRead verify failed!\n");
            return 1;
        }
    }
    
    lba = 0;
    printf("Write completed OK!\n");

    l2p_lookup(lba, &block, &page);
    printf("lba %d -> %d:%d\n", lba, block, page);

    
    if (manual_markbad(block) != 0) {
        printf("%s:[FAIL]\n", __FILE__);
        printf("\tmanual markbad error!\n");
        return 1;
    }
    printf("block %d marked bad!\n", block);

    for (lba = 0; lba < PAGES_PER_BLOCK; lba++) {
        if (l2p_lookup(lba, &new_block, &new_page) != 0) {
            printf("%s:[FAIL]\n", __FILE__);
            printf("\tl2p translation error!\n");
            return 1;
        }

        printf("lba %d remaped from %d:%d to %d:%d\n", lba, block, page, new_block, new_page);
        if (new_block == block || new_block < 0 || new_block > NAND_SIZE - 1) {
            printf("%s:[FAIL]\n", __FILE__);
            printf("\tblock index error! Unexpected value: %d\n", new_block);
            return 1;
        }
    }



    printf("%s [PASS]\n", __FILE__);
    return 0;
}