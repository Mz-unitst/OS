# OS课设：鼠标驱动和简单的图形接口实现

## 1.鼠标驱动

### 捕获鼠标中断

1. kernel/chr_drv/console.c

   ```c
   extern void mouse_interrupt(void);
   
   set_trap_gate(0x2c,&mouse_interrupt);
   ```

2. kernel/chr_drv/keyboard.S

​		从鼠标设备的数据寄存器中将鼠标信息读取出来（通过0x60端口)，通过inb命令将该数据寄存器中存放的1B信息读入到AL中，之后通过readmouse函数进行具体处理。

```assembly
#include <linux/config.h>
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

```

#### 键盘控制器i8042与中断控制器8259A

1. i8042有两个I/O端口：命令端口0x64，数据端口0x60。

   1. ​	鼠标涉及到的命令：

      1. 0xA8

         允许鼠标操作

      2.  0xD4

         告诉i8042,发往0x60端口的参数是给鼠标的

      3.  0x60

         将发往0x60的参数写入i8042(0100 0111,即0x47)

   2. 设置i8042(在set_trap_gate(0x2c,&mouse_interrupt)之前)

   ```
   outb_p(0xA8,0x64); //允许鼠标操作
   outb_p(0xD4,0x64); //给 0x64 端口发送 0xD4，表示接下来给 0x60 的命令是给鼠标的
   outb_p(0xF4,0x60); //设置鼠标，允许鼠标向主机自动发送数据包
   outb_p(0x60,0x64); //给 0x64 端口发送 0x60，表示接下来给 0x60 的命令是给 i8042 的
   outb_p(0x47,0x60); //设置 i8042 寄存器，允许鼠标接口及其中断
   ```
   
   3. 绑定0x2c与鼠标请求中断IRQ12

      boot/setup.s  中8259A屏蔽了所有中断（ICW1所有位置1），应解开

      ![image-20211215185945237](C:\Users\Mz\AppData\Roaming\Typora\typora-user-images\image-20211215185945237.png)

      ```assembly
      	outb_p(inb_p(0x21)&0xF9,0x21);！合并了原来的fd和需要的fb为f9，两位均置零 11111001
      	outb_p(inb_p(0xA1)&0xEF,0xA1);
      ```
   
      0xEF 对应的二进制是 1110 1111，是将从片 IR4 的屏蔽打开。
   
      以后，鼠标中断请求会经过从片的 IR4 和主片的 IR2 到达 CPU。
   
   4. 结束中断以便接受下一次中断
   
      kernel/chr_drv/keyboard.S
   
      ```assembly
      movb $0x20,%al //8259A 操作命令字 EOI
      outb %al,$0xA0 //向 8259A 从片发送 EOI
      outb %al,$0x20 //向 8259A 主片发送 EOI
      ```

### 鼠标输入数据的解码

1.  将将鼠标输入数据 从 0x60 端口读进来，将其压栈后再调用 C 函数 readmouse

```assembly
xor %eax,%eax
inb $0x60,%al //读入鼠标数据
pushl %eax
call readmouse
addl $4,%esp
```

2.  鼠标输入数据的格式

![image-20211214200624501](C:\Users\Mz\AppData\Roaming\Typora\typora-user-images\image-20211214200624501.png)

| 二进制位 | 含义与作用                                              |
| -------- | ------------------------------------------------------- |
| 7        | Y 溢出标志位，1 表示 Y 位移量溢出                       |
| 6        | X 溢出标志位，1 表示 X 位移量溢出                       |
| 5        | Y 符号标志位，1 表示 Y 位移量为负（向上为正、向下为负） |
| 4        | X 符号标志位，1 表示 X 位移量为负（向左为负、向右为正） |
| 3        | 保留位，总为 1                                          |
| 2        | 中键按下标志位，1 表示按下了中键                        |
| 1        | 右键按下标志位，1 表示按下了右键                        |
| 0        | 左键按下标志位，1 表示按下了左键                        |

3. 二位鼠标数据解码函数readmouse

   kernel/chr_drc/tty_io.c (不能放在console.c 中，会因为释放内核页表报错)

   ```
   static unsigned char mouse_input_count = 0; //用来记录是鼠标输入的第几个字节的全局变量
   static unsigned char mouse_left_down; //用来记录鼠标左键是否按下
   static unsigned char mouse_right_down; //用来记录鼠标右键是否按下
   static unsigned char mouse_left_move; //用来记录鼠标是否向左移动
   static unsigned char mouse_down_move;//用来记录鼠标是否向下移动
   
   static int mouse_x_position; //用来记录鼠标的 x 轴位置
   static int mouse_y_position;//用来记录鼠标的 y 轴位置
   //...
   //...
   void readmouse(int mousecode)
   {
   	if(mousecode == 0xFA) //0xFA 是 i8042 鼠标命令的成功响应的 ACK 字节，参见习题
   	{
   	mouse_input_count = 1;
   	return 0;
   	}
   switch(mouse_input_count)
     {
   case 1: //处理第一个字节，取出左、右键是否按下等信息
   //与运算过滤了无关位
   	mouse_left_down = (mousecode & 0x1) == 0x1;
   	mouse_right_down = (mousecode & 0x2) == 0x2;
   	mouse_left_move = (mousecode & 0x10) == 0x10;
   	mosue_down_move = (mousecode & 0x20) == 0x20;
   	mouse_input_count++；
   	break;
   case 2: //处理第二个字节，计算鼠标在 X 轴上的位置
   	if(mouse_left_move)
   	mouse_x_position += (int)(0xFFFFFF00 | mousecode); //此时mousecode 是一个 8 位负数的补码表示，要将其变成 32 位就需要在前面填充 1
   	//max limit
   	//此处的100应更改，按照分辨率更改为合适的数值
   	if(mouse_x_position>100) mouse_x_position=100;
   	if(mouse_x_position < 0) mouse_x_position = 0;
   	break;
   case 3: //处理第3个字节，计算鼠标在 Y 轴上的位置
   	if(mouse_down_move)
   	mouse_y_position += (int)(0xFFFFFF00 | mousecode);
   	if(mouse_y_position > 100) mouse_y_position = 100;
   	if(mouse_y_position < 0) mouse_y_position = 0;
   	break;
     }
   
   }
   ```

   ### 阶段成果展示
   
   ![image-20211215194918740](C:\Users\Mz\AppData\Roaming\Typora\typora-user-images\image-20211215194918740.png)
   
   

## 2.显示器的图形显示模式
