#include "ucl.h"
#include "ast.h"
#include "decl.h"
#include "expr.h"
#include "stmt.h"
#include "output.h"

static void DumpExpression(AstExpression expr, int pos)
{
	char *opname = NULL;

	if (expr == NULL)
	{
		fprintf(ASTFile, "NIL");
		return;
	}

	if (expr->op >= OP_COMMA && expr->op <= OP_POSTDEC)
	{
		opname = OPNames[expr->op];
	}

	switch (expr->op)
	{
	case OP_COMMA:
	case OP_ASSIGN:
	case OP_BITOR_ASSIGN:
	case OP_BITXOR_ASSIGN:
	case OP_BITAND_ASSIGN:
	case OP_LSHIFT_ASSIGN:
	case OP_RSHIFT_ASSIGN:
	case OP_ADD_ASSIGN:
	case OP_SUB_ASSIGN:
	case OP_MUL_ASSIGN:    
	case OP_DIV_ASSIGN:
	case OP_MOD_ASSIGN:
	case OP_OR:     
	case OP_AND:
	case OP_BITOR:  
	case OP_BITXOR:  
	case OP_BITAND:	
	case OP_EQUAL:  
	case OP_UNEQUAL: 
	case OP_GREAT: 
	case OP_LESS: 
	case OP_GREAT_EQ: 
	case OP_LESS_EQ:
	case OP_LSHIFT: 
	case OP_RSHIFT: 
	case OP_ADD:   
	case OP_SUB:
	case OP_MUL:    
	case OP_DIV: 
	case OP_MOD:
	case OP_INDEX:
		/*****************************************
		 (+ (+ (+ a
                 b)
              c)
           	d))
		 *****************************************/
		fprintf(ASTFile, "(%s ", opname);
		pos += strlen(opname) + 2;
		DumpExpression(expr->kids[0], pos);
		LeftAlign(ASTFile, pos);
		DumpExpression(expr->kids[1], pos);
		fprintf(ASTFile, ")");
		break;

	case OP_CALL:
		{
			/*********************************
			      (call printf
			            str0,
			            (call f
			                  i))
			 *********************************/
			AstExpression p = expr->kids[1];

			fprintf(ASTFile, "(%s ", opname);
			pos += strlen(opname) + 2;
			DumpExpression(expr->kids[0], pos);
			while (p)
			{
				LeftAlign(ASTFile, pos);
				DumpExpression(p, pos);
				if (p->next != NULL)
				{
					fprintf(ASTFile, ",");
				}
				p = (AstExpression)p->next;
			}
			fprintf(ASTFile, ")");
		}
		break;

	case OP_PREINC: 
	case OP_PREDEC: 
	case OP_POS:    
	case OP_NEG:
	case OP_NOT:     
	case OP_COMP:
	case OP_ADDRESS: 
	case OP_DEREF:

		fprintf(ASTFile, "(%s ", opname);
		pos += strlen(opname) + 2;
		DumpExpression(expr->kids[0], pos);
		fprintf(ASTFile, ")");
		break;

	case OP_CAST:

		fprintf(ASTFile, "(%s ", opname);
		pos += strlen(opname) + 2;
		fprintf(ASTFile, "%s",TypeToString(expr->ty));
		LeftAlign(ASTFile, pos);
		DumpExpression(expr->kids[0], pos);
		fprintf(ASTFile, ")");
		break;

	case OP_SIZEOF:

		fprintf(ASTFile, "(%s ", opname);
		pos += strlen(opname) + 2;
		if (expr->kids[0]->kind == NK_Expression)
		{
			DumpExpression(expr->kids[0], pos);
		}
		fprintf(ASTFile, ")");
		break;

	case OP_MEMBER:
	case OP_PTR_MEMBER:

		fprintf(ASTFile, "(%s ", opname);
		pos += strlen(opname) + 2;
		DumpExpression(expr->kids[0], pos);
		LeftAlign(ASTFile, pos);
		fprintf(ASTFile, "%s", ((Field)expr->val.p)->id);
		fprintf(ASTFile, ")");
		break;

	case OP_QUESTION:

		fprintf(ASTFile, "(%s ", opname);
		pos += strlen(opname) + 2;;
		DumpExpression(expr->kids[0], pos);
		LeftAlign(ASTFile, pos);
		DumpExpression(expr->kids[1]->kids[0], pos);
		LeftAlign(ASTFile, pos);
		DumpExpression(expr->kids[1]->kids[1], pos);
		fprintf(ASTFile, ")");
		break;

	case OP_POSTINC:
	case OP_POSTDEC:

		DumpExpression(expr->kids[0], pos);
		fprintf(ASTFile, "%s", opname);
		break;

	case OP_ID:

		fprintf(ASTFile, "%s", ((Symbol)expr->val.p)->name);
		break;

	case OP_STR:
		{
			String str = ((Symbol)expr->val.p)->val.p;
			int i;

			if (expr->ty->categ != CHAR)
				fprintf(ASTFile, "L");

			fprintf(ASTFile, "\"");

			for (i = 0; i < str->len; ++i)
			{
				if (isprint(str->chs[i]))
					fprintf(ASTFile, "%c", str->chs[i]);
				else
					fprintf(ASTFile, "\\x%x", str->chs[i]);
			}

			fprintf(ASTFile, "\"");
		}
		break;

	case OP_CONST:
		{
			int categ = expr->ty->categ;

			if (categ == INT || categ == LONG || categ == LONGLONG)
			{
				fprintf(ASTFile, "%d", expr->val.i[0]);
			}
			else if (categ == UINT || categ == ULONG || categ == ULONGLONG || categ == POINTER)
			{
				fprintf(ASTFile, "%u", expr->val.i[0]);
			}
			else if (categ == FLOAT)
			{
				fprintf(ASTFile, "%g", expr->val.f);
			}
			else
			{
				fprintf(ASTFile, "%g", expr->val.d);
			}
		}
		break;

	default:
		fprintf(ASTFile, "ERR");
		break;
	}

}

