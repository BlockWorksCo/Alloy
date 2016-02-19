//
// Copyright (C) BlockWorks Consulting Ltd - All Rights Reserved.
// Unauthorized copying of this file, via any medium is strictly prohibited.
// Proprietary and confidential.
// Written by Steve Tickle <Steve@BlockWorks.co>, November 2015.
//


#include <stdint.h>
#include <stdbool.h>
#include "Alloy.h"
#include <stdarg.h>
#include <sys/types.h>

int snprintf(char *str, size_t len, const char *fmt, ...);


#define dsb(option) asm volatile ("dsb " #option : : : "memory")
#define EI()        asm volatile ("cpsie i")
#define DI()        asm volatile ("cpsid i")

//
//
//
typedef struct
{
    CoreMessage             message;
    uint32_t                numberOfMessagesAvailable;
    
} GlobalData;

GlobalData              globals[NUMBER_OF_CORES];
CoreServicesBridge*     bridge                      = (CoreServicesBridge*)BRIDGE_BASE;



void __stack_chk_guard()
{

}


void __stack_chk_fail()
{

}

//
//
//
void PANIC()
{
    while(true);
}



//
// Get the Multiprocessor affinity register (core id).
//
uint32_t MPIDR()
{
    uint32_t    mpidr;

    __asm__ volatile("mrc p15, 0, %0, c0, c0, 5\n\t" : "=r"(mpidr));    

    uint32_t coreID     = mpidr & 0x03;
    return coreID;
}



//
// Cause a reset request.
//
void CWRR()
{
    //
    // Cause a reset request.
    //
    register uint32_t    cwrr    = 0x00000002;
    __asm__ volatile("mcr p14, 0, %0, c1, c4, 4\n\t" : : "r"(cwrr));
}


//
//
//
GlobalData* Globals()
{
    uint32_t    coreID              = MPIDR();

    return &globals[coreID];
}


//
//
//
void WaitForMessage()
{
    while(Globals()->numberOfMessagesAvailable == 0)
    {
        //WFI();
    }
}



#define STACK_SIZE              (1024)
#define NUMBER_OF_ALLOY_CORES   (4)
#define NUMBER_OF_VECTORS       (256)



//
//
//
void SetVectorTableAddress(uint32_t address)
{
    register uint32_t   temp    = address;
    asm volatile ("mcr p15, 0, %0, c12, c0,  0" : "=r" (temp));
}





//
//
//
void  __attribute__ ((interrupt ("IRQ"))) Handler()
{
    PANIC();
}


//
//
//
void TriggerMailboxInterrupt(uint32_t toID)
{
    uint32_t    mailboxSetAddress;
    uint32_t    coreID  = MPIDR();

    mailboxSetAddress     = 0x40000080 + (0x10*toID);
    *(uint32_t*)mailboxSetAddress     = 1<<coreID;
}

//
//
//
bool IsThereMailFromCore(uint32_t fromID)
{
    uint32_t    coreID              = MPIDR();
    uint32_t    mailboxAddress      = 0x400000c0 + (0x10*coreID);;
    uint32_t    mailboxSource       = *(uint32_t*)mailboxAddress;

    if( (mailboxSource&(1<<fromID)) != 0)
    {
        return true;
    }
    else
    {
        return false;
    }

}


//
//
//
CoreMessage* NextMessage()
{
    if(Globals()->numberOfMessagesAvailable > 0)
    {
        return &Globals()->message;
    }
    else
    {
        return 0;
    }
}

//
//
//
void ReleaseMessage(CoreMessage* msg)
{
    Globals()->numberOfMessagesAvailable--;
}


//
//
//
void ClearMailboxFromCore(uint32_t fromID)
{
    uint32_t    mailboxClearAddress;
    uint32_t    coreID  = MPIDR();

    mailboxClearAddress     = 0x400000c0 + (0x10*coreID);
    *(uint32_t*)mailboxClearAddress     = 1<<fromID;
}

//
//
//
void EnableMailboxFromCore()
{
    uint32_t    coreID  = MPIDR();
    uint32_t    mailboxInterruptControlAddress  = 0x40000050+(coreID*4);
    uint32_t    currentSettings     = *(uint32_t*)mailboxInterruptControlAddress;

    currentSettings |= 0x0000000f;
    *(uint32_t*)mailboxInterruptControlAddress   = currentSettings;
}


