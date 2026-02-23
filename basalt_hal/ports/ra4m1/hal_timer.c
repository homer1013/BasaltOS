#include <errno.h>
#include <stdint.h>

#include "hal/hal_timer.h"

typedef struct {
    uint64_t period_us;
    int periodic;
    hal_timer_cb_t cb;
    void *arg;
    int initialized;
    int running;
} hal_timer_impl_t;

_Static_assert(sizeof(hal_timer_impl_t) <= sizeof(((hal_timer_t *)0)->_opaque),
               "hal_timer_t opaque storage too small for ra4m1 hal_timer_impl_t");

static inline hal_timer_impl_t *T(hal_timer_t *timer) {
    return (hal_timer_impl_t *)timer->_opaque;
}

int hal_timer_init(hal_timer_t *timer,
                   uint64_t period_us,
                   int periodic,
                   hal_timer_cb_t cb,
                   void *arg) {
    if (!timer || period_us == 0) return -EINVAL;
    hal_timer_impl_t *impl = T(timer);
    impl->period_us = period_us;
    impl->periodic = (periodic != 0) ? 1 : 0;
    impl->cb = cb;
    impl->arg = arg;
    impl->initialized = 1;
    impl->running = 0;
    return 0;
}

int hal_timer_deinit(hal_timer_t *timer) {
    if (!timer) return -EINVAL;
    hal_timer_impl_t *impl = T(timer);
    if (!impl->initialized) return -EINVAL;
    impl->initialized = 0;
    impl->running = 0;
    return 0;
}

int hal_timer_start(hal_timer_t *timer) {
    if (!timer) return -EINVAL;
    hal_timer_impl_t *impl = T(timer);
    if (!impl->initialized) return -EINVAL;
    impl->running = 1;
    return 0;
}

int hal_timer_stop(hal_timer_t *timer) {
    if (!timer) return -EINVAL;
    hal_timer_impl_t *impl = T(timer);
    if (!impl->initialized) return -EINVAL;
    impl->running = 0;
    return 0;
}

int hal_timer_set_period(hal_timer_t *timer, uint64_t period_us) {
    if (!timer || period_us == 0) return -EINVAL;
    hal_timer_impl_t *impl = T(timer);
    if (!impl->initialized) return -EINVAL;
    impl->period_us = period_us;
    return 0;
}

int hal_timer_is_running(hal_timer_t *timer, int *running_out) {
    if (!timer || !running_out) return -EINVAL;
    hal_timer_impl_t *impl = T(timer);
    if (!impl->initialized) return -EINVAL;
    *running_out = impl->running;
    return 0;
}
