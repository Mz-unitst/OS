# OS课设：鼠标驱动和简单的图形接口实现

## 1.鼠标驱动

#### 捕获鼠标中断

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

    ![8259A](https://user-images.githubusercontent.com/56508903/148795704-803433d5-b6ab-4ec4-88a7-dea55d12ba2e.png)

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

#### 鼠标输入数据的解码

1.  将将鼠标输入数据 从 0x60 端口读进来，将其压栈后再调用 C 函数 readmouse

```assembly
xor %eax,%eax
inb $0x60,%al //读入鼠标数据
pushl %eax
call readmouse
addl $4,%esp
```

2.  鼠标输入数据的格式

![mousedata](https://user-images.githubusercontent.com/56508903/148795081-392d7798-0a0d-42e4-8660-44b919b3e6ca.png)

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

   ```c
   static unsigned char mouse_input_count = 0; //用来记录是鼠标输入的第几个字节的全局变量
   static unsigned char mouse_left_down; //用来记录鼠标左键是否按下
   static unsigned char mouse_right_down; //用来记录鼠标右键是否按下
   static unsigned char mouse_left_move; //用来记录鼠标是否向左移动
   static unsigned char mouse_down_move;//用来记录鼠标是否向下移动
   
   static int mouse_x_position; //用来记录鼠标的 x 轴位置
   static int mouse_y_position;//用来记录鼠标的 y 轴位置
   static int  fcreate=0;
   int cnt=0;
   //struct message *headd;
   //struct message *cur;
   //...
   //...
   //...
   void readmouse(int mousecode)
   {
   if(fcreate==0)
   {
   	fcreate=1;
   cnt=33;
   //mouse_input_count=0;
   	//jumpp=0;
   }
   //jumpp=mousecode;
   
   //cli();
   if(mousecode==0xFA || mouse_input_count>=4 )
   {
   //0xFA 是 i8042 鼠标命令的成功响应的 ACK 字节，应舍弃，并设置重置条件防止错位
   	mouse_input_count=1;
   //jumpp=600;
   //jumpp=cnt;
   //jumpp+=3;
   //jumpp++;
   //post_message();
   return ;
   }
   //jumpp=0;
   if(cnt!=mousecode)
   {
   //jumpp=mousecode;
   cnt=mousecode;
   }
   //jumpp=mousecode;
   switch(mouse_input_count)
   {
   case 1:
   //cuowei sheqi
   //if( (mousecode & 0xc8) == 0x08 )
   //if((mousecode & 0xc8) == 0x08)
   {
   //与运算过滤了无关位
   	mouse_left_down=(mousecode &0x01) ==0x01;
   	mouse_right_down=(mousecode &0x02)==0x02;
   	mouse_left_move=(mousecode & 0x10)==0x10;
   	mouse_down_move=(mousecode & 0x20)==0x20;
   	mouse_input_count++;
   //jumpp=mouse_left_move;
   	//jumpp=mousecode;  
   	//jumpp+=1;
   //jumpp++;
   }
   //return 0;
    
   	//jumpp+=1;
   	//jumpp=mouse_left_down;
   //jumpp=11;
   //jumpp=mousecode;
   // you yan chi ,zai 0xfa chu wu yan chi
   	if(mouse_left_down==1 && mouse_left_move==0 && mouse_down_move==0){
   	post_message();
   	//jumpp+=10;
   	}
   //return;
   //mouse_input_count++;
   	break;
   case 2:
   //处理第二个字节，计算鼠标在 X 轴上的位置
   //此时mousecode 是一个 8 位负数的补码表示，要将其变成 32 位就需要在前面填充 1
   	if(mouse_left_move) mouse_x_position +=(int)(0xFFFFFF00|mousecode);
   	if(mouse_x_position>100) mouse_x_position=100;
   	if(mouse_x_position<0) mouse_x_position=10;
   //	jumpp=200;
   //jumpp=mousecode;
   //jumpp=22;
   //jumpp+=10;
   //return ;
   mouse_input_count++;
   	break;
   case 3:
   //处理第3个字节，计算鼠标在 Y 轴上的位置
   //           jumpp=33;
   	if(mouse_down_move) mouse_y_position +=(int)(0xFFFFFF00|mousecode);
   	if(mouse_y_position>200) mouse_y_position=200;
   	if(mouse_y_position<0) mouse_y_position=0;
   //157 
   //jumpp=mousecode;
   	//mouse_input_count++;
   //jumpp+=10;
   //jumpp++;
   	mouse_input_count++;
   //jumpp-=10;
   //jumpp=33;
   //jumpp++;
   	break;
   case 4:
   // gun lun ,mouse3
   //jumpp++;
   //jumpp=44;
   // zan shi bu xie
   //jumpp-=10;
   jumpp=jumpp;
   break;
   }
   //jumpp=mouse_left_down;
   //jumpp=mouse_input_count;
   //jumpp=mouse_x_position;
   //sti();
   }
   ```
   
   
   
   #### 阶段成果展示
   
   ![step1](https://user-images.githubusercontent.com/56508903/148795144-3674aac6-573f-4797-b2c9-0bd79f71aca4.png)
   

## 2.显示器的图形显示模式

#### 1.启用VGA图形模式

工作模式：![VGA](https://user-images.githubusercontent.com/56508903/148795168-529d5215-bb43-4fae-9798-052c200fa09c.png)



#### 2.建立像素点阵与显存之间的映射 

启用mode 0x13,即线性256色模式，分辨率为320*200，每个像素用一个字节表示，共64000个像素点，64000B，略小于64KB

拼装器工作模式：

![shift256](https://user-images.githubusercontent.com/56508903/148795185-d0765be8-9714-418a-a7d2-6c2a65441107.png)
 将 4 个显存 plane 中 同一个地址处的 4 个字节按照每 4 位一组进行左移，共移出了 8 个字节，显卡会取出Byte1,3,5,7并将这四个 字节依次扫描到屏幕上形成四个像素

#### 3.设置屏幕分辨率

通过CRT控制器完成，工作原理：

0）从左到右，从上到下，通过扫描线扫描到显示器上

1）scanline counter用来计数是第几根扫描线，从0开始则扫描输出第一行像素

2）在生成每一行像素时，horizontal counter开始工作，其经过Display Enable Skew设定的距离后开始输出像素，当其增加到End Horizontal Display后CRT停止从显存中读取数据。

3）当一行输出完成后，horizontal counter置0，而scanline counter是否增加取决于SLDIV,默认每隔两个扫描周期后加一

4）扫描线增加1后，显存的shift地址向后移动一行对应的字节数Offset Register * MemoryAddresses *2并重复步骤2

5）当扫描线增加到Vertical Display End后不再输出

原理图：

![pricinple](https://user-images.githubusercontent.com/56508903/148795928-0e2d31d2-e485-4c9e-a257-b11760f0705a.png)

#### 4.开始绘制屏幕

只需将起始地址设为0xA0000即可，现在VGA屏幕中的图像对应内存如下：

![print_screen](https://user-images.githubusercontent.com/56508903/148795273-91265cc4-6102-464e-a45a-727bf814fd1a.png)


#### 5.代码

图形模式初始化通过系统调用sys_init_graphicis实现，代码如下：

```c
int volatile jumpp;
int ff=0;
int sys_init_graphics()
{
	int i,j,x,y;
    char *p=0xA0000;
    if(ff==0)
{
	outb(0x05,0x3CE);
    outb(0x40,0x3CF);/* shift256=1*/
    outb(0x06,0x3CE);
    outb(0x05,0x3CF);/*0101 0xA0000*/
    outb(0x04,0x3C4);
    outb(0x08,0x3C5);/*0000 jilian*/


    outb(0x01,0x3D4);
    outb(0x4F,0x3D5);/* end horizontal display=79 ??*/
    outb(0x03,0x3D4);
    outb(0x82,0x3D5);/*display enable skew=0*/

    outb(0x07,0x3D4);
    outb(0x1F,0x3D5);/*vertical display end No8,9 bit=1,0*/
    outb(0x12,0x3D4);
    outb(0x8F,0x3D5);/*vertical display end low 7b =0x8F*/
    outb(0x17,0x3D4);
    outb(0xA3,0x3D5);/*SLDIV=1 ,scanline clock/2*/



    outb(0x14,0x3D4);
    outb(0x40,0x3D5);/*DW=1*/
    outb(0x13,0x3D4);
    outb(0x28,0x3D5);/*Offset=40, 20:bian chang*/

    outb(0x0C,0x3D4);/**/
    outb(0x00,0x3D5);/**/
    outb(0x0D,0x3D4);/**/
    outb(0x00,0x3D5);/*Start Address=0xA0000*/
ff=1;
}
    
    

    p=memstart;
    for(i=0;i<memsize;i++) *p++=3;
//3-blue 4-red 12-purple

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

#### 6.阶段成果展示：

![step2](https://user-images.githubusercontent.com/56508903/148795300-abe09e93-7529-4ed0-aca0-9a24cffdfa28.png)

## 3.消息驱动框架

#### 1.原理

1. OS收集计算机各个地方的事件，如键盘按下，鼠标按下，网卡有数据包，时钟到时等

2. 操作系统负责将各个事件做成一个数据结构并将其放入系统消息队列中

3. 用户进程查看系统消息队列，当发现有消息给自己时，从中取出信息并进行处理

![queue](https://user-images.githubusercontent.com/56508903/148795333-1c2ea6fa-9e69-4c2d-a4aa-a1b467bccafe.png)
#### 2.主要工作

1. 定义消息队列数据结构

   原计划多进程共享队列，因为目前只有一个进程会使用这个调用，因此将原本计划的msg结构体（含mid,pid,next指针）简化为全局变量jumpp

   在include/linux/tty.h处定义全局变量

   ```c
   extern int volatile jumpp;
   ```

   同时在各个调用此变量的文件*.c处也要定义

   ```c
   int volatile jumpp;
   ```

   

2. 实现post_message将消息放入系统消息队列中

   位于kernel/chr_drv/ttyio.c

   ```c
   void post_message()
   {
   //	struct message *curr;
   //	curr->next=headd->next;
   //	if(msgg==NULL)return;
   //	while(curr->next!=NULL) {
   //	curr=curr->next;
   //}
   //curr->next=msgg;
   cli();
   //防止鼠标移动导致疯狂中断,jumpp起飞
   if(jumpp<=10)
   jumpp++;
   sti();
   return;
   }
   ```

   

3. 能为用户态进程提供获取系统消息队列的系统调用get_message,编写对应的内核函数sys_get_message

   1. 位于kernel/init_graphics.c

   ```c
   int sys_get_message()
   {
   	//msgg=headd;
   	//if(headd->mid!=1)return;
   	//headd=headd->next;
   	if(jumpp>0) --jumpp;
   	return jumpp;
   }
   ```

   

4. 在readmouse函数合适地方调用post_message

   位于kernel/chr_drc/tty_io.c

   ```c
   //...
   //...
   case 1:
   {
   //与运算过滤了无关位
   	mouse_left_down=(mousecode &0x01) ==0x01;
   	mouse_right_down=(mousecode &0x02)==0x02;
   	mouse_left_move=(mousecode & 0x10)==0x10;
   	mouse_down_move=(mousecode & 0x20)==0x20;
   	mouse_input_count++;
   }
   	if(mouse_left_down==1 && mouse_left_move==0 && mouse_down_move==0){
   	post_message();
   	}
   	//...
   	//...
   ```

   

5. 测试

![step3](https://user-images.githubusercontent.com/56508903/148795371-d1ec7e33-53db-4b31-bf8a-e5092be78a66.png)

![step3_code](https://user-images.githubusercontent.com/56508903/148795388-a3bea52f-a842-4db8-a09b-286c352a3aca.png)

其中m的值就是用户程序调用get_message的返回值jumpp

## 4.Flappy Bird

#### 1.思路：小鸟：一直向下移动，鼠标左键点击小鸟会向上移动一定距离；障碍物以一定速度左移，以实现整个场景运动。

#### 2.代码

1. 创建所有屏幕对象

2. 产生循环，定时重画屏幕

   1. 刷新屏幕为蓝，33

   2. 小鸟向下移动,py[0]+=10;

   3. 障碍物左移,px[k]-=20;

   4. 若有鼠标点击事件则小鸟向上移动py[0]-=10;

   5. 判断是否GAME OVER,结束则爆红,44

      ```c
      int sys_repaint(int x,int y,int h)
      {
      	int i,j,w;
      	char *p;
      	i=x;
      	j=y;
      	p=0xA0000;
      	w=barrier_width;
      	if(i+w>=320 || i<20 ) return 0;
      	if(i==33 || j==33){
      p=0xA0000;
      	for(i=0;i<memsize;i++) *p++=3;
      	return 0;
      	}
      	else if(i==44 || j==44 ){
      p=0xA0000;
      	for(i=0;i<memsize;i++) *p++=4;
      	return 0;
      	}else{
      	for(i=x;i<=x+w;i++){	
      		for(j=y;j<=y+h;j++){
      			p=0xA0000+j*320+i;
      			*p=12;
      		}
      		}
      }
      return 0;
      }
      ```

3. 最终效果

   游戏中：

   ![game](https://user-images.githubusercontent.com/56508903/148795422-e5cad648-6060-49dc-9fdc-8e32582aaa3c.png)

   GAME OVER:

   ![gameover](https://user-images.githubusercontent.com/56508903/148795441-1bba34db-2af7-47d2-9a7c-398c98724e6d.png)
