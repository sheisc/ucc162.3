#COMPILE.cpp = $(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
CXX = g++ -Wno-write-strings
OUTPUT_OPTION = -o $@

myc: expression.o program_body.o statement.o function_body.o var_declare.o \
	param_declare.o sym_table.o virtual_machine.o lex.o c.o 
	$(CXX) $^ -o myc
%.o: %.cpp
	$(CXX) -c $(OUTPUT_OPTION) $<
clean:
	rm -rf *.o myc
