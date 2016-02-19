#
#
#


CC 			= arm-linux-gnueabi-gcc
CCC			= arm-linux-gnueabi-g++
LD 			= arm-linux-gnueabi-g++

OBJS		+=	Main.o \
    		  	vsnprintf.o

CFLAGS 		= -g -fno-common  -march=armv7-a -mlittle-endian -mtune=cortex-a7
LDFLAGS		= -r -fno-common  -Wl,--build-id=none -Wl,-eEntryPoint -nostdlib -L/usr/lib/arm-none-eabi  -T Main.ld

INCLUDES	= -I../../Source/Module


clean:
	-@ $(RM) -f *.o
	-@ $(RM) -f Main



all: Main




Main: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ 


.cpp.o:
	$(CCC) $(CFLAGS) $(INCLUDES) -c $<

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<









