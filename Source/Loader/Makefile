#
#
#


SRC=main.c loader.c
ASRC=

TARGET=CoreLoader

ifeq ($(PLATFORM),RPI)
 CROSS	= arm-linux-gnueabi-
 CFLAGS = -O0 -ggdb3 -mcpu=cortex-a7 -mthumb \
	-ffunction-sections -fdata-sections \
	-Iarm/CMSIS/include -I.
endif

ifeq ($(PLATFORM),PC)
 CROSS	= 
 CFLAGS = -O0 -ggdb3 \
	-ffunction-sections -fdata-sections \
	-I.
endif

CC=$(CROSS)gcc
AS=$(CROSS)gcc
LD=$(CROSS)gcc


INCLUDES	= -I../CoreServices

LDFLAGS=\
  -Wl,--start-group -lgcc -lc -lc -lm -Wl,--end-group \
  -Wl,--gc-sections

OBJS=$(SRC:.c=.o) $(ASRC:.S=.o)
DEPS=$(SRC:.c=.d)

all: $(TARGET)

-include $(DEPS)

%.o: %.c
	@echo " CC $<"
	@$(CC) -MMD $(CFLAGS)  $(INCLUDES) -o $@ -c $<

%.o: %.S
	@echo " AS $<"
	@$(AS) $(CFLAGS) -o $@ -c $<

.PHONY: clean all debug app

$(TARGET): $(OBJS)
	@echo " LINK $@"
	@$(LD) -o $@ $(LDFLAGS) $^

app:
	@$(MAKE) -C app clean all list

clean:
	@echo " CLEAN"
	@rm -fR $(OBJS) $(DEPS) $(TARGET)
