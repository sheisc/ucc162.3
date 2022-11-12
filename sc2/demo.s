#Auto-Genereated by SC2
.data
	.input_fmtstr:	.string	"%d"
	.output_fmtstr:	.string	"%d\012"
	.comm	year,4

.text
.globl  SQAless
SQAless:
	movl 8(%esp), %eax
	cmpl %eax, 4(%esp)
	setl    %al
	movzbl  %al, %eax
	ret

.text
.globl  SQAlarger
SQAlarger:
	movl 8(%esp), %eax
	cmpl %eax, 4(%esp)
	setg    %al
	movzbl  %al, %eax
	ret

.text
.globl  SQAequal
SQAequal:
	movl 8(%esp), %eax
	cmpl %eax, 4(%esp)
	sete    %al
	movzbl  %al, %eax
	ret

.text
.globl	f
f:
	pushl %ebp
	pushl %ebx
	pushl %esi
	pushl %edi
	movl %esp, %ebp
	subl $16, %esp
	cmpl $0, 20(%ebp)
	je Label_0
	movl 20(%ebp), %eax
	subl $1, %eax
	movl %eax, -8(%ebp)
	pushl -8(%ebp)
	call f
	addl $4, %esp
	movl %eax, -4(%ebp)
	movl -4(%ebp), %eax
	addl 20(%ebp), %eax
	movl %eax, -16(%ebp)
	movl -16(%ebp), %eax
	jmp Label_1
Label_0:
	movl 20(%ebp), %eax
Label_1:
	movl %ebp, %esp
	popl %edi
	popl %esi
	popl %ebx
	popl %ebp
	ret

.text
.globl	g
g:
	pushl %ebp
	pushl %ebx
	pushl %esi
	pushl %edi
	movl %esp, %ebp
	subl $16, %esp
	movl $0, %eax
	movl %eax, -4(%ebp)
Label_2:
	movl 24(%ebp), %eax
	addl $1, %eax
	movl %eax, -12(%ebp)
	pushl -12(%ebp)
	pushl 20(%ebp)
	call SQAless
	addl $8, %esp
	movl %eax, -8(%ebp)
	cmpl $0, -8(%ebp)
	je Label_3
	movl -4(%ebp), %eax
	addl 20(%ebp), %eax
	movl %eax, -16(%ebp)
	movl -16(%ebp), %eax
	movl %eax, -4(%ebp)
	movl 20(%ebp), %eax
	addl 28(%ebp), %eax
	movl %eax, -20(%ebp)
	movl -20(%ebp), %eax
	movl %eax, 20(%ebp)
	jmp Label_2
Label_3:
	movl -4(%ebp), %eax
	movl %ebp, %esp
	popl %edi
	popl %esi
	popl %ebx
	popl %ebp
	ret

.text
.globl	main
main:
	pushl %ebp
	pushl %ebx
	pushl %esi
	pushl %edi
	movl %esp, %ebp
	subl $16, %esp
	movl $2022, %eax
	movl %eax, year
	pushl $100
	call f
	addl $4, %esp
	movl %eax, -8(%ebp)
	movl -8(%ebp), %eax
	movl %eax, -4(%ebp)
	leal .output_fmtstr, %eax
	pushl -4(%ebp)
	pushl %eax
	call printf
	addl $8, %esp
	pushl $1
	pushl $100
	pushl $1
	call g
	addl $12, %esp
	movl %eax, -12(%ebp)
	movl -12(%ebp), %eax
	movl %eax, -4(%ebp)
	leal .output_fmtstr, %eax
	pushl -4(%ebp)
	pushl %eax
	call printf
	addl $8, %esp
	leal .output_fmtstr, %eax
	pushl year
	pushl %eax
	call printf
	addl $8, %esp
	leal .output_fmtstr, %eax
	pushl 20(%ebp)
	pushl %eax
	call printf
	addl $8, %esp
	movl $0, %eax
	movl %ebp, %esp
	popl %edi
	popl %esi
	popl %ebx
	popl %ebp
	ret
