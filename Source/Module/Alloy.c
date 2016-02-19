//
// Copyright (C) BlockWorks Consulting Ltd - All Rights Reserved.
// Unauthorized copying of this file, via any medium is strictly prohibited.
// Proprietary and confidential.
// Written by Steve Tickle <Steve@BlockWorks.co>, November 2015.
//


#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt


#include <linux/module.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <asm/smp_plat.h>
#include <asm/irq.h>

#include "Alloy.h"


#define SUCCESS 0
#define DEVICE_NAME "Alloy"
#define BUF_LEN 80

#define NUMBER_OF_ELEMENTS(a)      (sizeof(a)/sizeof(a[0]))


//
//
//
FullCoreMessage     fullCoreMessages[128];
uint32_t            head                    = 0;
uint32_t            tail                    = 0;

void*               alloyRam                = 0;




//
//
//
bool PutMessage(FullCoreMessage* message)
{
    uint32_t    newHead     = (head + 1) % NUMBER_OF_ELEMENTS(fullCoreMessages);

    if(newHead != tail)
    {
        memcpy( &fullCoreMessages[head], message, sizeof(fullCoreMessages[0]) );
        head    = newHead;        
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
bool GetMessage(FullCoreMessage* message)
{
    if(tail != head)
    {
        memcpy( message, &fullCoreMessages[tail], sizeof(fullCoreMessages[0]) );
        tail    = (tail + 1) % NUMBER_OF_ELEMENTS(fullCoreMessages);        
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
static int device_open(struct inode* inode, struct file* file)
{
    printk(KERN_INFO "device_open(%p)\n", file);
    try_module_get(THIS_MODULE);

    return SUCCESS;
}



//
//
//
static int device_release(struct inode* inode, struct file* file)
{
    module_put(THIS_MODULE);

    return SUCCESS;
}



//
//
//
static ssize_t device_read(struct file* file,   /* see include/linux/fs.h   */
                           char __user* buffer,    /* buffer to be filled with data */
                           size_t length,   /* length of the buffer     */
                           loff_t* offset)
{
    FullCoreMessage     msg;
    uint32_t            i       = 0;
    uint8_t*            src     = (uint8_t*)&msg;

    if( length >= sizeof(msg) )
    {
        bool msgAvailable   = GetMessage( &msg );
        if(msgAvailable == true)
        {
            /*
             * Actually put the data into the buffer
             */
            for(i=0; i<sizeof(msg); i++)
            {
                put_user( *src, buffer );
                src++;
                buffer++;
            }

            /*
             * Read functions are supposed to return the number
             * of bytes actually inserted into the buffer
             */
            return sizeof(FullCoreMessage);
        }
        else
        {
            return 0;
        }        
    }
    else
    {
        return 0;
    }
    
}




//
//
//
static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset)
{
    return 0;
}


//
//
//
bool IsThereMailFromCore(uint32_t fromID)
{
    uint32_t                coreID  = read_cpuid_mpidr() & 0x3;
    uint32_t                mailboxSource       = readl( __io_address(ARM_LOCAL_MAILBOX0_CLR0) + (coreID*0x10));

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
irqreturn_t MailboxIRQHandler0(int irq, void *dev_id, struct pt_regs *regs)
{
    CoreServicesBridge*     bridge  = (CoreServicesBridge*)alloyRam;
    uint32_t                coreID  = read_cpuid_mpidr() & 0x3;
    uint32_t                mailboxSource       = readl( __io_address(ARM_LOCAL_MAILBOX0_CLR0) + (coreID*0x10));
    FullCoreMessage         msg;

    //
    // Clear the interrupt...
    //
    coreID  = 0;
    if( (mailboxSource&(1<<0)) != 0 )
    {
        printk("Message from Core 0: type=%08x payload=%08x\n", bridge->coreMessages[coreID][0].type, bridge->coreMessages[coreID][0].payload );

        msg.coreID      = 0;
        msg.type        = bridge->coreMessages[coreID][0].type;
        msg.payload     = bridge->coreMessages[coreID][0].payload;
        PutMessage( &msg );

        writel( 1<<0, __io_address(ARM_LOCAL_MAILBOX0_CLR0) );
    }

    if( (mailboxSource&(1<<1)) != 0 )
    {
        printk("Message from Core 1: type=%08x payload=%08x\n", bridge->coreMessages[coreID][1].type, bridge->coreMessages[coreID][1].payload );

        msg.coreID      = 1;
        msg.type        = bridge->coreMessages[coreID][1].type;
        msg.payload     = bridge->coreMessages[coreID][1].payload;
        PutMessage( &msg );

        writel( 1<<1, __io_address(ARM_LOCAL_MAILBOX0_CLR0) );
    }

    if( (mailboxSource&(1<<2)) != 0 )
    {
        printk("Message from Core 2: type=%08x payload=%08x\n", bridge->coreMessages[coreID][2].type, bridge->coreMessages[coreID][2].payload );

        msg.coreID      = 2;
        msg.type        = bridge->coreMessages[coreID][2].type;
        msg.payload     = bridge->coreMessages[coreID][2].payload;
        PutMessage( &msg );

        writel( 1<<2, __io_address(ARM_LOCAL_MAILBOX0_CLR0) );
    }

    if( (mailboxSource&(1<<3)) != 0 )
    {
        printk("Message from Core 3: type=%08x payload=%08x\n", bridge->coreMessages[coreID][3].type, bridge->coreMessages[coreID][3].payload );

        msg.coreID      = 3;
        msg.type        = bridge->coreMessages[coreID][3].type;
        msg.payload     = bridge->coreMessages[coreID][3].payload;
        PutMessage( &msg );

        writel( 1<<3, __io_address(ARM_LOCAL_MAILBOX0_CLR0) );
    }


    return IRQ_HANDLED;
}


//
//
//
irqreturn_t MailboxIRQHandler1(int irq, void *dev_id, struct pt_regs *regs)
{
    //
    // Clear the interrupt...
    //
    writel( 0xffffffff, __io_address(ARM_LOCAL_MAILBOX0_CLR0) + 0x04 );
    dsb();

    printk(KERN_INFO "MailboxIRQHandler 1 called.\n");

    return IRQ_HANDLED;
}


//
//
//
irqreturn_t MailboxIRQHandler2(int irq, void *dev_id, struct pt_regs *regs)
{
    //
    // Clear the interrupt...
    //
    writel( 0xffffffff, __io_address(ARM_LOCAL_MAILBOX0_CLR0) + 0x08 );
    dsb();

    printk(KERN_INFO "MailboxIRQHandler 2 called.\n");

    return IRQ_HANDLED;
}


//
//
//
irqreturn_t MailboxIRQHandler3(int irq, void *dev_id, struct pt_regs *regs)
{
    //
    // Clear the interrupt...
    //
    writel( 0xffffffff, __io_address(ARM_LOCAL_MAILBOX0_CLR0) + 0x0c );
    dsb();

    printk(KERN_INFO "MailboxIRQHandler 3 called.\n");

    return IRQ_HANDLED;
}

//
//
//
void SendDoorBellToCore(uint32_t coreNumber)
{
    uint32_t coreID;

    //
    // Ensure that stores to Normal memory are visible to the
    // other CPUs before issuing the IPI.
    //
    dsb();

    //
    // Cause the doorbell interrupt on the remote core.
    //
    coreID  = read_cpuid_mpidr();
    writel(1<<coreID, __io_address(ARM_LOCAL_MAILBOX0_SET0 + (0x10*coreNumber) ) );
}

//
//
//
void StartCore(uint32_t coreID, uint32_t entrypoint)
{
    uint32_t    physAddress     = 0x4000008c + (coreID<<4);
    uint32_t*   entryPointValue = (uint32_t*)physAddress;

    dsb();

    printk(KERN_INFO "Setting core %d entrypoint value to %08x. phys=%08x virt=%08x\n", coreID, entrypoint, physAddress, (uint32_t)entryPointValue);
    writel( entrypoint, __io_address(physAddress) );
    printk(KERN_INFO "Done.\n");
}



//
//
//
long device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    printk(KERN_INFO "ioctl cmd=%d, arg=%08lx", cmd, arg);


    //
    //
    //
    switch (cmd)
    {
        //
        //
        //
        case IOCTL_START_CORE:
        {
            CoreStartData   coreStartData;

            unsigned long r =  __copy_from_user( &coreStartData, (void*)arg, sizeof(coreStartData) );
            if(r == 0)
            {
                StartCore( coreStartData.coreID, coreStartData.startPoint );
            }


            break;
            
        }

        //
        //
        //
        case IOCTL_TRIGGER_DOORBELL:
        {
            SendDoorBellToCore( arg );
            printk(KERN_INFO "Sending doorbell to core %d\n", (int)arg);
            break;
        }

        //
        //
        //
        default:
            return ~SUCCESS;
            break;

    }

    return SUCCESS;
}







//
//
//
struct file_operations Fops =
{
    .read           = device_read,
    .write          = device_write,
    .unlocked_ioctl = device_ioctl,
    .open           = device_open,
    .release        = device_release,
};


//
//
//
static int __init CoreServicesInit(void)
{
    int         ret_val         = 0;
    int         result          = 0;
    uint32_t    currentSettings = 0;



    //
    //
    //
    struct resource* r = request_mem_region(ALLOY_RAM_BASE, ALLOY_DEDICATED_RAM_SIZE, "AlloyRAM");
    if(r == NULL)
    {
        printk("request_mem_region failed.\n");
    }
    else
    {
        printk("request_mem_region ok.\n");
    }

    //
    //
    //
    r = request_mem_region(0x40000000, 4096, "AlloyPeripherals");
    if(r == NULL)
    {
        printk("request_mem_region failed.\n");
    }
    else
    {
        printk("request_mem_region ok.\n");
    }

    //
    //
    //
    ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &Fops);
    if (ret_val < 0)
    {
        printk(KERN_ALERT "%s failed with %d\n", "Sorry, registering the character device ", ret_val);
        return ret_val;
    }

    //
    //
    //
    set_irq_flags(IRQ_ARM_LOCAL_MAILBOX2, IRQF_VALID);
    irq_clear_status_flags(IRQ_ARM_LOCAL_MAILBOX2, IRQ_PER_CPU);
    irq_clear_status_flags(IRQ_ARM_LOCAL_MAILBOX2, IRQ_LEVEL);

    //
    //
    //
    irq_modify_status(IRQ_ARM_LOCAL_MAILBOX0,0xffffffff,0x00000000);
    irq_modify_status(IRQ_ARM_LOCAL_MAILBOX1,0xffffffff,0x00000000);
    irq_modify_status(IRQ_ARM_LOCAL_MAILBOX2,0xffffffff,0x00000000);
    irq_modify_status(IRQ_ARM_LOCAL_MAILBOX3,0xffffffff,0x00000000);

    //
    // Register the interrupt handler for mailbox IRQs.
    // 
    set_irq_flags(IRQ_ARM_LOCAL_MAILBOX2, IRQF_VALID);
    result = request_threaded_irq(   IRQ_ARM_LOCAL_MAILBOX0,                 // The interrupt number requested 
                            (irq_handler_t) MailboxIRQHandler0,      // The pointer to the handler function (above)
                            NULL,
                            IRQF_SHARED,           // Interrupt is on rising edge (button press in Fig.1)
                            "MailboxIRQHandler",                    // Used in /proc/interrupts to identify the owner
                            DEVICE_NAME);                                  // The *dev_id for shared interrupt lines, NULL here

    result = request_threaded_irq(   IRQ_ARM_LOCAL_MAILBOX1,                 // The interrupt number requested 
                            (irq_handler_t) MailboxIRQHandler1,      // The pointer to the handler function (above)
                            NULL,
                            IRQF_SHARED,           // Interrupt is on rising edge (button press in Fig.1)
                            "MailboxIRQHandler",                    // Used in /proc/interrupts to identify the owner
                            DEVICE_NAME);                                  // The *dev_id for shared interrupt lines, NULL here

    result = request_threaded_irq(   IRQ_ARM_LOCAL_MAILBOX2,                 // The interrupt number requested 
                            (irq_handler_t) MailboxIRQHandler2,      // The pointer to the handler function (above)
                            NULL,
                            IRQF_SHARED,           // Interrupt is on rising edge (button press in Fig.1)
                            "MailboxIRQHandler",                    // Used in /proc/interrupts to identify the owner
                            DEVICE_NAME);                                  // The *dev_id for shared interrupt lines, NULL here

    result = request_threaded_irq(   IRQ_ARM_LOCAL_MAILBOX3,                 // The interrupt number requested 
                            (irq_handler_t) MailboxIRQHandler3,      // The pointer to the handler function (above)
                            NULL,
                            IRQF_SHARED,           // Interrupt is on rising edge (button press in Fig.1)
                            "MailboxIRQHandler",                    // Used in /proc/interrupts to identify the owner
                            DEVICE_NAME);                                  // The *dev_id for shared interrupt lines, NULL here
    if(result == 0)
    {
        printk(KERN_INFO "Mailbox ISR registered ok.\n");
    }
    else
    {
        printk(KERN_INFO "Mailbox ISR registration failed (%d).\n", result);        
    }

    //
    // Enable the interupt.
    // We're on Core0 and we want to enable the Mailbox 1 interrupt.
    //
    currentSettings     = readl( __io_address(ARM_LOCAL_MAILBOX_INT_CONTROL0) );
    currentSettings |= 0x0000000f;
    writel( currentSettings, __io_address(ARM_LOCAL_MAILBOX_INT_CONTROL0) );


    //
    //
    //
    alloyRam    = ioremap_nocache( ALLOY_RAM_BASE, ALLOY_DEDICATED_RAM_SIZE );

    printk(KERN_INFO "%s The major device number is %d.\n", "Registeration is a success", MAJOR_NUM);
    printk(KERN_INFO "If you want to talk to the device driver,\n");
    printk(KERN_INFO "you'll have to create a device file. \n");
    printk(KERN_INFO "We suggest you use:\n");
    printk(KERN_INFO "mknod %s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM);
    printk(KERN_INFO "The device file name is important, because\n");
    printk(KERN_INFO "the ioctl program assumes that's the\n");
    printk(KERN_INFO "file you'll use.\n");

    return 0;
}



//
//
//
static void __exit CoreServicesExit(void)
{
    //
    //
    //
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}




module_init(CoreServicesInit);
module_exit(CoreServicesExit);

MODULE_AUTHOR("Steve Tickle");
MODULE_DESCRIPTION("Alloy Core Services module.");
MODULE_LICENSE("GPL");
