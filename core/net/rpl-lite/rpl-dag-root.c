/*
 * Copyright (c) 2012-2014, Thingsquare, http://www.thingsquare.com/.
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
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "contiki.h"
#include "contiki-net.h"

#include "rpl.h"
#include "net/ipv6/uip-ds6-route.h"

#include <string.h>

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

#define RPL_DAG_GRACE_PERIOD (CLOCK_SECOND * 20 * 1)

static uint8_t to_become_root;
static struct ctimer c;

/*---------------------------------------------------------------------------*/
static void
create_dag_callback(void *ptr)
{
  const uip_ipaddr_t *root, *ipaddr;

  root = curr_instance.used ? &curr_instance.dag.dag_id : NULL;
  ipaddr = rpl_get_global_address();

  if(root == NULL || uip_ipaddr_cmp(root, ipaddr)) {
    /* The RPL network we are joining is one that we created, so we
       become root. */
    if(to_become_root) {
      rpl_dag_root_init_dag_immediately();
      to_become_root = 0;
    }
  } else {
#if DEBUG
    printf("Found a network we did not create\n");
    printf("version %d grounded %d preference %d rank %d\n",
           curr_instance.dag.version, curr_instance.dag.grounded,
           curr_instance.dag.preference, curr_instance.dag.rank);
#endif /* DEBUG */

    /* We found a RPL network that we did not create so we just join
       it without becoming root. But if the network has an infinite
       rank, we assume the network has broken, and we become the new
       root of the network. */

    if(curr_instance.dag.rank == RPL_INFINITE_RANK) {
      if(to_become_root) {
        rpl_dag_root_init_dag_immediately();
        to_become_root = 0;
      }
    }

    /* Try again after the grace period */
    ctimer_set(&c, RPL_DAG_GRACE_PERIOD, create_dag_callback, NULL);
  }
}
/*---------------------------------------------------------------------------*/
static void
set_global_address(uip_ipaddr_t *prefix, uip_ipaddr_t *iid)
{
  static uip_ipaddr_t root_ipaddr;
  int i;
  uint8_t state;

  /* Assign a unique local address (RFC4193,
     http://tools.ietf.org/html/rfc4193). */
  if(prefix == NULL) {
    uip_ip6addr(&root_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
  } else {
    memcpy(&root_ipaddr, prefix, 8);
  }
  if(iid == NULL) {
    uip_ds6_set_addr_iid(&root_ipaddr, &uip_lladdr);
  } else {
    memcpy(((uint8_t*)&root_ipaddr) + 8, ((uint8_t*)iid) + 8, 8);
  }

  uip_ds6_addr_add(&root_ipaddr, 0, ADDR_AUTOCONF);

  printf("IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
      printf("\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
void
rpl_dag_root_init(uip_ipaddr_t *prefix, uip_ipaddr_t *iid)
{
  static uint8_t initialized = 0;

  if(!initialized) {
    to_become_root = 0;
    set_global_address(prefix, iid);
    initialized = 1;
  }
}
/*---------------------------------------------------------------------------*/
int
rpl_dag_root_init_dag_immediately(void)
{
  struct uip_ds6_addr *root_if;
  int i;
  uint8_t state;
  uip_ipaddr_t *ipaddr = NULL;

  rpl_dag_root_init(NULL, NULL);

  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       state == ADDR_PREFERRED &&
       !uip_is_addr_linklocal(&uip_ds6_if.addr_list[i].ipaddr)) {
      ipaddr = &uip_ds6_if.addr_list[i].ipaddr;
    }
  }

  root_if = uip_ds6_addr_lookup(ipaddr);
  if(ipaddr != NULL || root_if != NULL) {

    rpl_dag_init_root(RPL_DEFAULT_INSTANCE, ipaddr,
      (uip_ipaddr_t *)rpl_get_global_address(), 64, UIP_ND6_RA_FLAG_AUTONOMOUS);
    rpl_dag_update_state();

    PRINTF("rpl_dag_root_init_dag: created a new RPL dag\n");
    return 0;
  } else {
    PRINTF("rpl_dag_root_init_dag: failed to create a new RPL DAG\n");
    return -1;
  }
}
/*---------------------------------------------------------------------------*/
void
rpl_dag_root_init_dag_delay(void)
{
  rpl_dag_root_init(NULL, NULL);

  ctimer_set(&c, RPL_DAG_GRACE_PERIOD, create_dag_callback, NULL);
  to_become_root = 1;

  /* Send a DIS packet to request RPL info from neighbors. */
  rpl_icmp6_dis_output(NULL);
}
/*---------------------------------------------------------------------------*/
int
rpl_dag_root_is_root(void)
{
  return curr_instance.used && curr_instance.dag.rank == ROOT_RANK;
}
/*---------------------------------------------------------------------------*/
