


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

