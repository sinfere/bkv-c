#include "mem.h"


int main() {
    bs_init();

    u_int8_t* p = bs_malloc(8);
    if (p == NULL) {
        LOGE("malloc fail");
    }    

    bs_debug();

    bs_free(p);

    bs_debug();    

    return 0;
}