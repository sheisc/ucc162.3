	A demo to show the basic idea of a "Simple C compiler"

On Linux:
make
make run

iron@MSI:/mnt/d/github/Function_me$ make
gcc  -o sc lex.c expr.c error.c decl.c stmt.c main.c emit.c func.c
./sc < demo.c   > demo.s
gcc  -m32 demo.s -o demo

iron@MSI:/mnt/d/github/Function_me$ make run
./demo 2022 11 12 17 35
5050
2022
6

iron@MSI:/mnt/d/github/Function_me$ wc -l `find . -name "*.h"`
   8 ./decl.h
  35 ./emit.h
   6 ./error.h
  36 ./expr.h
  33 ./func.h
  41 ./lex.h
  28 ./stmt.h
 187 total
 
iron@MSI:/mnt/d/github/Function_me$ wc -l `find . -name "*.c"`
  154 ./decl.c
   21 ./demo.c
   66 ./emit.c
   17 ./error.c
  347 ./expr.c
  112 ./func.c
  107 ./lex.c
  117 ./main.c
  397 ./stmt.c
 1338 total

														sheisc@163.com

