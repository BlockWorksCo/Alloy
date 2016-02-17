#
#
#


CC 			= arm-linux-gnueabi-gcc
CCC			= arm-linux-gnueabi-g++
LD 			= arm-linux-gnueabi-g++

OBJS		+=	CoreTerminal.o

CFLAGS		= 

INCLUDES	= -I../Module


clean:
	-@ $(RM) -f *.o
	-@ $(RM) -f CoreTerminal



all: CoreTerminal




CoreTerminal: $(OBJS)
	$(LD) -o $@ $^ 


.cpp.o:
	$(CCC) $(CFLAGS) $(INCLUDES) -c $<

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<









