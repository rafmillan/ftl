#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "nand.h"

#define REM_LEN 8

int main(void) {
    uint8_t wbyte = 0xAA;
    uint8_t rbuf[(PAGE_SIZE * 2) + REM_LEN] = {0x00};
    uint8_t wbuf[(PAGE_SIZE * 2) + REM_LEN];
    uint8_t obuf[REM_LEN] = {0};

    memset(wbuf, wbyte, sizeof(wbuf));

    nand_init();

    if (nand_write_bytes(wbuf, sizeof(wbuf)) == RET_FAILURE) {
        printf("[FAIL]\n");
        printf("\tnand write failed!\n");
        return 1;
    }

    nand_page_read(rbuf, 0, 0);
    nand_page_read(rbuf + PAGE_SIZE, 0, 1);
    nand_page_read(rbuf+(PAGE_SIZE * 2), 0, 2);

    for (int i = 0; i < PAGE_SIZE*2; i++) {
        if (rbuf[i] != wbyte) {
            printf("test_nand_write_bytes: ");
            printf("[FAIL]\n");
            printf("\tExpected: 0x%02X, Actual: 0x%02X\n", wbyte, rbuf[i]);
            return 1;
        }
    }

    for (int i = 0; i < REM_LEN; i++) {
        if (rbuf[(PAGE_SIZE*2) + i] != wbyte) {
            printf("test_nand_write_bytes: ");
            printf("[FAIL]\n");
            printf("\tExpected: 0x%02X, Actual: 0x%02X\n", wbyte, rbuf[(PAGE_SIZE*2) + i]);
            return 1;
        }
    }

    printf("test_nand_write_bytes: ");
    printf("[PASS]\n");
    return 0;
}