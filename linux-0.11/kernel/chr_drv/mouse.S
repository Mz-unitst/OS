#include <linux/config.h>

.globl mouse_interrupt
mouse_interrupt:
	
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	push %ds
	//push %es

//prepare for call readmouse ,ds is in kernel
//call tt
	movl $0x10,%eax
	mov %ax,%ds 
	//mov %ax,%es

// eax store mouse data ,put into stack for readmouse's params	
	

xor %eax,%eax
	inb $0x60,%al	
	pushl %eax
	call readmouse
	addl $4,%esp
xor %eax,%eax
	inb $0x60,%al	
	pushl %eax
	call readmouse
	addl $4,%esp

xor %eax,%eax
	inb $0x60,%al	
	pushl %eax
	call readmouse
	addl $4,%esp
xor %eax,%eax
	inb $0x60,%al	
	pushl %eax
	call readmouse
	addl $4,%esp







	// 3*EOI
	movb $0x20,%al
	outb %al,$0xA0 
	outb %al,$0x20

	//pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	iret