//
//
//
void ProcessMessage(CoreMessage* msg)
{
    uint32_t    coreID  = MPIDR();
    bridge->messageCounts[coreID]++;

    //
    //
    //
    if(msg->type == CORE_MESSAGE_RESET)
    {
        //CWRR();
    }

    if(msg->type == CORE_MESSAGE_TEST)
    {
    }
#if 0
    if(coreID == 2)
    {
        TriggerMailboxInterrupt( 3 );        
    }
    if(coreID == 3)
    {
        TriggerMailboxInterrupt( 1 );        
    }
#endif    
}


//
//
//
void  __attribute__ ((interrupt ("IRQ"))) IRQHandler()
{
    uint32_t    coreID  = MPIDR();

    //
    //
    //
    if(IsThereMailFromCore(0) == true)
    {
        Globals()->message.type    = bridge->coreMessages[coreID][0].type;
        Globals()->message.payload = bridge->coreMessages[coreID][0].payload;
        Globals()->numberOfMessagesAvailable++;

        ClearMailboxFromCore( 0 );
    }

    if(IsThereMailFromCore(1) == true)
    {
        Globals()->message.type    = bridge->coreMessages[coreID][1].type;
        Globals()->message.payload = bridge->coreMessages[coreID][1].payload;
        Globals()->numberOfMessagesAvailable++;
        
        ClearMailboxFromCore( 1 );
    }

    if(IsThereMailFromCore(2) == true)
    {
        Globals()->message.type    = bridge->coreMessages[coreID][2].type;
        Globals()->message.payload = bridge->coreMessages[coreID][2].payload;
        Globals()->numberOfMessagesAvailable++;
        
        ClearMailboxFromCore( 2 );
    }

    if(IsThereMailFromCore(3) == true)
    {
        Globals()->message.type    = bridge->coreMessages[coreID][3].type;
        Globals()->message.payload = bridge->coreMessages[coreID][3].payload;
        Globals()->numberOfMessagesAvailable++;
        
        ClearMailboxFromCore( 3 );
    }

}


//
//
//
void __attribute__ ( (naked, aligned(128) ) ) VectorTable()
{
    asm volatile ("ldr pc, =Handler");
    asm volatile ("ldr pc, =Handler");
    asm volatile ("ldr pc, =Handler");
    asm volatile ("ldr pc, =Handler");
    asm volatile ("ldr pc, =Handler");
    asm volatile ("ldr pc, =Handler");
    asm volatile ("ldr pc, =IRQHandler");
    asm volatile ("ldr pc, =Handler");
    asm volatile ("ldr pc, =Handler");
    asm volatile ("ldr pc, =Handler");
    asm volatile ("ldr pc, =Handler");
    asm volatile ("ldr pc, =Handler");
    asm volatile ("ldr pc, =Handler");
    asm volatile ("ldr pc, =Handler");
    asm volatile ("ldr pc, =Handler");
    asm volatile ("ldr pc, =Handler");
    asm volatile ("ldr pc, =Handler");
    asm volatile ("ldr pc, =Handler");
}



//
//
//
void DebugText(char* text)
{
    uint32_t    coreID  = MPIDR();

    SystemCall  systemCall  = 
    {
        .type           = 0x00000001,
        .payload        = (uint32_t)text,
        .processedFlag  = false,
    };

    bridge->coreMessages[0][coreID].type    = 123;
    bridge->coreMessages[0][coreID].payload = (uint32_t)&systemCall;
    dsb();
    TriggerMailboxInterrupt(0);            

    while( systemCall.processedFlag == false );    
}


//
//
//
void Core1Main(uint32_t coreID)
{
    //
    //
    //
    bridge->heartBeats[coreID]   = 0;

    //
    // Enable the malbox interrupt.
    //
    EnableMailboxFromCore();

    EI();

    //
    //
    //
    while(true)    
    {
        bridge->heartBeats[coreID]++;
        dsb();
        if( (bridge->heartBeats[coreID] % 0x4ffff) == 0 )
        {
            //
            //
            //
            char    string[64];
            snprintf(string, sizeof(string), "Count on core %d is %d", coreID, bridge->heartBeats[coreID] );
            DebugText( &string[0] );

            TriggerMailboxInterrupt(2);            
        }

        //
        //
        //
        CoreMessage*    msg     = NextMessage();
        if(msg != 0)
        {
            ProcessMessage( msg );
            DebugText("Message Received!");
            ReleaseMessage( msg );
        }
    }        
}

