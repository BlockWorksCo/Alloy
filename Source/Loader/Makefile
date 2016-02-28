#
# Copyright (C) BlockWorks Consulting Ltd - All Rights Reserved.
# Unauthorized copying of this file, via any medium is strictly prohibited.
# Proprietary and confidential.
# Written by Steve Tickle <Steve@BlockWorks.co>, November 2015.
#


CC 			= arm-linux-gnueabi-gcc
CCC			= arm-linux-gnueabi-g++
LD 			= arm-linux-gnueabi-g++

OBJS		+=	Main.o \
    		  	Loader.o

CFLAGS		= 

INCLUDES	= -I../Module


clean:
	-@ $(RM) -f *.o
	-@ $(RM) -f Loader



all: Loader




Loader: $(OBJS)
	$(LD) -o $@ $^ 


.cpp.o:
	$(CCC) $(CFLAGS) $(INCLUDES) -c $<

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<









