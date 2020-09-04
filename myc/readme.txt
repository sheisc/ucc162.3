1.Directory:
---
  |
  |------src 源代码
  |
  |
  |------doc 相关文档
  |
  |
  |------readme.txt

2. How to use
   (1) 在Linux平台上,进入src目录,执行make命令,即可得到myc编译器 
	      make
      	
   (2) 运算测试代码

	    ./myc ./test.c

3. 简单C编译器
(1). 实现栈式存储管理，支持函数的递归调用
(2). 支持函数参数的传递
(3). 实现if语句、while语句、赋值语句(尚未支持for语句)
(4). 不要求局部变量都放在函数体的开始处声明，提供了局部变量声明的灵活性。
(5). 只支持int型（尚支持其他数据类型)，不支持void,要求所有函数的返回值为int
(6). 允许int型全局变量
(7). 为了简单起见，提供scanf和printf关键字来支持输入输出，如
   int a,b ;
   scanf(a,b);		//从键盘接受一个输入
   printf(a+1,b);		//输出a+1和b的值
(8). 尚未支持指针
(9). 要求程序提供int型的main函数
(10). 2007年写的旧代码. 采用的是先编译后解释的思想，自定义了类似X86的汇编形式的中间代码
   然后在虚拟机上解释执行。
(11). 采用的是基于语法图的自顶向下的分析方法。
    expression.cpp实现了表达式的翻译
    program_body.cpp实现了整个程序体的翻译
    statement.cpp实现了语句的翻译
    function_body.cpp实现了函数体的翻译
    var_declare.cpp实现了变量声明的翻译
    param_declare.cpp实现了函数参数声明的翻译
    sym_table.cpp实现了符号表的管理
    lex.cpp实现了词法分析器    
    c.cpp是主程序
    virtual_machine.cpp实现了虚拟机
    c.h为头文件   	
(12). 简单C编译器采用的是Wirth先生的PL/0(简化后的Pascal语言编译器）的框架.		
    

																						sheisc@163.com

																													
						
			
