#Auto-Genereated by SE352
.data
	.input_fmtstr:	.string	"%d"
	.output_fmtstr:	.string	"%d\012"
	.comm	year,4
	.comm	month,4
	.comm	day,4

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
.globl  SQAstore
SQAstore:
	movl 12(%esp), %eax
	movl 4(%esp), %ecx
	movl 8(%esp), %edx
	movl %eax, (%ecx,%edx)
	ret

.text
.globl  SQAload
SQAload:
	movl 4(%esp), %edx
	movl 8(%esp), %eax
	movl (%edx,%eax), %eax
	ret

.text
.globl	f
f:
	pushl %ebp
	pushl %ebx
	pushl %esi
	pushl %edi
	movl %esp, %ebp
	subl $124, %esp
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
	subl $124, %esp
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
.globl	printStrs
printStrs:
	pushl %ebp
	pushl %ebx
	pushl %esi
	pushl %edi
	movl %esp, %ebp
	subl $124, %esp
	movl $0, %eax
	movl %eax, -4(%ebp)
	movl -4(%ebp), %eax
	imull $4, %eax
	movl %eax, -16(%ebp)
	pushl -16(%ebp)
	pushl 20(%ebp)
	call SQAload
	addl $8, %esp
	movl %eax, -12(%ebp)
	movl -12(%ebp), %eax
	movl %eax, -8(%ebp)
Label_4:
	cmpl $0, -8(%ebp)
	je Label_5
	pushl -8(%ebp)
	call puts
	addl $4, %esp
	movl %eax, -20(%ebp)
	movl -4(%ebp), %eax
	addl $1, %eax
	movl %eax, -24(%ebp)
	movl -24(%ebp), %eax
	movl %eax, -4(%ebp)
	movl -4(%ebp), %eax
	imull $4, %eax
	movl %eax, -32(%ebp)
	pushl -32(%ebp)
	pushl 20(%ebp)
	call SQAload
	addl $8, %esp
	movl %eax, -28(%ebp)
	movl -28(%ebp), %eax
	movl %eax, -8(%ebp)
	jmp Label_4
Label_5:
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
	subl $124, %esp
	pushl $40
	call malloc
	addl $4, %esp
	movl %eax, -20(%ebp)
	movl -20(%ebp), %eax
	movl %eax, -8(%ebp)
	movl $0, %eax
	movl %eax, -12(%ebp)
Label_6:
	pushl $10
	pushl -12(%ebp)
	call SQAless
	addl $8, %esp
	movl %eax, -24(%ebp)
	cmpl $0, -24(%ebp)
	je Label_7
	movl -12(%ebp), %eax
	imull $4, %eax
	movl %eax, -32(%ebp)
	pushl -12(%ebp)
	pushl -32(%ebp)
	pushl -8(%ebp)
	call SQAstore
	addl $12, %esp
	movl %eax, -28(%ebp)
	movl -12(%ebp), %eax
	addl $1, %eax
	movl %eax, -36(%ebp)
	movl -36(%ebp), %eax
	movl %eax, -12(%ebp)
	jmp Label_6
Label_7:
	movl $0, %eax
	movl %eax, -12(%ebp)
Label_8:
	pushl $10
	pushl -12(%ebp)
	call SQAless
	addl $8, %esp
	movl %eax, -40(%ebp)
	cmpl $0, -40(%ebp)
	je Label_9
	movl -12(%ebp), %eax
	imull $4, %eax
	movl %eax, -48(%ebp)
	pushl -48(%ebp)
	pushl -8(%ebp)
	call SQAload
	addl $8, %esp
	movl %eax, -44(%ebp)
	movl -44(%ebp), %eax
	movl %eax, -16(%ebp)
	leal .output_fmtstr, %eax
	pushl -16(%ebp)
	pushl %eax
	call printf
	addl $8, %esp
	movl -12(%ebp), %eax
	addl $1, %eax
	movl %eax, -52(%ebp)
	movl -52(%ebp), %eax
	movl %eax, -12(%ebp)
	jmp Label_8
