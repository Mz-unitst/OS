#define __LIBRARY__
#include<stdio.h>
#include<unistd.h>
#include<errno.h>
#include<time.h>

#define vga_graph_memstart 0xA0000
#define vga_graph_memsize 64000
#define cursor_side 6
#define vga_width 320
#define vga_height 200

#define BIRD_X 120
#define BIRD_Y 100
#define BIRD_WIDTH 10
#define BIRD_HEIGHT 8

typedef struct{
int posx;
int posy;
int width;
int height;
}object;
char *p;
int i,j,x_pos,y_pos;
int m;
int cnt;
int bird_y;
object objects[20];

void repaint(int nn){
object cur;
cur=objects[nn];
char *startt;
char *pp;
int ii,jj;
startt=(char *)vga_graph_memstart+cur.posy*vga_width+cur.posx;
for(ii=0;ii<cur.width;ii++)
{
    for(jj=0;jj<cur.height;jj++)
    {
        pp=startt+vga_width*jj+ii;
        *pp=13;
        //*pp is op
    }
}

}


_syscall0(void,init_graphics)
_syscall0(int,get_message)


int main(void)
{

    p=vga_graph_memstart;
    x_pos=20;
    y_pos=20;
    bird_y=BIRD_Y;

    init_graphics();
    for(i=0;i<vga_graph_memsize;i++)
     *p++ = 3;
    for (i=x_pos-cursor_side;i<=x_pos+cursor_side;i++)
    for(j=y_pos-cursor_side;j<=y_pos+cursor_side;j++)
        {
        p=(char *)vga_graph_memstart+j*vga_width+i;
        *p=12;
        }

objects[0].posx=BIRD_X;
objects[0].posy=bird_y;
objects[0].width=BIRD_WIDTH;
objects[0].height=BIRD_HEIGHT;
cnt=10;
//
// sheng cheng zhang ai wu
//



}
#define __LIBRARY__
#include<stdio.h>
#include<unistd.h>
#include<errno.h>
#include<time.h>

#define vga_graph_memstart 0xA0000
#define vga_graph_memsize 64000
#define cursor_side 6
#define vga_width 320
#define vga_height 200

#define BIRD_X 120
#define BIRD_Y 100
#define BIRD_WIDTH 10
#define BIRD_HEIGHT 8

typedef struct{
int posx;
int posy;
int width;
int height;
}object;
char *p;
int i,j,x_pos,y_pos;
int m;
int cnt;
int bird_y;
object objects[20];

void repaint(int nn){
object cur;
cur=objects[nn];
char *startt;
char *pp;
int ii,jj;
startt=(char *)vga_graph_memstart+cur.posy*vga_width+cur.posx;
for(ii=0;ii<cur.width;ii++)
{
    for(jj=0;jj<cur.height;jj++)
    {
        pp=startt+vga_width*jj+ii;
        *pp=13;
        //*pp is op
    }
}

}


_syscall0(void,init_graphics)
_syscall0(int,get_message)


int main(void)
{

    p=vga_graph_memstart;
    x_pos=20;
    y_pos=20;
    bird_y=BIRD_Y;

    init_graphics();
    for(i=0;i<vga_graph_memsize;i++)
     *p++ = 3;
    for (i=x_pos-cursor_side;i<=x_pos+cursor_side;i++)
    for(j=y_pos-cursor_side;j<=y_pos+cursor_side;j++)
        {
        p=(char *)vga_graph_memstart+j*vga_width+i;
        *p=12;
        }

objects[0].posx=BIRD_X;
objects[0].posy=bird_y;
objects[0].width=BIRD_WIDTH;
objects[0].height=BIRD_HEIGHT;
cnt=10;
//
// sheng cheng zhang ai wu
//



}

