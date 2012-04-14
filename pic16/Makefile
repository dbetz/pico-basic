NAME = pico-basic

CHIP = 24HJ256GP206

OBJS = \
pico-basic.o \
db_compiler.o \
db_expr.o \
db_generate.o \
db_scan.o \
db_statement.o \
db_strings.o \
db_symbols.o \
db_system.o \
db_vmdebug.o \
db_vmfcn.o \
db_vmheap.o \
db_vmint.o \
osint_pic16.o \
XGS_PIC_UART_DRV_V010.o \
XGS_PIC_SYSTEM_V010.o \
simple_vsnprintf.o

CFLAGS = -Os -DXGSPIC -mcpu=$(CHIP) -omf=elf -I..
LDFLAGS = $(CFLAGS) -Wl,--heap=4,-Map=$(NAME).map,--report-mem,-Tp$(CHIP).gld

vpath %.c ..

# basic gnu tools
CC = pic30-gcc
CXX = pic30-g++
LD = pic30-ld
AS = pic30-as
AR = pic30-ar
OBJCOPY = pic30-objcopy
LOADER = pic30-load

$(NAME).elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o *.elf *.map