static void DumpStatement(AstStatement stmt, int pos)
{
	switch (stmt->kind)
	{
	case NK_ExpressionStatement:

		DumpExpression(AsExpr(stmt)->expr, pos);
		break;

	case NK_LabelStatement:

		fprintf(ASTFile, "(label %s:\n", AsLabel(stmt)->id);
		LeftAlign(ASTFile, pos + 2);
		DumpStatement(AsLabel(stmt)->stmt, pos + 2);
		LeftAlign(ASTFile, pos);
		fprintf(ASTFile, "end-label)");
		break;

	case NK_CaseStatement:

		fprintf(ASTFile, "(case  ");
		DumpExpression(AsCase(stmt)->expr, pos + 7);
		LeftAlign(ASTFile, pos + 2);
		DumpStatement(AsCase(stmt)->stmt, pos + 2);
		LeftAlign(ASTFile, pos);
		fprintf(ASTFile, "end-case)");
		break;

	case NK_DefaultStatement:

		fprintf(ASTFile, "(default  ");
		DumpStatement(AsDef(stmt)->stmt, pos + 10);
		LeftAlign(ASTFile, pos);
		fprintf(ASTFile, "end-default)");
		break;

	case NK_IfStatement:
/*****************************************
 (if  (<= n
		  2)
   (then  
	 {
	   (ret 1)
	 }
   end-then)
   (else  
	 {
	   (ret (+ (call f
					 (- n
						1))
			   (call f
					 (- n
						2))))
	 }
   end-else)
 end-if)

 *****************************************/
		fprintf(ASTFile, "(if  ");
		DumpExpression(AsIf(stmt)->expr, pos + 5);
		LeftAlign(ASTFile, pos + 2);
		fprintf(ASTFile, "(then  ");
		LeftAlign(ASTFile, pos + 4);
		DumpStatement(AsIf(stmt)->thenStmt, pos + 4);
		LeftAlign(ASTFile, pos + 2);
		fprintf(ASTFile, "end-then)");
		if (AsIf(stmt)->elseStmt != NULL)
		{
			LeftAlign(ASTFile, pos + 2);
			fprintf(ASTFile, "(else  ");
			LeftAlign(ASTFile, pos + 4);
			DumpStatement(AsIf(stmt)->elseStmt, pos + 4);
			LeftAlign(ASTFile, pos + 2);
			fprintf(ASTFile, "end-else)");
		}
		LeftAlign(ASTFile, pos);
		fprintf(ASTFile, "end-if)");
		break;

	case NK_SwitchStatement:

		fprintf(ASTFile, "(switch ");
		DumpExpression(AsSwitch(stmt)->expr, pos + 9);
		LeftAlign(ASTFile, pos + 2);
		DumpStatement(AsSwitch(stmt)->stmt, pos + 2);
		LeftAlign(ASTFile, pos);
		fprintf(ASTFile, "end-switch)");
		break;

	case NK_WhileStatement:
/*************************************
 (while  (< i
			5)
   {
	 (call printf
		   str0,
		   (call f
				 i))
   }
 end-while)

 *************************************/
		fprintf(ASTFile, "(while  ");
		DumpExpression(AsLoop(stmt)->expr, pos + 8);
		LeftAlign(ASTFile, pos + 2);
		DumpStatement(AsLoop(stmt)->stmt, pos + 2);
		LeftAlign(ASTFile, pos);
		fprintf(ASTFile, "end-while)");
		break;

	case NK_DoStatement:

		fprintf(ASTFile, "(do  ");
		DumpExpression(AsLoop(stmt)->expr, pos + 5);
		LeftAlign(ASTFile, pos + 2);
		DumpStatement(AsLoop(stmt)->stmt, pos + 2);
		LeftAlign(ASTFile, pos);
		fprintf(ASTFile, ")");
		break;

	case NK_ForStatement:

		fprintf(ASTFile, "(for  ");
		DumpExpression(AsFor(stmt)->initExpr, pos + 6);
		LeftAlign(ASTFile, pos + 6);
		DumpExpression(AsFor(stmt)->expr, pos + 6);
		LeftAlign(ASTFile, pos + 6);
		DumpExpression(AsFor(stmt)->incrExpr, pos + 6);
		LeftAlign(ASTFile, pos + 2);
		DumpStatement(AsFor(stmt)->stmt, pos + 2);
		LeftAlign(ASTFile, pos);
		fprintf(ASTFile, "end-for)");
		break;

	case NK_GotoStatement:

		fprintf(ASTFile, "(goto %s)", AsGoto(stmt)->id);
		break;

	case NK_ContinueStatement:

		fprintf(ASTFile, "(continue)");
		break;

	case NK_BreakStatement:

		fprintf(ASTFile, "(break)");
		break;

	case NK_ReturnStatement:
	//   (ret 0)
		fprintf(ASTFile, "(ret ");
		DumpExpression(AsRet(stmt)->expr, pos + 5);
		fprintf(ASTFile, ")");
		break;

	case NK_CompoundStatement:
		{
			AstNode p = ((AstCompoundStatement)stmt)->stmts;

			fprintf(ASTFile, "{");
			while (p != NULL)
			{
				LeftAlign(ASTFile, pos + 2);
				DumpStatement((AstStatement)p, pos + 2);
				if (p->next != NULL)
					fprintf(ASTFile, "\n");
				p = p->next;
			}
			LeftAlign(ASTFile, pos);
			fprintf(ASTFile, "}");
			break;
		}

	default:
		assert(0);
	}
}

static void DumpFunction(AstFunction func)
{
	fprintf(ASTFile, "function %s\n", func->fdec->dec->id);
	DumpStatement(func->stmt, 0);
	fprintf(ASTFile, "\n\n");
}

void DumpTranslationUnit(AstTranslationUnit transUnit)
{
	AstNode p;

	ASTFile = CreateOutput(Input.filename, ".ast");

	p = transUnit->extDecls;
	while (p)
	{
		if (p->kind == NK_Function)
		{
			DumpFunction((AstFunction)p);
		}
		p = p->next;
	}
	fclose(ASTFile);
}
