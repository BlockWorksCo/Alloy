#
# Copyright (C) BlockWorks Consulting Ltd - All Rights Reserved.
# Unauthorized copying of this file, via any medium is strictly prohibited.
# Proprietary and confidential.
# Written by Steve Tickle <Steve@BlockWorks.co>, November 2015.
#

CC 			= arm-linux-gnueabi-gcc
CCC			= arm-linux-gnueabi-gcc
LD 			= arm-linux-gnueabi-gcc

OBJS		+=	Main.o \
    		  	vsnprintf.o

CFLAGS 		= -g -fno-common  -march=armv7-a -mlittle-endian -mtune=cortex-a7 
LDFLAGS		= -g -r -fno-common  -Wl,--build-id=none -Wl,-eEntryPoint -nostdlib -L/usr/lib/arm-none-eabi  -T Main.ld -Xlinker -Map=Main.map
LIBS   		= -lgcc -lc

INCLUDES	= -I../../Source/Module


clean:
	-@ $(RM) -f *.o
	-@ $(RM) -f Main



all: Main




Main: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS) 


.cpp.o:
	$(CCC) $(CFLAGS) $(INCLUDES) -c $<

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<









