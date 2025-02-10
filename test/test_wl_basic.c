#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "nand.h"

int main(void)
{
    uint8_t rbuf[PAGE_SIZE];
    uint8_t wbuf[PAGE_SIZE];

    memset(wbuf, 0xAA, PAGE_SIZE);

    nand_init();
    uint32_t lba = 0;

    // Wear down block 0
    nand_block_erase(0);

    write_lba(0, wbuf);

    uint32_t blk, pg;
    l2p_lookup(0, &blk, &pg);

    if (blk == 0) {
        printf("%s [FAIL]\n", __FILE__);
        printf("Write to unexpected block! Expected: %d, Actual %d\n", 0, blk);
    }

    printf("%s [PASS]\n", __FILE__);
    return 0;
}