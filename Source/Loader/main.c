//
// https://github.com/embedded2014/elf-loader.git
//


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "loader.h"
#include "CoreServices.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>      /* ioctl */
#include <ctype.h>


#define PAGEMAP_LENGTH  8
#define PAGE_SHIFT      12

#define PERIPHERALS_RAM_BASE    (0x40000000)

#define dsb(option) asm volatile ("dsb " #option : : : "memory")


#define DBG(...) printf("ELF: " __VA_ARGS__)
#define ERR(msg) do { perror("ELF: " msg); exit(-1); } while(0)
#define MSG(msg) puts("ELF: " msg)


ELFSymbol_t     exports[]   = {0};
ELFEnv_t        env         = { exports, 0 };

int             memFD       = -1;
uint8_t*        alloyRAM    = 0;
uint8_t*        topOfHeap   = 0;


//
//
//
void* do_alloc(size_t size, size_t align, ELFSecPerm_t perm, uint32_t* physicalAddress)
{
    //
    //
    //
    void*       block       = topOfHeap;
    uint32_t    blockSize   = ((size+(align/2))/align)*align;

    topOfHeap           += blockSize;
    *physicalAddress    = ((uint32_t)block - (uint32_t)alloyRAM) + ALLOY_RAM_BASE;

    printf("topOfHeap = %08x, block = %08x  physicalAddress=%08x alloyRAM=%08x\n", (uint32_t)topOfHeap, (uint32_t)block, (uint32_t)*physicalAddress, (uint32_t)alloyRAM );
    memset(block, 0, size);

    return block;
}




//
// Cause the specified core to start execution at the specified point in physical memory.
//
int TriggerCoreExecution(uint32_t coreID, uint32_t physicalAddress)
{
    int     file_desc;

    file_desc = open(DEVICE_FILE_NAME, 0);

    if (file_desc < 0)
    {
        ERR("Can't open " DEVICE_FILE_NAME);
    }

    CoreStartData   coreStartData   = 
    {
        .coreID         = coreID,
        .startPoint     = physicalAddress,
    };
    int ret_val = ioctl(file_desc, IOCTL_START_CORE, &coreStartData );

    if (ret_val < 0)
    {
        printf("ioctl failed:%d\n", ret_val);
        exit(-1);
    }
    
    close(file_desc);
}


//
// Cause the specified core to start execution at the specified point in physical memory.
//
int SendMail(uint32_t coreNumber)
{
    int     file_desc;

    file_desc = open(DEVICE_FILE_NAME, 0);

    if (file_desc < 0)
    {
        ERR("Can't open " DEVICE_FILE_NAME);
    }

    int ret_val = ioctl(file_desc, IOCTL_TRIGGER_DOORBELL, coreNumber );

    if (ret_val < 0)
    {
        printf("ioctl failed:%d\n", ret_val);
        exit(-1);
    }
    
    close(file_desc);
}



//
//
//
void arch_jumpTo(entry_t entry)
{
    printf("EntryPoint @ %08x \n", (uint32_t)entry );

    //
    // Start the core(s) executing.
    //
    TriggerCoreExecution( 1, (uint32_t)entry );
    TriggerCoreExecution( 2, (uint32_t)entry );
    TriggerCoreExecution( 3, (uint32_t)entry );
}

//
//
//
int main(int argc, char* argv[])
{
    //
    //
    //
    memFD = open("/dev/mem", O_RDWR|O_SYNC);
    if(memFD < 0) 
    {
        perror("open");
        return -1;
    }

    alloyRAM = mmap( (void*)ALLOY_RAM_BASE, 1024*1024*8, PROT_READ|PROT_WRITE, MAP_SHARED , memFD, ALLOY_RAM_BASE );
    printf("Alloy RAM base = %08x\n", (uint32_t)alloyRAM);
    memset(alloyRAM, 0, 4096);
    topOfHeap   = alloyRAM + 4096;

    //
    //
    //
    exec_elf(argv[1], &env);
#if 0
    //
    // Wait for completion.
    //
    while(true)
    {
        CoreServicesBridge*  b  = (CoreServicesBridge*)alloyRAM;

        printf("[%08x %08x %08x %08x] ", b->heartBeats[0], b->heartBeats[1], b->heartBeats[2], b->heartBeats[3] );
        printf("[%08x %08x %08x %08x] \n", b->messageCounts[0], b->messageCounts[1], b->messageCounts[2], b->messageCounts[3] );

        b->coreMessages[2][0].type = CORE_MESSAGE_TEST;
        SendMail(2);

        sleep(1);
    }
#endif
    return 0;
}


