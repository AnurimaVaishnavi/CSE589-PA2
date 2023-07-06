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
/*
 * note about get_sim_time(): 
 * this function returns the absolute time of this simulator rather 
 * than giving us the relative time between every starttimer()
 */

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/


#include <stdio.h>

#include <string.h>

#include <iostream>

#include <vector>
#include <stdlib.h>

using namespace std;
#define TIMEOUT_INTERVAL 30.0
#define BUFLEN 1000


struct SRpacket {
  struct pkt packet;
  float timer;
  int acked; // 0- not yet, 1- acked
};


SRpacket source_buffer[2000];
pkt receiver_buffer[1000];
vector < struct msg > stash_buffer;
int active_msg_size = 0;
int base = 0;
int next_seq_num = 0;
int head = 0;
int tail = 0;
int window_size = 0;
int expected_seq_num = 0;
int receiver_base = 0;
struct pkt A_pkt;
struct pkt B_ack;
int count=0;




char buffer[BUFLEN][20];
int buffered = 0;
int buffer_head = 0;
int buffer_tail = 0;


int calc_checksum(struct pkt * p) {
  int checksum = 0;

  if (p == NULL) {
    return checksum;
  }
  for (int i = 0; i < 20; i++) {
    checksum += (unsigned char) p -> payload[i];
  }
  checksum += p -> seqnum;
  checksum += p -> acknum;
  return checksum;
}

int corrupt(struct pkt packet) {
  int checksum = calc_checksum( & packet);
  if (checksum == packet.checksum) {
    return 0; //correct
  } else {
    return 1; //corrupted
  }

}


void add_to_source_buffer(struct pkt A_pkt)
{
          struct SRpacket source_packet;
        source_packet.packet.seqnum = A_pkt.seqnum;
        source_packet.packet.acknum = A_pkt.acknum;
        source_packet.packet.checksum = A_pkt.checksum;
        memcpy(source_packet.packet.payload, A_pkt.payload, 20);
        source_packet.acked = 0;
        source_packet.timer = get_sim_time() + TIMEOUT_INTERVAL;

        source_buffer[next_seq_num] = source_packet;
}


/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message) {
  cout << "A_output" << endl;
  count++;
  if (next_seq_num < base + window_size) {
    cout << "next_seq_num is " << next_seq_num << endl;
    cout << "base is " << base << endl;
    cout << "active_msg_size " << buffered << endl;
    if (buffered==0) {
      cout << "msg sent direct " << next_seq_num << endl;
      A_pkt.seqnum = next_seq_num;
      A_pkt.acknum = 0;
      A_pkt.checksum = 0;

      memcpy(A_pkt.payload, message.data, 20);
       A_pkt.checksum = calc_checksum( & A_pkt);
       add_to_source_buffer(A_pkt);
      tolayer3(0, A_pkt);
      next_seq_num++;
      for (int i = 0; i <= count; i++) {
        cout << source_buffer[i].packet.seqnum << " ";
      }
      cout << endl;

    } else {
      int flag = 0;
      while (next_seq_num < base + window_size && buffered > 0) {
        cout << "msg sent from buffer " << next_seq_num << endl;
        A_pkt.seqnum = next_seq_num;
        A_pkt.acknum = 0;
        A_pkt.checksum = 0;
      memcpy(A_pkt.payload, buffer[buffer_head], 20);
      A_pkt.checksum = calc_checksum( & A_pkt);
      buffer_head = (buffer_head + 1) % BUFLEN;
      buffered--;

      add_to_source_buffer(A_pkt);
        if (flag == 0) {
          memcpy(buffer[buffer_tail], message.data, 20);
          buffer_tail = (buffer_tail + 1) % BUFLEN;
          buffered++;
          flag = 1;
        }
        tolayer3(0, A_pkt);
        next_seq_num++;
        for (int i = 0; i <= count; i++) {
          cout << source_buffer[i].packet.seqnum << " ";
        }
        cout << endl;

      }
    }
  } else {
    if (buffered < BUFLEN) {
      memcpy(buffer[buffer_tail], message.data, 20);
      buffer_tail = (buffer_tail + 1) % BUFLEN;
      buffered++;
    }
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet) {
  cout << "A_input" << endl;
  cout << "corrupt(packet) is " << corrupt(packet) << endl;
  cout << "base  " << base << endl;
  if (corrupt(packet) == 0) {
    source_buffer[packet.acknum].acked = 1;
    cout << "ack received for " << packet.acknum << endl;

    if (packet.acknum == base) {
      while (source_buffer[base].acked == 1) {
        base++;
      }
    }

    while (buffered > 0 && next_seq_num < base + window_size) {
      A_pkt.seqnum = next_seq_num;
      A_pkt.acknum = 0;
      A_pkt.checksum = 0;
      memcpy(A_pkt.payload, buffer[buffer_head], 20);
      A_pkt.checksum = calc_checksum( & A_pkt);
      buffer_head = (buffer_head + 1) % BUFLEN;
      buffered--;
      add_to_source_buffer(A_pkt);

      tolayer3(0, A_pkt);
      next_seq_num++;
      for (int i = 0; i <= count; i++) {
        cout << source_buffer[i].packet.seqnum << " ";
      }
      cout << endl;
    }
  }

}

/* called when A's timer goes off */
void A_timerinterrupt() {
  cout << "A_timerinterrupt" << endl;
  cout << "base is " << base << endl;
  cout << "next_seq_num " << next_seq_num << endl;
  for (int i = 0; i <= count; i++) {
    cout << source_buffer[i].packet.seqnum << " ";
  }
  cout << endl;
  for (int i = base; i < next_seq_num; i++) {
    if (source_buffer[i].acked == 0 && source_buffer[i].timer <= get_sim_time()) {
      source_buffer[i].timer = get_sim_time() + TIMEOUT_INTERVAL;
      tolayer3(0, source_buffer[i].packet);
    }
  }
  starttimer(0, 5.0);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {
  base = 1;
  next_seq_num = 1;
   window_size = getwinsize();
  starttimer(0, 5.0);
}


void send_ack(int seqnum)
{
        B_ack.seqnum = 0;
      B_ack.acknum = seqnum;
      B_ack.checksum = 0;
      memset(B_ack.payload, 0, 20);
       B_ack.checksum = calc_checksum( & B_ack);
      tolayer3(1, B_ack);

}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet) {
  cout << "B_input" << endl;
  cout << "corrupt check " << corrupt(packet) << endl;
  cout << "receiver_base " << receiver_base << endl;
  cout << "recieved pkt " << packet.seqnum << endl;
  if (corrupt(packet) == 0) {
    if (packet.seqnum <= receiver_base - 1 && packet.seqnum >= receiver_base - window_size) {
      cout << "msgs already in the buffer " << packet.seqnum << endl;
      send_ack(packet.seqnum);

    } else {
      cout << "got new msg " << packet.seqnum << endl;
      send_ack(packet.seqnum);

      receiver_buffer[packet.seqnum] = packet;
      for (int i = 0; i <= count; i++) {
        cout << receiver_buffer[i].seqnum << " ";
      }
      cout << endl;
      if (packet.seqnum == receiver_base) {
        while (receiver_buffer[receiver_base].seqnum != 0) {
          tolayer5(1, receiver_buffer[receiver_base].payload);
          receiver_base++;
        }
      }
    }
  }
  cout << "base finally in reciever " << receiver_base << endl;
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() {
  receiver_base = 1;
}

