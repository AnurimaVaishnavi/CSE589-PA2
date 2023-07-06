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

#include <limits.h>

using namespace std;
#define TIMEOUT_INTERVAL 300.0

vector < struct msg > buffer;
vector < struct msg > stash_buffer;

int A_state;
int B_state;

int active_msg = 0;
int base = 0;
int next_seq_num = 0;
int head = 0;
int tail = 0;
int window_size = 0;
int expected_seq_num = 0;
int window_start = 0;
int window_end = 0;


int calc_checksum(struct pkt packet)
{
    char data[20];
    memcpy(data, packet.payload, 20);
    int checkSum = 0;
    int i = 0;
    for(i = 0; i < 20; i++)
    {
      if(data[i] == '\0')
      {
        break;
      }
      checkSum += data[i];
    }
    checkSum += packet.seqnum;
    checkSum += packet.acknum;
    return checkSum;
}

int corrupt(struct pkt packet) {
  int checksum = calc_checksum(  packet);
  if (checksum == packet.checksum) {
    return 0; //correct
  } else {
    return 1; //corrupted
  }

}




/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message) {

  buffer.push_back(message);
  if (next_seq_num < base + window_size) {
    if (tail - head > 0) {
      int to_keep = 0;
      while (tail - head > 0 && next_seq_num < base + window_size) {
        //cout << "sending msg from buffer " << next_seq_num << endl;
        struct pkt A_pkt;
        A_pkt.acknum = 0;
        A_pkt.seqnum = next_seq_num;
        struct msg text = stash_buffer[head];
        memcpy(A_pkt.payload, text.data, 20);
        A_pkt.checksum = calc_checksum(  A_pkt);
        if (to_keep == 0) {
          stash_buffer.push_back(message);
          tail++;
          active_msg++;
          to_keep = 1;
        }

        head++;
        active_msg--;
        tolayer3(0, A_pkt);
        if (base == next_seq_num) {
          starttimer(0, TIMEOUT_INTERVAL);
        }
        next_seq_num++;

      }

    } else {
      for (int i = base; i <= next_seq_num; i++) {
       // cout << " msg sent from else  " << i << endl;
        struct pkt A_pkt;
        A_pkt.acknum = 0;
        A_pkt.seqnum = i;
        struct msg message = buffer[i];
        memcpy(A_pkt.payload, message.data, 20);
        A_pkt.checksum = calc_checksum( A_pkt);
        tolayer3(0, A_pkt);
        if (base == next_seq_num) {
          starttimer(0, TIMEOUT_INTERVAL);
        }
      }
      next_seq_num++;
    }
  } else {
   // cout << "msg just stored in stash" << endl;
    stash_buffer.push_back(message);
    tail++;
    active_msg++;
  }

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet) {
  if (corrupt(packet) == 0 && packet.acknum >= base && packet.acknum < next_seq_num && packet.seqnum >= 0) {
    // cout << "ack received for packet.acknum " << packet.acknum << endl;
    base = packet.acknum + 1;
    if (base == next_seq_num) {
      stoptimer(0);
    } else {
      if (tail - head > 0) {
        while (tail - head > 0 && next_seq_num < base + window_size) {
          // cout << "sending msg from buffer " << next_seq_num << endl;
          struct pkt A_pkt;
          A_pkt.acknum = 0;
          A_pkt.seqnum = next_seq_num;
          struct msg text = stash_buffer[head];
          memcpy(A_pkt.payload, text.data, 20);
          A_pkt.checksum = calc_checksum(  A_pkt);
          head++;
          active_msg--;
          tolayer3(0, A_pkt);
          if (base == next_seq_num) {
            starttimer(0, TIMEOUT_INTERVAL);
          }
          next_seq_num++;
        }
      } else {
        stoptimer(0);
        starttimer(0, TIMEOUT_INTERVAL);
      }
    }
  }
  //cout << "window_start " << window_start << endl;

}

/* called when A's timer goes off */
void A_timerinterrupt() {
  starttimer(0, TIMEOUT_INTERVAL);

  for (int i = base; i < next_seq_num; i++) {
   // cout << "resending window msg " << i << endl;
    struct pkt A_pkt;
    A_pkt.acknum = 0;
    A_pkt.seqnum = i;
    struct msg message = buffer[i];
    memcpy(A_pkt.payload, message.data, 20);
    A_pkt.checksum = calc_checksum(  A_pkt);
    tolayer3(0, A_pkt);
  }

}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {
  window_size = getwinsize();
  base = 0;
  next_seq_num = 0;
  head = 0;
  tail = 0;
  active_msg = 0;

}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet) {
  struct pkt B_ack;
  if (corrupt(packet) == 0 && packet.seqnum == expected_seq_num) {
    cout << "correct" << endl;
    tolayer5(1, packet.payload);
    B_ack.acknum = expected_seq_num;
    B_ack.seqnum = expected_seq_num;
    memcpy(B_ack.payload, packet.payload, 20);
    B_ack.checksum = calc_checksum(  B_ack);
    tolayer3(1, B_ack);
    expected_seq_num++;
  } else {
    cout << "wrong" << endl;
    B_ack.acknum = expected_seq_num;
    B_ack.seqnum = -100;
    memcpy(B_ack.payload, packet.payload, 20);
    B_ack.checksum = calc_checksum(  B_ack);
    tolayer3(1, B_ack);

  }

}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() {
  expected_seq_num = 0;

}

