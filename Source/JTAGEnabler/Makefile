#
# Copyright (C) BlockWorks Consulting Ltd - All Rights Reserved.
# Unauthorized copying of this file, via any medium is strictly prohibited.
# Proprietary and confidential.
# Written by Steve Tickle <Steve@BlockWorks.co>, November 2015.
#



CC      = arm-linux-gnueabi-gcc
CCC     = arm-linux-gnueabi-g++
LD      = arm-linux-gnueabi-g++



clean:
	- $(RM) -f *.o
	- $(RM) -f JTAGEnabler



all: JTAGEnabler



JTAGEnabler: JTAGEnabler.o
	$(LD) -o $@ $^ 


.cpp.o:
	$(CCC) -c $<

.c.o:
	$(CC) -c $<

