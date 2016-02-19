//
// Copyright (C) BlockWorks Consulting Ltd - All Rights Reserved.
// Unauthorized copying of this file, via any medium is strictly prohibited.
// Proprietary and confidential.
// Written by Steve Tickle <Steve@BlockWorks.co>, September 2014.
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
#include "Alloy.h"

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
// Cause the specified core to start execution at the specified point in physical memory.
//
int SendMail(uint32_t coreNumber)
{
    int     file_desc;

    file_desc = open(DEVICE_FILE_NAME, 0);

    if (file_desc < 0)
    {
        printf("Can't open " DEVICE_FILE_NAME);
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
void ResetCore(uint32_t coreID)
{
    printf("Resetting core %d\n", coreID);

    bridge->coreMessages[coreID][0].type = CORE_MESSAGE_RESET;
    SendMail(coreID);
}


//
//
//
int main(int argc, char* argv[])
{
    memFD = open("/dev/mem", O_RDWR|O_SYNC);
    if(memFD < 0) 
    {
        perror("open");
        return -1;
    }

    alloyRAM = mmap( (void*)ALLOY_RAM_BASE, 1024*1024*8, PROT_READ|PROT_WRITE, MAP_SHARED , memFD, ALLOY_RAM_BASE );
    printf("Alloy RAM base = %08x\n", (uint32_t)alloyRAM);
    bridge   = (CoreServicesBridge*)alloyRAM;

    ResetCore( atoi(argv[1]) );

    return 0;
}


