/*
 * Copyright (c) 2016, Inria.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         An example of Rime/TSCH
 * \author
 *         Simon Duquennoy <simon.duquennoy@inria.fr>
 *
 */

#include <stdio.h>
#include "contiki-conf.h"
#include "net/netstack.h"
#include "net/net-debug.h"
#include "net/rime/rime.h"
#include "net/mac/tsch/tsch.h"
#include "deployment.h"
#include "orchestra.h"

const linkaddr_t coordinator_addr =    { { 1, 0 } };

static int glossy_tx_count;

#define TX_COUNT 3
static uint16_t glossy_round = 0;

#define START_DELAY (30 * CLOCK_SECOND)

/*---------------------------------------------------------------------------*/
PROCESS(unicast_test_process, "Rime Glossy Node");
AUTOSTART_PROCESSES(&unicast_test_process);

/*---------------------------------------------------------------------------*/
static void
glossy_send()
{
  if(glossy_tx_count--) {
    packetbuf_copyfrom(&glossy_round, sizeof(glossy_round));
    packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_CMDFRAME); /* We use CMD frame as a hack */
    NETSTACK_MAC.send(NULL, NULL);
    printf("App: glossy sending %u\n", glossy_round);
  }
}
/*---------------------------------------------------------------------------*/
void
glossy_input()
{
  uint16_t recv_glossy_round;
  NETSTACK_FRAMER.parse();
  recv_glossy_round = *((uint16_t*)packetbuf_dataptr());
  if(recv_glossy_round > glossy_round) {
    glossy_round = recv_glossy_round;
    glossy_tx_count = TX_COUNT;
  }
  printf("App: glossy received %x\n", glossy_round);
  glossy_send();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(unicast_test_process, ev, data)
{
  static struct etimer et;
  PROCESS_BEGIN();

  if(deployment_init(ROOT_ID)) {
  } else {
    etimer_set(&et, 60 * CLOCK_SECOND);
    while(1) {
      printf("Info: Not running. My MAC address: ");
      net_debug_lladdr_print((const uip_lladdr_t *)&linkaddr_node_addr);
      printf("\n");
      PROCESS_WAIT_UNTIL(etimer_expired(&et));
      etimer_reset(&et);
    }
  }

  printf("App: my node id %u %u\n", node_id, LOG_ID_FROM_LINKADDR(&linkaddr_node_addr));

  tsch_set_coordinator(node_id == ROOT_ID);
  NETSTACK_MAC.on();

  orchestra_init();

  etimer_set(&et, START_DELAY);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

  if(node_id == ROOT_ID) {

    etimer_set(&et, 15 * CLOCK_SECOND);
    while(1) {
      glossy_round++;
      glossy_tx_count = TX_COUNT;
      glossy_send();

      etimer_set(&et, 10 * CLOCK_SECOND + ((random_rand()>>6) % (5 * CLOCK_SECOND)));
      PROCESS_WAIT_UNTIL(etimer_expired(&et));
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
