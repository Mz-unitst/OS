#include "linux/tty.h"
struct message sys_get_message(struct message * msgg)
{
struct message *curr;
curr->next=headd->next;
if(msgg==NULL)return;
while(curr->next!=NULL) {
curr=curr->next;
curr->next=msgg;
}
return;
}
