#include "vhal.h"
#include "flash.h"

uint8_t vhal_uidstr[33];
uint8_t vhal_uid[16];

int vhalInitNFO(void *data){
	(void)data;
	flash_read_unique_id((uint32_t*)vhal_uid, 4);
	uint8_t *uid = vhal_uid;
    int i;
    for (i = 0; i < 16; i++) {
        uint8_t n1 = uid[i] & 0x0f;
        uint8_t n2 = (uid[i] & 0xf0) >> 4;
        vhal_uidstr[2 * i] = (n1 < 10) ? ('0' + n1) : ('a' - 10 + n1);
        vhal_uidstr[2 * i + 1] = (n2 < 10) ? ('0' + n2) : ('a' - 10 + n2);
    }
    vhal_uidstr[32] = 0;
	return 0;
}

int vhalNfoGetUIDLen() {
    return 16;
}

int vhalNfoGetUID(uint8_t *buf) {
	memcpy(buf,vhal_uid,16);
    return 16;
}

uint8_t* vhalNfoGetUIDStr() {
    return vhal_uidstr;
}
