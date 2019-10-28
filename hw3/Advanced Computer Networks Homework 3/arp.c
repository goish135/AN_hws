#include "arp.h"

#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <linux/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//You can fill the following functions or add other functions if needed. If not, you needn't write anything in them.  
void set_hard_type(struct ether_arp *packet, unsigned short int type)
{}
void set_prot_type(struct ether_arp *packet, unsigned short int type)
{}
void set_hard_size(struct ether_arp *packet, unsigned char size)
{}
void set_prot_size(struct ether_arp *packet, unsigned char size)
{}
void set_op_code(struct ether_arp *packet, short int code)
{}

void set_sender_hardware_addr(struct ether_arp *packet, char *address)
{}
void set_sender_protocol_addr(struct ether_arp *packet, char *address)
{}
void set_target_hardware_addr(struct ether_arp *packet, char *address)
{}
void set_target_protocol_addr(struct ether_arp *packet, char *address)
{}

char* get_target_protocol_addr(struct ether_arp *packet)
{
	// if you use malloc, remember to free it.
}
char* get_sender_protocol_addr(struct ether_arp *packet)
{
	// if you use malloc, remember to free it.
}
char* get_sender_hardware_addr(struct ether_arp *packet)
{
	// if you use malloc, remember to free it.
}
char* get_target_hardware_addr(struct ether_arp *packet)
{
	// if you use malloc, remember to free it.
}
