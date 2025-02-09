#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "nand.h"

int main(void)
{
    uint8_t rbuf[PAGE_SIZE];
    uint8_t wbuf[PAGE_SIZE];

    memset(rbuf, 0x00, PAGE_SIZE);
    memset(wbuf, 0xAA, PAGE_SIZE);

    uint32_t lba;

    nand_init();

    for (lba = 0; lba <= get_max_lba(); lba++) {
        if (write_lba(lba, wbuf) == RET_FAILURE) {
            printf("%s:[FAIL]\n", __FILE__);
            printf("\tWrite to lba %d failed!\n", lba);
            return 1;
        }

        if (read_lba(lba, rbuf)) {
            printf("%s:[FAIL]\n", __FILE__);
            printf("\tRead to lba %d failed!\n", lba);
            return 1;
        }

        if (memcmp(rbuf, wbuf, PAGE_SIZE) != 0) {
            printf("%s:[FAIL]\n", __FILE__);
            printf("\tRead verify failed!\n");
            return 1;
        }

        if (trim_lba(lba) == RET_FAILURE) {
            printf("%s:[FAIL]\n", __FILE__);
            printf("\tTrim Lba %d failed!\n", lba);
            return 1;
        }

        if (read_lba(lba, rbuf) != RET_FAILURE) {
            printf("%s:[FAIL]\n", __FILE__);
            printf("\tIncorrect LBA access!\n");
            return 1;
        }
    }

    for (uint32_t block = 0; block < NAND_SIZE; block++) {
        nand_block_erase(block);
    }

    for (lba = 0; lba <= get_max_lba(); lba++) {
        if (write_lba(lba, wbuf) == RET_FAILURE) {
            printf("%s:[FAIL]\n", __FILE__);
            printf("\tWrite to lba %d failed!\n", lba);
            return 1;
        }
    }

    for (lba = 0; lba <= get_max_lba(); lba++) {
        if (read_lba(lba, wbuf) == RET_FAILURE) {
            printf("%s:[FAIL]\n", __FILE__);
            printf("\tRead to lba %d failed!\n", lba);
            return 1;
        }

         if (memcmp(rbuf, wbuf, PAGE_SIZE) != 0) {
            printf("%s:[FAIL]\n", __FILE__);
            printf("\tRead verify failed!\n");
            return 1;
        }

        if (trim_lba(lba) == RET_FAILURE) {
            printf("%s:[FAIL]\n", __FILE__);
            printf("\tTrim Lba %d failed!\n", lba);
            return 1;
        }

        if (read_lba(lba, rbuf) != RET_FAILURE) {
            printf("%s:[FAIL]\n", __FILE__);
            printf("\tIncorrect LBA access!\n");
            return 1;
        }
    }

    printf("%s:[PASS]\n", __FILE__);
    return 0;
}