/*
 * This file is part of HiKoB Openlab.
 *
 * HiKoB Openlab is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, version 3.
 *
 * HiKoB Openlab is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with HiKoB Openlab. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2011,2012 HiKoB.
 */

/**
 * \file
 *         rtimer-arch.c for OpenLab platforms
 * \author
 *         Clement Burin Des Roziers <clement.burin-des-roziers.at.hikob.com>
 *         Antoine Fraboulet <antoine.fraboulet.at.hikob.com>
 */

#include "contiki-conf.h"
#include "sys/energest.h"
#include "sys/rtimer.h"
#include "rtimer-arch.h"

#include "stm32/timer_.h"
#include "stm32f1xx/rcc.h"
#include "cortex-m3/cm3_nvic_registers.h"
#include "softtimer/soft_timer_.h"

#define LOG_LEVEL LOG_LEVEL_INFO
#define NO_DEBUG_HEADER
#include "lib/debug.h"

static rtimer_clock_t currently_scheduled;
static void rtimer_arch_schedule_current();

/*
 * rtimer is mapped on the soft_timer
 */
#define RTIMER_TIMER        ((const _openlab_timer_t *) softtim.timer)
#define RTIMER_CHANNEL      softtim.channel
#define RTIMER_IRQ_LINE     RTIMER_TIMER->irq_line

/*-----------------------------------------------------------------------------------*/

void rtimer_arch_init(void)
{
  /* timer is already started in agilefox_drivers.c */
  log_debug("rtimer_arch_init() called");
  // ensure lowest priority so that rtimer tasks can be interrupted
  nvic_set_priority(RTIMER_IRQ_LINE, 0xff);
}

/*-----------------------------------------------------------------------------------*/

rtimer_clock_t
rtimer_arch_now(void)
{
  return soft_timer_time();
}

/*-----------------------------------------------------------------------------------*/

static void rtimer_cb(handler_arg_t arg, uint16_t value)
{
  (void) value;

  ENERGEST_ON(ENERGEST_TYPE_IRQ);

  if(currently_scheduled == 0) { /* Timer expired */
    rtimer_run_next();
  } else {
    rtimer_arch_schedule_current();
  }

  ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}

/*---------------------------------------------------------------------------*/
static void
rtimer_arch_schedule_current(void)
{
  uint16_t target;
  rtimer_clock_t now;

  platform_enter_critical();

  now = RTIMER_NOW();

  if(currently_scheduled == 0) {
    platform_exit_critical();
    return;
  }

  if(RTIMER_CLOCK_LT(currently_scheduled, now + 1)) {
    rtimer_run_next();
    platform_exit_critical();
    return;
  }

  if(RTIMER_CLOCK_DIFF(currently_scheduled, now) & 0xffff0000) { /* Cannot be scheduled yet */
    target = now & 0xffff; /* Wait for next wrap */
    timer_set_channel_compare(RTIMER_TIMER, RTIMER_CHANNEL, target, rtimer_cb, NULL);
  } else { /* Can be scheduled now */
    target = currently_scheduled & 0xffff;
    currently_scheduled = 0;
    timer_set_channel_compare(RTIMER_TIMER, RTIMER_CHANNEL, target, rtimer_cb, NULL);
  }

  platform_exit_critical();

}

void rtimer_arch_schedule(rtimer_clock_t t)
{
  currently_scheduled = t;
  rtimer_arch_schedule_current();
}

/*-----------------------------------------------------------------------------------*/

