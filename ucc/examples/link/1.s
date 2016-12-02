	.file	"1.c"
	.text
	.globl	add
	.type	add, @function
add:
.LFB0:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	subl	$16, %esp
	movl	8(%ebp), %eax
	movl	%eax, -8(%ebp)
	movl	12(%ebp), %eax
	movl	%eax, -4(%ebp)
	movl	16(%ebp), %eax
	movl	%eax, -16(%ebp)
	movl	20(%ebp), %eax
	movl	%eax, -12(%ebp)
	fldl	-8(%ebp)
	faddl	-16(%ebp)
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE0:
	.size	add, .-add
	.globl	fpVal
	.data
	.align 8
	.type	fpVal, @object
	.size	fpVal, 8
fpVal:
	.long	0
	.long	1077149696
	.section	.rodata
.LC1:
	.string	"%f: %08x %8x \n"
.LC2:
	.string	"%x \n"
.LC4:
	.string	"%f \n"
	.text
	.globl	main
	.type	main, @function
main:
.LFB1:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	andl	$-16, %esp
	subl	$64, %esp
	fldl	fpVal
	fstpl	48(%esp)
	leal	48(%esp), %eax
	movl	%eax, 60(%esp)
	movl	$300, abc
	movl	60(%esp), %eax
	addl	$4, %eax
	movl	(%eax), %ecx
	movl	60(%esp), %eax
	movl	(%eax), %edx
	fldl	48(%esp)
	movl	$.LC1, %eax
	movl	%ecx, 16(%esp)
	movl	%edx, 12(%esp)
	fstpl	4(%esp)
	movl	%eax, (%esp)
	call	printf
	fldl	fpVal
	fstpl	(%esp)
	call	fabs250
	movl	$.LC2, %edx
	movl	%eax, 4(%esp)
	movl	%edx, (%esp)
	call	printf
	fldl	fpVal
	fstpl	(%esp)
	call	fabs250
	movl	%eax, 44(%esp)
	fildl	44(%esp)
	fldl	.LC3
	fstpl	8(%esp)
	fstpl	(%esp)
	call	add
	movl	$.LC4, %eax
	fstpl	4(%esp)
	movl	%eax, (%esp)
	call	printf
	movl	$0, %eax
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE1:
	.size	main, .-main
	.section	.rodata
	.align 8
.LC3:
	.long	0
	.long	1074266112
	.ident	"GCC: (Ubuntu/Linaro 4.6.3-1ubuntu5) 4.6.3"
	.section	.note.GNU-stack,"",@progbits
