#pragma once

#include <errno.h>

#include "esp_err.h"

static inline int hal_esp_err_to_errno(esp_err_t err) {
    switch (err) {
        case ESP_OK: return 0;
        case ESP_ERR_INVALID_ARG: return -EINVAL;
        case ESP_ERR_INVALID_STATE: return -EBUSY;
        case ESP_ERR_NO_MEM: return -ENOMEM;
        case ESP_ERR_TIMEOUT: return -ETIMEDOUT;
#ifdef ESP_ERR_NOT_SUPPORTED
        case ESP_ERR_NOT_SUPPORTED: return -ENOTSUP;
#endif
#ifdef ESP_ERR_NOT_FOUND
        case ESP_ERR_NOT_FOUND: return -ENOENT;
#endif
        default: return -EIO;
    }
}
