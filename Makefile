NAME = pico-basic

COMPILER_OBJS = \
db_compiler.o \
db_statement.o \
db_expr.o \
db_scan.o \
db_generate.o

RUNTIME_OBJS = \
db_strings.o \
db_system.o \
db_vmdebug.o \
db_vmfcn.o \
db_vmheap.o \
db_vmint.o \
osint_posix.o

OBJS = pico-basic.o $(COMPILER_OBJS) $(RUNTIME_OBJS)

CFLAGS = -Wall -Os -DMAC -m32
LDFLAGS = $(CFLAGS)

$(NAME): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o $(NAME)
