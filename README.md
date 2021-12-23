# OS课设：鼠标驱动和简单的图形接口实现

## 1.鼠标驱动

### 捕获鼠标中断

1. kernel/chr_drv/console.c添加中断，0.11中的unistd.h中也要加宏

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

   ![8259A](https://user-images.githubusercontent.com/56508903/147203488-fcd87cc6-cf9a-47c8-9030-f55d70b48616.png)

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

![mousedata](https://user-images.githubusercontent.com/56508903/147203522-8edc9247-85ca-4f4c-a28a-a7ba10f7197d.png)
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
   
![step1](https://user-images.githubusercontent.com/56508903/147203550-63675d36-958b-4b0a-af82-69778565c120.png)
   

## 2.显示器的图形显示模式

1. 启用VGA图形模式

   工作模式：![VGA](https://user-images.githubusercontent.com/56508903/147203576-a5ec21e6-2125-4450-82ac-e03db69923fe.png)

   

2. 建立像素点阵与显存之间的映射 

   启用mode 0x13,即线性256色模式，分辨率为320*200，每个像素用一个字节表示，共64000个像素点，64000B，略小于64KB

   拼装器工作模式：

   ![shift256](https://user-images.githubusercontent.com/56508903/147203592-fcb83669-7964-4ebe-b1fa-72bce6ba9cd5.png)

   r 将 4 个显存 plane 中 同一个地址处的 4 个字节按照每 4 位一组进行左移，共移出了 8 个字节，显卡会取出Byte1,3,5,7并将这四个 字节依次扫描到屏幕上形成四个像素

   3. 设置屏幕分辨率

      通过CRT控制器完成，工作原理：

      0）从左到右，从上到下，通过扫描线扫描到显示器上

      1）scanline counter用来计数是第几根扫描线，从0开始则扫描输出第一行像素

      2）在生成每一行像素时，horizontal counter开始工作，其经过Display Enable Skew设定的距离后开始输出像素，当其增加到End Horizontal Display后CRT停止从显存中读取数据。

      3）当一行输出完成后，horizontal counter置0，而scanline counter是否增加取决于SLDIV,默认每隔两个扫描周期后加一

      4）扫描线增加1后，显存的shift地址向后移动一行对应的字节数Offset Register * MemoryAddresses *2并重复步骤2

      5）当扫描线增加到Vertical Display End后不再输出

      原理图：

      ![image-20211223145557292](https://user-images.githubusercontent.com/56508903/147203625-c038f62b-6d55-4d5c-a8e9-0ead9e8ffd00.png)

      

   4. 开始绘制屏幕

      只需将起始地址设为0xA0000即可，现在VGA屏幕中的图像对应内存如下：

      ![print_screen](https://user-images.githubusercontent.com/56508903/147203682-12f9265a-e905-4d2b-8633-10eb16ccfa95.png)

      

   5.  代码如下：

   图形模式初始化通过系统调用sys_init_graphicis实现，代码如下：

   ```c
   #include <linux/kernel.h>
   #include<asm/io.h>
   #define memstart 0xA0000+1360
   #define memsize 64000
   #define cursor_side 3
   #define width 320
   #define height 200
   int sys_init_graphics()
   {
       char *p;
       int i,j,x,y;
       outb(0x05,0x3CE);
       outb(0x40,0x3CF);/* shift256=1,256色，取出方式为shift*/
       outb(0x06,0x3CE);
       outb(0x05,0x3CF);/*0101 ,开启图形模式，起始地址0xA0000*/
       outb(0x04,0x3C4);
       outb(0x08,0x3C5);/*1000 四个显存片级联*/
   
   
       outb(0x01,0x3D4);
       outb(0x4F,0x3D5);/* end horizontal display=79， 320 *2 /8 -1=79*/
       outb(0x03,0x3D4);
       outb(0x82,0x3D5);/*display enable skew=0,经过0距离开始绘制*/
   
       outb(0x07,0x3D4);
       outb(0x1F,0x3D5);/*vertical display end No8,9 bit=1,0*/
       outb(0x12,0x3D4);
       outb(0x8F,0x3D5);/*vertical display end low 7b =0x8F*/
       outb(0x17,0x3D4);
       outb(0xA3,0x3D5);/*SLDIV=1 ,scanline clock/2*/
   
   
   
       outb(0x14,0x3D4);
       outb(0x40,0x3D5);/*DW=1，双字模式 Memory Addresses Size=4*/
       outb(0x13,0x3D4);
       outb(0x28,0x3D5);/*Offset=40,320/2/4=40*/
   
       outb(0x0C,0x3D4);/**/
       outb(0x00,0x3D5);/**/
       outb(0x0D,0x3D4);/**/
       outb(0x00,0x3D5);/*Start Address=0xA0000*/
   
       p=memstart;
       for(i=0;i<memsize;i++) *p++=3;
   
   
       x=20;
       y=10;
       for(i=x-cursor_side;i<=x+cursor_side;i++)
           for(j=y-cursor_side;j<=y+cursor_side;j++){
               p=(char *) memstart+j*width+i;
               *p=12;
           }
   
       return 0;
   }
   ```

   6. 阶段成果展示：

      ![step2](https://user-images.githubusercontent.com/56508903/147203698-8b28bb06-65f2-44fc-a0a6-05e46a831b25.png)

2. 消息驱动框架
