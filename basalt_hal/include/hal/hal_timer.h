#pragma once

#include <stdint.h>

#include "hal/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

int hal_timer_init(hal_timer_t *timer,
                   uint64_t period_us,
                   int periodic,
                   hal_timer_cb_t cb,
                   void *arg);

int hal_timer_deinit(hal_timer_t *timer);

int hal_timer_start(hal_timer_t *timer);

int hal_timer_stop(hal_timer_t *timer);

int hal_timer_set_period(hal_timer_t *timer, uint64_t period_us);

int hal_timer_is_running(hal_timer_t *timer, int *running_out);

#ifdef __cplusplus
}
#endif
