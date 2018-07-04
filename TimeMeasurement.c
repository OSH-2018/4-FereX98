#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <emmintrin.h>
#include <x86intrin.h>

int main(){
    int temp = 0;
    unsigned char *addr;
    unsigned char array[10 * 4096];
    unsigned long int basetime;             // time before cache / memory access
    int ite;                                // iterator
    unsigned long int catime, mtime;        // cache access time / memory access time

    memset(array, 0, sizeof(array));

    for(ite = 0; ite < 100; ite++){         // iterate for 100 times to increase reliablility of result
        for(int i = 0; i < 10; ++ i) 
            _mm_clflush(&array[i * 4096]);  // flush relevant cache blocks

        array[4 * 4096] = ite;
        array[9 * 4096] = ite;              // two blocks should be cached

        for(int i = 0;i < 10;++ i){         // measure time of cache / memory access
            addr = &array[i * 4096];
            basetime = __rdtscp(&temp);
            temp = *addr;
            if(i == 4 || i == 9)
                catime += __rdtscp(&temp) - basetime;
            else
                mtime += __rdtscp(&temp) - basetime;
        }
    }

    printf("cache access mean: %d\n", (int)(catime / 2 / ite));
    printf("memory access mean: %d\n", (int)(mtime / 8 / ite));
    return 0;
}