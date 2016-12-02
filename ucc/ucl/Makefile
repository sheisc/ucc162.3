C_SRC       = alloc.c ast.c decl.c declchk.c dumpast.c emit.c \
              error.c expr.c exprchk.c flow.c fold.c gen.c \
              input.c lex.c output.c reg.c simp.c stmt.c \
              stmtchk.c str.c symbol.c tranexpr.c transtmt.c type.c \
              ucl.c uildasm.c vector.c x86.c x86linux.c
OBJS        = $(C_SRC:.c=.o)
CC          = gcc
CFLAGS      = -g -D_UCC
UCC         = ../driver/ucc

all: $(OBJS) assert.o
	$(CC) -o ucl $(CFLAGS) $(OBJS)

clean:
	rm -f *.o ucl

test: $(C_SRC)
	$(UCC) -o ucl1 $(C_SRC)
	mv $(UCCDIR)/ucl $(UCCDIR)/ucl.bak
	cp ucl1 $(UCCDIR)/ucl
	$(UCC) -o ucl2 $(C_SRC)
	mv $(UCCDIR)/ucl.bak $(UCCDIR)/ucl
	strip ucl1 ucl2
	cmp -l ucl1 ucl2
	rm ucl1 ucl2

