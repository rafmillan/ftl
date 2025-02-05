#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "nand.h"

int main(void) {

    uint8_t rbuf[PAGE_SIZE];
    uint8_t wbuf[PAGE_SIZE];

    uint8_t erased = DATA_ERASED;
    char* str = "Hello World!";
    memcpy(wbuf, str, strlen(str));

    nand_init();

    nand_page_write(wbuf, 0, 0);
    nand_page_read(rbuf, 0, 0);

    for (int i = 0; i < PAGE_SIZE; i++) {
        if (rbuf[i] != wbuf[i]) {
            printf("test_nand_write_bytes: ");
            printf("[FAIL]\n");
            printf("\tExpected: %s, Actual: %s\n", wbuf, rbuf);
            return 1;
        }
    }

    memset(rbuf, 0, PAGE_SIZE);
    nand_block_erase(0);
    nand_page_read(rbuf, 0, 0);

    for (int i = 0; i < PAGE_SIZE; i++) {
        if (rbuf[i] != erased) {
            printf("test_nand_write_bytes: ");
            printf("[FAIL]\n");
            printf("\tExpected: 0x%02X, Actual: 0x%02X\n", erased, rbuf[i]);
            return 1;
        }
    }

    printf("test_nand_write_bytes: ");
    printf("[PASS]\n");
    return 0;
}