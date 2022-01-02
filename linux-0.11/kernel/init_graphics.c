#include <linux/kernel.h>
#include<asm/io.h>
#include "linux/tty.h"
#define memstart 0xA0000+1360
#define memsize 64000
#define cursor_side 3
#define width 320
#define height 200
//struct message *headd;
int volatile jumpp;
int sys_init_graphics()
{
    char *p;
    int i,j,x,y;
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

int sys_get_message()
{
	//msgg=headd;
	//if(headd->mid!=1)return;
	//headd=headd->next;
	//if(jumpp>0) --jumpp;
	return jumpp;
}