//
//
//
void Core2Main(uint32_t coreID)
{
    //
    //
    //
    bridge->heartBeats[coreID]   = 0;

    //
    // Enable the malbox interrupt.
    //
    EnableMailboxFromCore();

    EI();

    //
    //
    //
    while(true)    
    {
        bridge->heartBeats[coreID]++;
        dsb();
        if( (bridge->heartBeats[coreID] % 0x4ffff) == 0 )
        {
            //
            //
            //
            char    string[64];
            snprintf(string, sizeof(string), "Count on core %d is %d", coreID, bridge->heartBeats[coreID] );
            DebugText( &string[0] );
        }

        //
        //
        //
        CoreMessage*    msg     = NextMessage();
        if(msg != 0)
        {
            ProcessMessage( msg );
            DebugText("Message Received!");
            TriggerMailboxInterrupt(3);            
            ReleaseMessage( msg );
        }
    }        
}

//
//
//
void Core3Main(uint32_t coreID)
{
    //
    //
    //
    bridge->heartBeats[coreID]   = 0;

    //
    // Enable the malbox interrupt.
    //
    EnableMailboxFromCore();

    EI();

    //
    //
    //
    while(true)    
    {
        bridge->heartBeats[coreID]++;
        dsb();
        if( (bridge->heartBeats[coreID] % 0x4ffff) == 0 )
        {
            //
            //
            //
            char    string[64];
            snprintf(string, sizeof(string), "Count on core %d is %d", coreID, bridge->heartBeats[coreID] );
            DebugText( &string[0] );
        }

        //
        //
        //
        CoreMessage*    msg     = NextMessage();
        if(msg != 0)
        {
            ProcessMessage( msg );
            DebugText("Message Received!");
            ReleaseMessage( msg );
        }
    }        
}


//
//
//
void CoreMain(uint32_t coreID)
{
    //
    //
    //
    SetVectorTableAddress( (uint32_t)&VectorTable );

    //
    //
    //
    switch(coreID)
    {
        case 1:
            Core1Main(coreID);
            break;
            
        case 2:
            Core2Main(coreID);
            break;
            
        case 3:
            Core3Main(coreID);
            break;
            
        default:
            break;
    }
}



uint8_t     usrStack[NUMBER_OF_ALLOY_CORES*STACK_SIZE];
uint8_t     irqStack[NUMBER_OF_ALLOY_CORES*STACK_SIZE];


//
//
//
void __attribute__ ( ( naked ) ) EntryPoint()
{
    //
    // Setup the stack(s).
    //
    uint32_t   mpidr;

    //
    //
    //
    __asm__ volatile("mrc p15, 0, %0, c0, c0, 5\n\t" : "=r"(mpidr) ); 
    uint32_t    coreID  = mpidr&0x3;   

    //
    //
    //
    uint32_t            usrStackPointer    = ((uint32_t)&usrStack[coreID*STACK_SIZE]) + STACK_SIZE - 16;
    __asm__ volatile("MOV sp, %0\n\t" : : "r"(usrStackPointer));

    //
    //
    //
    uint32_t            irqStackPointer    = ((uint32_t)&irqStack[coreID*STACK_SIZE]) + STACK_SIZE - 16;
    __asm__ volatile("MSR     CPSR_c, 0xd2");
    __asm__ volatile("MOV sp, %0\n\t" : : "r"(irqStackPointer));
    __asm__ volatile("MSR     CPSR_c, 0xd3");

    //
    //
    //
    Globals()->numberOfMessagesAvailable   = 0;
    bridge                      = (CoreServicesBridge*)BRIDGE_BASE;    

    //
    // Call the CoreMain.
    //
    CoreMain(coreID);

    //
    // Should never get here.
    //
    PANIC();
}