Label_9:
	movl $97, %eax
	movl %eax, -16(%ebp)
	movl $0, %eax
	movl %eax, -12(%ebp)
Label_10:
	pushl $26
	pushl -12(%ebp)
	call SQAless
	addl $8, %esp
	movl %eax, -56(%ebp)
	cmpl $0, -56(%ebp)
	je Label_11
	movl -16(%ebp), %eax
	addl -12(%ebp), %eax
	movl %eax, -64(%ebp)
	pushl -64(%ebp)
	pushl -12(%ebp)
	pushl -8(%ebp)
	call SQAstore
	addl $12, %esp
	movl %eax, -60(%ebp)
	movl -12(%ebp), %eax
	addl $1, %eax
	movl %eax, -68(%ebp)
	movl -68(%ebp), %eax
	movl %eax, -12(%ebp)
	jmp Label_10
Label_11:
	pushl $0
	pushl $26
	pushl -8(%ebp)
	call SQAstore
	addl $12, %esp
	movl %eax, -72(%ebp)
	pushl -8(%ebp)
	call puts
	addl $4, %esp
	movl %eax, -76(%ebp)
	pushl -8(%ebp)
	call free
	addl $4, %esp
	movl %eax, -80(%ebp)
	pushl 24(%ebp)
	call printStrs
	addl $4, %esp
	movl %eax, -84(%ebp)
	pushl 28(%ebp)
	call printStrs
	addl $4, %esp
	movl %eax, -88(%ebp)
	movl $2022, %eax
	movl %eax, year
	movl $11, %eax
	movl %eax, month
	movl $14, %eax
	movl %eax, day
	pushl $100
	call f
	addl $4, %esp
	movl %eax, -96(%ebp)
	movl -96(%ebp), %eax
	movl %eax, -92(%ebp)
	leal .output_fmtstr, %eax
	pushl -92(%ebp)
	pushl %eax
	call printf
	addl $8, %esp
	pushl $1
	pushl $100
	pushl $1
	call g
	addl $12, %esp
	movl %eax, -100(%ebp)
	movl -100(%ebp), %eax
	movl %eax, -92(%ebp)
	leal .output_fmtstr, %eax
	pushl -92(%ebp)
	pushl %eax
	call printf
	addl $8, %esp
	leal .output_fmtstr, %eax
	pushl year
	pushl %eax
	call printf
	addl $8, %esp
	leal .output_fmtstr, %eax
	pushl month
	pushl %eax
	call printf
	addl $8, %esp
	leal .output_fmtstr, %eax
	pushl day
	pushl %eax
	call printf
	addl $8, %esp
	leal .output_fmtstr, %eax
	pushl 20(%ebp)
	pushl %eax
	call printf
	addl $8, %esp
	call fork
	movl %eax, -108(%ebp)
	movl -108(%ebp), %eax
	movl %eax, -104(%ebp)
	pushl $0
	pushl -104(%ebp)
	call SQAless
	addl $8, %esp
	movl %eax, -112(%ebp)
	cmpl $0, -112(%ebp)
	je Label_12
	leal .output_fmtstr, %eax
	pushl -104(%ebp)
	pushl %eax
	call printf
	addl $8, %esp
	jmp Label_15
Label_12:
	pushl $0
	pushl -104(%ebp)
	call SQAequal
	addl $8, %esp
	movl %eax, -116(%ebp)
	cmpl $0, -116(%ebp)
	je Label_13
	leal .output_fmtstr, %eax
	pushl -104(%ebp)
	pushl %eax
	call printf
	addl $8, %esp
	jmp Label_14
Label_13:
	pushl $0
	call wait
	addl $4, %esp
	movl %eax, -120(%ebp)
	leal .output_fmtstr, %eax
	pushl -104(%ebp)
	pushl %eax
	call printf
	addl $8, %esp
Label_14:
Label_15:
	movl $0, %eax
	movl %ebp, %esp
	popl %edi
	popl %esi
	popl %ebx
	popl %ebp
	ret
