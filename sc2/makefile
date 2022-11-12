C_SRC       = lex.c expr.c error.c decl.c stmt.c main.c emit.c func.c
H_SRC			= lex.h expr.h error.h decl.h stmt.h tokens.txt emit.h func.h
#OBJS        = $(C_SRC:.c=.o)
CC          = gcc 


all:  $(C_SRC) $(H_SRC)
	$(CC) -o sc $(C_SRC)
	./sc < demo.c	> demo.s
	$(CC) -m32 demo.s -o demo

run:
	./demo 2022 11 12 17 35
	
clean:
	rm -f *.o sc demo

