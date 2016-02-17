//
//
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
#include "CoreServices.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>      /* ioctl */
#include <ctype.h>


#define dsb(option) asm volatile ("dsb " #option : : : "memory")

int             memFD       = -1;
uint8_t*        alloyRAM    = 0;





CoreServicesBridge*     bridge  = NULL;



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
    bridge   = (CoreServicesBridge*)alloyRAM;

    //
    //
    //
    int fd = open(DEVICE_FILE_NAME, 0);
    while(true)
    {
        FullCoreMessage     msg;

        int r   = read(fd, &msg, sizeof(msg) );
        if(r == sizeof(msg))
        {
            SystemCall*     systemCall  = (SystemCall*)&alloyRAM[msg.payload - ALLOY_RAM_BASE];
            switch(systemCall->type)
            {
                case 1:
                {
                    //
                    // DebugText
                    //
                    char*           string      = (char*)&alloyRAM[systemCall->payload - ALLOY_RAM_BASE];
                    printf("%d: %s\n", msg.coreID, string );
                    systemCall->processedFlag   = true;

                    break;
                }

                default:
                    break;
            }

        }
    }

    return 0;
}


