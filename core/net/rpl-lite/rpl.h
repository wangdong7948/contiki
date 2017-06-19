/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 * \file
 *	Public API declarations for RPL.
 * \author
 *	Joakim Eriksson <joakime@sics.se> & Nicolas Tsiftes <nvt@sics.se>
 *  Simon Duquennoy <simon.duquennoy@ri.se>
 *
 */

#ifndef RPL_H
#define RPL_H

/********** Includes **********/

#include "uip.h"
#include "rpl-const.h"
#include "rpl-conf.h"
#include "rpl-types.h"
#include "rpl-icmp6.h"
#include "rpl-dag.h"
#include "rpl-ns.h"
#include "rpl-dag-root.h"
#include "rpl-neighbor.h"
#include "rpl-ext-header.h"
#include "rpl-timers.h"

/**
 * \addtogroup uip6
 * @{
 */

/********** Public symbols **********/

/* The only instance */
extern rpl_instance_t curr_instance;
/* The RPL multicast address (used for DIS and DIO) */
extern uip_ipaddr_t rpl_multicast_addr;

/********** Public functions **********/

/**
 * Set prefix from an prefix data structure (from DIO)
 *
 * \param prefix The prefix
 * \return 1 if success, 0 otherwise
 */
int rpl_set_prefix(rpl_prefix_t *prefix);
/**
* Set prefix from an IPv6 address
*
* \param addr The prefix
* \param len The prefix length
* \param flags The DIO prefix flags
* \return 1 if success, 0 otherwise
*/

int rpl_set_prefix_from_addr(uip_ipaddr_t *addr, unsigned len, uint8_t flags);

/**
 * Removes current prefx
 *
 * \param last_prefix The last prefix (which is to be removed)
 */
void rpl_reset_prefix(rpl_prefix_t *last_prefix);

/**
 * Get one of the node's global addresses
 *
 * \return A (constant) pointer to the global IPv6 address found.
 * The pointer directs to the internals of DS6, should only be used
 * in the current function's local scope
 */
const uip_ipaddr_t *rpl_get_global_address(void);

/**
 * Get the RPL's best guess on if we are reachable via have downward route or not.
 *
 * \return 1 if we are reachable, 0 otherwise.
 */
int rpl_is_reachable(void);

/**
 * Greater-than function for a lollipop counter
 *
 * \param a The first counter
 * \param b The second counter
 * \return 1 is a>b else 0
 */
int rpl_lollipop_greater_than(int a, int b);

/**
 * Initialize RPL main module
 */
void rpl_init(void);

 /** @} */

#endif /* RPL_H */
