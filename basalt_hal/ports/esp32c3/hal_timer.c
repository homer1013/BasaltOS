// BasaltOS ESP32-c3 HAL - Timer (esp_timer backend)

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include "hal/hal_timer.h"

#include "esp_err.h"
#include "hal_errno.h"
#include "esp_timer.h"

typedef struct {
    esp_timer_handle_t h;
    uint64_t period_us;
    bool periodic;
    bool initialized;
    bool running;
    hal_timer_cb_t cb;
    void *arg;
} hal_timer_impl_t;

_Static_assert(sizeof(hal_timer_impl_t) <= sizeof(((hal_timer_t *)0)->_opaque),
               "hal_timer_t opaque storage too small for esp32c3 hal_timer_impl_t");

static inline hal_timer_impl_t *T(hal_timer_t *timer) {
    return (hal_timer_impl_t *)timer->_opaque;
}

static void timer_cb_thunk(void *arg) {
    hal_timer_impl_t *t = (hal_timer_impl_t *)arg;
    if (t && t->cb) t->cb(t->arg);
}

int hal_timer_init(hal_timer_t *timer,
                   uint64_t period_us,
                   int periodic,
                   hal_timer_cb_t cb,
                   void *arg) {
    if (!timer || !cb || period_us == 0) return -EINVAL;

    hal_timer_impl_t *t = T(timer);
    t->h = NULL;
    t->period_us = period_us;
    t->periodic = periodic != 0;
    t->cb = cb;
    t->arg = arg;
    t->initialized = false;
    t->running = false;

    esp_timer_create_args_t args = {
        .callback = timer_cb_thunk,
        .arg = t,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "basalt_hal_timer",
        .skip_unhandled_events = false,
    };

    esp_err_t e = esp_timer_create(&args, &t->h);
    if (e != ESP_OK) return hal_esp_err_to_errno(e);

    t->initialized = true;
    return 0;
}

int hal_timer_deinit(hal_timer_t *timer) {
    if (!timer) return -EINVAL;
    hal_timer_impl_t *t = T(timer);
    if (!t->initialized || !t->h) return -EINVAL;

    if (t->running) {
        (void)esp_timer_stop(t->h);
        t->running = false;
    }
    esp_err_t e = esp_timer_delete(t->h);
    t->h = NULL;
    t->initialized = false;
    return hal_esp_err_to_errno(e);
}

int hal_timer_start(hal_timer_t *timer) {
    if (!timer) return -EINVAL;
    hal_timer_impl_t *t = T(timer);
    if (!t->initialized || !t->h) return -EINVAL;

    if (t->running) {
        (void)esp_timer_stop(t->h);
        t->running = false;
    }

    esp_err_t e = t->periodic
        ? esp_timer_start_periodic(t->h, t->period_us)
        : esp_timer_start_once(t->h, t->period_us);
    if (e != ESP_OK) return hal_esp_err_to_errno(e);

    t->running = true;
    return 0;
}

int hal_timer_stop(hal_timer_t *timer) {
    if (!timer) return -EINVAL;
    hal_timer_impl_t *t = T(timer);
    if (!t->initialized || !t->h) return -EINVAL;
    if (!t->running) return 0;

    esp_err_t e = esp_timer_stop(t->h);
    if (e != ESP_OK) return hal_esp_err_to_errno(e);

    t->running = false;
    return 0;
}

int hal_timer_set_period(hal_timer_t *timer, uint64_t period_us) {
    if (!timer || period_us == 0) return -EINVAL;
    hal_timer_impl_t *t = T(timer);
    if (!t->initialized || !t->h) return -EINVAL;

    bool was_running = t->running;
    if (was_running) {
        int rc = hal_timer_stop(timer);
        if (rc != 0) return rc;
    }

    t->period_us = period_us;
    if (was_running) {
        return hal_timer_start(timer);
    }
    return 0;
}

int hal_timer_is_running(hal_timer_t *timer, int *running_out) {
    if (!timer || !running_out) return -EINVAL;
    hal_timer_impl_t *t = T(timer);
    if (!t->initialized) return -EINVAL;

    *running_out = t->running ? 1 : 0;
    return 0;
}
