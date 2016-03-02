//
// Copyright (C) BlockWorks Consulting Ltd - All Rights Reserved.
// Unauthorized copying of this file, via any medium is strictly prohibited.
// Proprietary and confidential.
// Written by Steve Tickle <Steve@BlockWorks.co>, September 2014.
//


#ifndef __CORESERVICES_H__
#define __CORESERVICES_H__


#include <linux/ioctl.h>

//
// Device number
//
#define MAJOR_NUM                   (100)




//
// ioctls for the alloy device.
//
#define IOCTL_START_CORE            _IOWR(MAJOR_NUM, 0, char *)
#define IOCTL_TRIGGER_DOORBELL      _IOWR(MAJOR_NUM, 1, int)


//
// Data transferred to Alloy.ko from userspace to Alloy.ko for the IOCTL_START_CODE operation.
//
typedef struct
{
    uint32_t    coreID;
    uint32_t    startPoint;

} CoreStartData;


//
// Definition of a message as seen from Linux userspace as deliverd by Alloy.ko read().
//
typedef struct
{
    uint32_t    coreID;
    uint32_t    type;
    uint32_t    payload;

} FullCoreMessage;




// 
// The name of the device file 
//
#define DEVICE_FILE_NAME "/dev/Alloy"


//
// Total number of cores.
//
#define NUMBER_OF_CORES         (4)

//
// Address space constants.
//
#define UNCACHED_AREA                   (0x00000000)
#define ALLOY_DEDICATED_RAM_SIZE        (1024*1024*706)
#define ALLOY_RAM_BASE                  (UNCACHED_AREA+(256*1024*1024))
#define BRIDGE_BASE                     (0x10000000)






//
//
//
#define CORE_MESSAGE_NONE       ((uint32_t)-1)
#define CORE_MESSAGE_RESET      (0)
#define CORE_MESSAGE_TEST       (1)

//
// Definition of an Alloy->Linux sytem call message.
//
typedef struct
{
    uint32_t    type;
    uint32_t    payload;
    bool        processedFlag;
    
} SystemCall;

 
//
// Definition of a message as communicated by all the cores.
//
typedef struct
{
    uint32_t    type;
    uint32_t    payload;

} CoreMessage;


//
// Definition of the Bridge area, shared between all cores.
//
typedef struct
{
    volatile CoreMessage    coreMessages[NUMBER_OF_CORES][NUMBER_OF_CORES];
    volatile uint32_t       heartBeats[NUMBER_OF_CORES];
    volatile uint32_t       messageCounts[NUMBER_OF_CORES];

} CoreServicesBridge;


extern CoreServicesBridge*     bridge;



#endif



