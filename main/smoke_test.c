#include "smoke_test.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#if BASALT_ENABLE_PSRAM
#include "esp_psram.h"
#endif

#include "hal/hal_gpio.h"
// Build-time feature gates / board config
#include "basalt_config.h"
// UART smoke test temporarily disabled:
// #include "hal/hal_uart.h"

static const char *TAG = "basalt_smoke";

static int check_esp(const char *name, esp_err_t err) {
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "%s: OK", name);
        printf("[smoke] %s: OK\n", name);
        return 0;
    }
    ESP_LOGE(TAG, "%s: FAIL (%s)", name, esp_err_to_name(err));
    printf("[smoke] %s: FAIL (%s)\n", name, esp_err_to_name(err));
    return -1;
}

static int check_rc(const char *name, int rc) {
    if (rc == 0) {
        ESP_LOGI(TAG, "%s: OK", name);
        printf("[smoke] %s: OK\n", name);
        return 0;
    }
    ESP_LOGE(TAG, "%s: FAIL (rc=%d)", name, rc);
    printf("[smoke] %s: FAIL (rc=%d)\n", name, rc);
    return -1;
}

static int smoke_gpio_blink(int pin) {
    hal_gpio_t led;
    int rc = 0;
    char name[48];

    if (pin < 0) {
        ESP_LOGW(TAG, "GPIO smoke test skipped (no LED pin configured)");
        printf("[smoke] GPIO smoke test skipped (no LED pin configured)\n");
        return 0;
    }

    snprintf(name, sizeof(name), "hal_gpio_init(GPIO%d)", pin);
    rc |= check_rc(name, hal_gpio_init(&led, pin));
    rc |= check_rc("hal_gpio_set_mode(OUT)", hal_gpio_set_mode(&led, HAL_GPIO_OUTPUT));

    // Blink a few times
    for (int i = 0; i < 3; i++) {
        rc |= check_rc("hal_gpio_write(1)", hal_gpio_write(&led, 1));
        vTaskDelay(pdMS_TO_TICKS(120));
        rc |= check_rc("hal_gpio_write(0)", hal_gpio_write(&led, 0));
        vTaskDelay(pdMS_TO_TICKS(120));
    }

    int val = -1;
    rc |= check_rc("hal_gpio_read()", hal_gpio_read(&led, &val));
    ESP_LOGI(TAG, "GPIO%d readback: %d", pin, val);
    printf("[smoke] GPIO%d readback: %d\n", pin, val);

    rc |= check_rc("hal_gpio_deinit()", hal_gpio_deinit(&led));
    return (rc == 0) ? 0 : -1;
}

/*
// UART smoke test temporarily disabled (UART0 is usually already installed by IDF console/logging)

static int smoke_uart0_tx(void) {
    hal_uart_t uart;
    int rc = 0;

    rc |= check_rc("hal_uart_init(UART0,115200)", hal_uart_init(&uart, 0, 115200));

    const char *msg = "BasaltOS smoke: UART0 TX OK\r\n";
    int sent = hal_uart_send(&uart, (const uint8_t *)msg, (int)strlen(msg), 1000);
    if (sent < 0) {
        ESP_LOGE(TAG, "hal_uart_send: FAIL (sent=%d)", sent);
        printf("[smoke] hal_uart_send: FAIL (sent=%d)\n", sent);
        rc = -1;
    } else {
        ESP_LOGI(TAG, "hal_uart_send: OK (%d bytes)", sent);
        printf("[smoke] hal_uart_send: OK (%d bytes)\n", sent);
    }

    rc |= check_rc("hal_uart_deinit()", hal_uart_deinit(&uart));
    return (rc == 0) ? 0 : -1;
}
*/

static int smoke_data_rw(void) {
#if !BASALT_ENABLE_FS_SPIFFS && !BASALT_ENABLE_FS_SD
    // No filesystem backend enabled, so skip /data I/O and don't fail the smoke test.
    ESP_LOGW(TAG, "FS smoke test skipped (no FS backend enabled)");
    printf("[smoke] FS smoke test skipped (no FS backend enabled)\n");
    return 0;
#else
    // BasaltOS should already have SPIFFS (or other FS) mounted at /data.
    // So we just do normal POSIX file I/O here.
    const char *path = "/data/smoke.txt";

    FILE *f = fopen(path, "w");
    if (!f) {
        ESP_LOGE(TAG, "fopen(w) failed: %s (errno=%d: %s)", path, errno, strerror(errno));
        printf("[smoke] fopen(w) failed: %s (errno=%d: %s)\n", path, errno, strerror(errno));
        return -1;
    }

    fprintf(f, "BasaltOS smoke test OK\n");
    fclose(f);

    char buf[128] = {0};
    f = fopen(path, "r");
    if (!f) {
        ESP_LOGE(TAG, "fopen(r) failed: %s (errno=%d: %s)", path, errno, strerror(errno));
        printf("[smoke] fopen(r) failed: %s (errno=%d: %s)\n", path, errno, strerror(errno));
        return -1;
    }

    if (!fgets(buf, sizeof(buf), f)) {
        ESP_LOGE(TAG, "fgets failed: %s (errno=%d: %s)", path, errno, strerror(errno));
        printf("[smoke] fgets failed: %s (errno=%d: %s)\n", path, errno, strerror(errno));
        fclose(f);
        return -1;
    }

    fclose(f);

    ESP_LOGI(TAG, "DATA read: %s", buf);
    printf("[smoke] DATA read: %s", buf);
    return 0;
#endif
}

int basalt_smoke_test_run(void) {
    ESP_LOGI(TAG, "=== BasaltOS POST start ===");
    printf("[smoke] === BasaltOS POST start ===\n");

    esp_chip_info_t info;
    esp_chip_info(&info);

    ESP_LOGI(TAG, "chip: model=%d cores=%d rev=%d", info.model, info.cores, info.revision);
    printf("[smoke] chip: model=%d cores=%d rev=%d\n", info.model, info.cores, info.revision);

    size_t heap_free = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    ESP_LOGI(TAG, "heap free (8bit): %u", (unsigned)heap_free);
    printf("[smoke] heap free (8bit): %u\n", (unsigned)heap_free);

    int rc = 0;
    rc |= smoke_gpio_blink(BASALT_PIN_LED);

    // UART smoke test temporarily disabled
    // rc |= smoke_uart0_tx();

    // File I/O under /data (assumes FS already mounted by BasaltOS)
    rc |= smoke_data_rw();

#if BASALT_ENABLE_PSRAM
    if (!esp_psram_is_initialized()) {
        ESP_LOGE(TAG, "PSRAM init: FAIL (not initialized)");
        printf("[smoke] PSRAM init: FAIL (not initialized)\n");
        rc |= -1;
    } else {
        size_t psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        ESP_LOGI(TAG, "PSRAM init: OK (free=%u)", (unsigned)psram_free);
        printf("[smoke] PSRAM init: OK (free=%u)\n", (unsigned)psram_free);
    }
#endif

    if (rc == 0) {
        ESP_LOGI(TAG, "=== BasaltOS POST PASS ===");
        printf("[smoke] === BasaltOS POST PASS ===\n");
        return 0;
    }

    ESP_LOGE(TAG, "=== BasaltOS POST FAIL ===");
    printf("[smoke] === BasaltOS POST FAIL ===\n");
    return -1;
}
