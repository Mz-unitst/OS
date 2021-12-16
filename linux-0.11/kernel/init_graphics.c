#include <linux/kernel.h>
#include<asm/io.h>
int sys_init_graphics()
{
outb(0x05,0x3CE);
outb(0x40,0x3CF);/* 256 yidong fengzhuang*/
outb(0x06,0x3CE);
outb(0x05,0x3CF);/*kaiqi tuxingmoshi A0000*/
outb(0x04,0x3C4);
outb(0x08,0x3C5);/*jilian*/

outb(0x01,0x3D4);
outb(0x4F,0x3D5);/* end horizontal display=79*/
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
outb(0x28,0x3D5);/*Offset=40*/

outb(0x0C,0x3D4);/**/
outb(0x0,0x3D5);/**/
outb(0x0D,0x3D4);/**/
outb(0x0,0x3D5);/*Start Address=0xA0000*/

}
