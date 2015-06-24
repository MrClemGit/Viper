#include "vhal.h"


uint8_t vhal_uidstr[25];

int vhalNfoGetUIDLen() {
    return 12;
}

int vhalNfoGetUID(uint8_t *buf) {
    memcpy(buf, (uint8_t *)0x1FFF7A10, 12);
    return 12;
}

uint8_t* vhalNfoGetUIDStr() {
    return vhal_uidstr;
}

int vhalInitNFO(void *data){
    (void)data;
    uint8_t *uid = (uint8_t *)0x1FFF7A10;
    int i;
    for (i = 0; i < 12; i++) {
        uint8_t n1 = uid[i] & 0x0f;
        uint8_t n2 = (uid[i] & 0xf0) >> 4;
        vhal_uidstr[2 * i] = (n1 < 10) ? ('0' + n1) : ('a' - 10 + n1);
        vhal_uidstr[2 * i + 1] = (n2 < 10) ? ('0' + n2) : ('a' - 10 + n2);
    }
    vhal_uidstr[24] = 0;
    return 0;
}