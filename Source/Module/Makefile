#
# Copyright (C) BlockWorks Consulting Ltd - All Rights Reserved.
# Unauthorized copying of this file, via any medium is strictly prohibited.
# Proprietary and confidential.
# Written by Steve Tickle <Steve@BlockWorks.co>, November 2015.
#




TARGET		= CoreServices.ko
MDIR  		= arch/arm/CoreServices
DRIVER_OBJS = CoreServices.o
CURRENT		= $(shell uname -r)

PWD 		= $(shell pwd)
DEST		= /lib/modules/$(CURRENT)/kernel/$(MDIR)
 
obj-m              	:= CoreServices.o
 


default:
	$(MAKE) -C $(KDIR) M=$$PWD
 
 
clean:
	rm -f *.o
	rm -f *.ko
	rm -f *.cmd
	rm -f *.mod.c
	rm -f .*.cmd
	rm -f modules.order
	rm -f Module.symvers
	rm -rf .tmp_versions

-include $(KDIR)/Rules.make

