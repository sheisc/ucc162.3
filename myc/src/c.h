#ifndef C_H
#define C_H
/////////////////////////////////////////////////////////////
#define CtrlInfoSize 3	  //活动记录的控制信息的大小,
						  //包括:返回地址、上层活动记录的相对地址，返回值
#define MaxGSize 1024	  //全局符号表最大长度
#define MaxLSize 1024	  //局部符号表最大长度		
#define DsSize 65536		  //数据区大小
#define CsSize 65536		  //代码区大小
#define MaxIdLen 20		  //标志符的最大长度
#define KeyWordsCount 7	  //语言关键字个数
#define HaltingPC  -1	  //若程序计数器值为-1,则停机
/////////////////////////////////////////////////////////////
//四元式
typedef struct {
	int optr;		//运算符
	int arg1;		//左操作数的地址
	int arg2;		//右操作数的地址
	int result;     //存放结果的地址
}Instruction;
//表达式 (采用C的风格，非0为真，0为假)
typedef struct {
	int truelist;		//真出口链    
	int falselist;		//假出口链
	int address;		//存放表达式值的地址或常量的值
						//若为负数，则存放全局变量的地址
						//若为正数，则为局部变量的相对地址
	bool isTemp;		//是否为临时变量
}ExprNode;

//符号表表项
typedef struct {
	int type;			//类型 FUN 或 VAR
	int address;		//地址 全局为非正  局部为正
	char lexeme[MaxIdLen+1];	//词素，变量名或函数名	
	int paramCount;		//函数的参数个数
}SymbolEntry;

//符号表
struct SymbolTable{
	SymbolEntry * entries;	//各表项
	int index;				//符号表当前表尾的下标，表中第0项不用
};

//虚拟机指令集
enum InstrType{
	InsJtrue,	//为真则跳转	(InsJtrue, arg1, , dest)
	InsJfalse,	//为假则跳转	(InsJfalse,arg1, , dest)
	InsJmp,		//无条件跳转	(InsJmp,  ,   , dest)	
	InsMov,		//数据复制      (InsMov, arg1,	  ,dest)
	InsInit,	//初始化某单元	(InsInit,arg1,num,	   )
	InsAdd,		//加法			(InsAdd, arg1,arg2,dest)
	InsSub,		//减法			(InsSub, arg1,arg2,dest)
	InsMul,		//乘法			(InsMul, arg1,arg2,dest)
	InsDiv,		//除法			(InsDiv, arg1,arg2,dest)
	InsMod,		//取余
	InsNop,		//空操作		(InsNop,	,	, 	)
	InsJlt,		//判断是否<		(InsLt,arg1,arg2,result)
	InsJle,		//判断是否<=	(InsLe,arg1,arg2,result)
	InsJgt,		//判断是否>		(InsGt,arg1,arg2,result)
	InsJge,		//判断是否>=	
	InsJeq,		//判断是否==
	InsJne,		//判断是否!=
	InsOr,		//逻辑或运算
	InsAnd,		//逻辑与运算
	InsNot,		//逻辑非运算
	InsIn,		//读入一个整数到单元dest (InsIn,dest , ,);
	InsOut,		//输出一个整数	(InsOut,num, ,);
	InsUminus,	//负数			(InsUminus,oprn, ,dest)
	InsCall,	//过程调用		(InsCall,des, , ,);		
	InsRet,		//过程返回		(InsRet,expr, , );
	InsSetBx,	//设置bx指针，指向活动记录首地址(InsSetBx,addr, , )
	InsAddBx,	//bx指针增加			(InsSetBx,addr);
};

//记号类型
enum SymType
{
	OR ,				//或
	AND,			//与
	RELOP,			//关系运算符
	ADDOP,			//加减	
	MULOP,			//乘除
	NOT,			//非
	LP,				//左括号
	RP,				//右括号
	ID,				//标志符
	NUM,			//数
	ASSIGN,			//赋值	
	LB,				//左大括号
	RB,				//右大括号
	COMMA,			//逗号
	SEMICOLON,		//分号	
	UNDEFINED,		//未定义
	INT,			//int
	IF,				//if
	ELSE,			//else
	WHILE,			//while
	RETURN,			//return
	PRINTF,			//printf
	SCANF,			//scanf
};
//
enum AddOp{
	ADD,			//加法
	SUB				//减法
};
//
enum MulOp{
	MUL,			//乘法
	DIV,			//除法
	MOD,			//取余
};
//标识符类型
enum IdType{
	FUN,			//函数
	VAR				//变量
};

//关系运算符类型
enum RelType{
	GT,				//大于		>
	GE,				//大于等于	>=
	EQ,				//等于		==
	LT,				//小于		<
	LE,				//小于等于	<=
	NE,				//不等于	!=
	NR				//没有关系  用于优先关系表
};
//查找结果
struct SearchingResult{
	int index;		//在符号表中第几项 用0表示未找到
	int tableType;	//符号表的类型
	int address;	//找到的地址
};
//符号表类型 
enum TableType{
	GLOBAL,
	LOCAL,
};

/////////////////////////////////////////////////////////////
extern int errorCount;
extern int ds[DsSize];					//数据段			
extern Instruction cs[CsSize];			//代码段
extern int csIndex;		//代码段下标  0号预留给跳转到main的指令
//extern int dsIndex;		//数据段下标
extern FILE * infile;		//输入文件
extern FILE * outfile;		//输出文件
extern SymbolTable globalTable;				//全局符号表
extern SymbolTable localTable;				//局部符号表
extern char * instrs[];

extern int token;							//当前记号(的类型)
extern int value;							//当前记号的值
extern char id[MaxIdLen+1];					//当前符号串
extern int lineNumber;						//当前行号
extern char * types[];
extern FILE * infile;		//输入文件
/////////////////////////////////////////////////////////////////////
bool isExpressionStarting(int tok);		//tok是否为表达式开始记号
void interpret();			//解释执行中间代码
int getGlobalMemSize();
void callExpr(char * func);
void setActiveTop(int top);
bool isStatementStarting(int tok);
SearchingResult mixingSearch(int idType,char * lexeme);
bool isGlobalState();
int getActiveTop();
void getToken();
int newtemp();
void freeTemp();
void syntaxError(char * info);
void gen(int instrType, int arg1,int arg2,int result);  //产生一条四元式
void programBody();			//程序体
void varDeclare();				//变量声明列表
void funcBody();			//函数体
int paramDeclare();		//函数参数声明
int statement();			//语句
bool isCondition(ExprNode * node);
void decrementTopIfTemp(ExprNode * node);
void setExprNode(ExprNode * node, int tlist,int flist,int addr, bool isTemp);
int mergeList(int list1, int list2);  //合并链表
void backpatch(int list, int cx);	  //回填链表
void changeConditionToArith(ExprNode * node);
void changeArithToCondition(ExprNode * node);
ExprNode expression();		//表达式
int enter(SymbolTable * table,int idType,char * lexeme);
int lookup(SymbolTable * table,int idType,char * lexeme);

////////////////////////////////////////////////////////////////////
#endif
