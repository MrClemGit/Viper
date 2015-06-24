#include "vhal.h"
#include "vhal_nfo.h"

uint8_t vhal_uidstr[UID_BYTES*2+1];

int vhalNfoGetUIDLen() {
    return UID_BYTES;
}

int vhalNfoGetUID(uint8_t *buf) {
    memcpy(buf, UID_ADDR, 12);
    return UID_BYTES;
}

uint8_t* vhalNfoGetUIDStr() {
    return vhal_uidstr;
}

int vhalInitNFO(void *data){
    (void)data;
    const uint8_t* const uid = UID_ADDR;
    int i;
    for (i = 0; i < UID_BYTES; i++) {
        uint8_t n1 = uid[i] & 0x0f;
        uint8_t n2 = (uid[i] & 0xf0) >> 4;
        vhal_uidstr[2 * i] = (n1 < 10) ? ('0' + n1) : ('a' - 10 + n1);
        vhal_uidstr[2 * i + 1] = (n2 < 10) ? ('0' + n2) : ('a' - 10 + n2);
    }
    vhal_uidstr[UID_BYTES*2] = 0;
    return 0;
}
