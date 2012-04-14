NAME = ebasic

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
osint_posix.o

CFLAGS = -Os -DMAC -m32
LDFLAGS = $(CFLAGS)

$(NAME): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o $(NAME)
