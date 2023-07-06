#include "../include/simulator.h"

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

#include <stdio.h>

#include <string.h>

#include <iostream>

#include <vector>

using namespace std;
#define TIMEOUT_INTERVAL 10.0

vector < msg > buffer;
int head, tail;
int active_msg;
int expected_packet=0;





int calc_checksum(struct pkt *p)
{
  int checksum = 0;

  if(p == NULL)
  {
    return checksum;
  }
  for (int i=0; i<20; i++)
  {
    checksum += (unsigned char)p->payload[i];
  }
  checksum += p->seqnum;
  checksum += p->acknum;
  return checksum;
}

int corrupt(struct pkt packet) {
	int checksum= calc_checksum(&packet);
	if(checksum==packet.checksum)
	{
		return 0; //correct
	}
	else
	{
		return 1; //corrupted
	}
	
}

void send_msg()
{
	cout<<"send_msg"<<endl;
	cout<<"sending pkt with num "<<head<<endl;
	 struct pkt A_pkt;
	    struct msg text=buffer[head];
		A_pkt.acknum=head;
		A_pkt.seqnum=head;
		memcpy(A_pkt.payload, text.data, 20);
        A_pkt.checksum = calc_checksum(& A_pkt);
        active_msg--;
        head++;
		tolayer3(0, A_pkt);
        starttimer(0, TIMEOUT_INTERVAL);
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message) {
	cout<<"A_output"<<endl;
	buffer.push_back(message);
	tail++;
	active_msg++;
	if(tail-head>0)
	{
		cout<<"tail-head "<<tail-head<<"calling send msg"<<endl;
		send_msg();
	}

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet) {
	cout<<"A_input"<<endl;
	cout<<"head "<<head<<endl;
	cout<<"packet.acknum "<<packet.acknum<<endl;
	if(head-1==packet.acknum && corrupt(packet)==0)
	{
		cout<<"received pkt with no "<<packet.acknum<<endl;
		if(tail-head>0)
		{
			send_msg();
		}
		else
		{
		stoptimer(0);
		}
		

	}

}

/* called when A's timer goes off */
void A_timerinterrupt() {
	cout<<"A_timerinterrupt"<<endl;
	if(head-1>=0)
	{
		cout<<"head -1 is "<<head-1<<endl;
			struct msg text=buffer[head-1];
			 struct pkt A_pkt;
		A_pkt.acknum=head-1;
		A_pkt.seqnum=head-1;
		memcpy(A_pkt.payload, text.data, 20);
		A_pkt.checksum = calc_checksum(& A_pkt);
		tolayer3(0, A_pkt);
        starttimer(0, TIMEOUT_INTERVAL);
        cout<<"sent msg with no "<<head-1<<endl;
		
	}

}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {
  head=0;
  tail=0;
  active_msg=0;

}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet) {
	cout<<"B_input"<<endl;
	
	cout<<"received pkt "<<packet.acknum<<endl;
	cout<<"expected "<<expected_packet<<endl;
	cout<<"corrupt check "<<corrupt(packet)<<endl;
	 struct pkt B_ack;
	if(packet.acknum==expected_packet && corrupt(packet)==0)
	{
		cout<<"correct"<<endl;
		tolayer5(1, packet.payload);
	B_ack.seqnum = expected_packet;
    B_ack.acknum = expected_packet;
    memcpy(B_ack.payload, packet.payload, 20);
    B_ack.checksum = calc_checksum(& B_ack);
    //B_ack.checksum = checksum((unsigned short *)&B_ack, sizeof(struct pkt));
    tolayer3(1, B_ack);
    expected_packet++;
	}
	else
	{
		cout<<"wrong"<<endl;
	B_ack.seqnum = -1;
    B_ack.acknum = -1;
    memcpy(B_ack.payload, packet.payload, 20);
     B_ack.checksum = calc_checksum(& B_ack);
    tolayer3(1, B_ack);
	}
	
	
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() {
 expected_packet=0;

}


