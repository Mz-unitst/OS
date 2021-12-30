#include <linux/config.h>

.text
.globl mouse_interrupt
mouse_interrupt:
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	push %ds

	movl $0x10,%eax
	mov %ax,%ds
	xor %eax,%eax
	inb $0x60,%al

	pushl %eax
	call readmouse

	addl $4,%esp

	movb $0x20,%al
	outb %al,$0xA0 
	outb %al,$0x20

	pop %ds
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	iret


! Print some inane message

	mov	ah,#0x03		! read cursor pos
	xor	bh,bh
	int	0x10
	
	mov	cx,#12
	mov	bx,#0x0070		! page 0, attribute 7 (normal)
	mov	bp,#msg11
	mov	ax,#0x1301		! write string, move cursor
	int	0x10


msg11:
	.byte 13,10
	.ascii "Mouse!"
	.byte 13,10,13,10

!geng gai shunxu hou
mov $0x03,%ah		
	xor	%bh,%bh
	int	$0x10
	
	
	mov $12,%cx
	mov	$0x0070,%bx		
	mov	$msg111,%bp
	mov	$0x1301,%ax		
	int	$0x10

