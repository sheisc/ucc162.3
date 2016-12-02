	.file	"hello.c"
	.text
	.globl	f
	.type	f, @function
f:
	pushl	%ebp
	movl	%esp, %ebp
	popl	%ebp
	ret
	.size	f, .-f
	.globl	main
	.type	main, @function
main:
	pushl	%ebp
	movl	%esp, %ebp
	movl	$0, %eax
	popl	%ebp
	ret
	.size	main, .-main
	.ident	"GCC: (Ubuntu/Linaro 4.6.3-1ubuntu5) 4.6.3"
	.section	.note.GNU-stack,"",@progbits
