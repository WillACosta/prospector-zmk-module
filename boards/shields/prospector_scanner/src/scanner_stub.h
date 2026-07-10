/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/status_scanner.h>
#include <zmk/status_advertisement.h>

#define SCANNER_PENDING_DISPLAY_NAME_LEN 32

struct scanner_pending_display_data {
    volatile bool update_pending;
    volatile bool signal_update_pending;
    volatile bool no_keyboards;

    char device_name[SCANNER_PENDING_DISPLAY_NAME_LEN];
    char layer_name[5];
    int layer;
    int wpm;
    bool usb_ready;
    bool ble_connected;
    bool ble_bonded;
    int profile;
    uint8_t modifiers;
    int bat[4];
    int8_t rssi;
    float rate_hz;
    int scanner_battery;
    bool scanner_battery_pending;

    uint8_t kb_version_major;
    uint8_t kb_version_minor;
    uint8_t kb_version_patch;
    bool kb_version_dev;
    bool kb_version_valid;
};

/**
 * @brief Send keyboard data received from BLE advertisement
 *
 * Lock-free: pushes data into SPSC ring buffer.
 * Called from BT RX thread. Data is processed by scanner_process_incoming()
 * in the LVGL timer context.
 *
 * @param adv_data Parsed advertisement data
 * @param rssi Signal strength
 * @param device_name Keyboard device name
 * @param ble_addr BLE MAC address (6 bytes)
 * @param ble_addr_type BLE address type
 * @return 0 on success, negative error code on failure
 */
int scanner_msg_send_keyboard_data(const struct zmk_status_adv_data *adv_data,
                                   int8_t rssi, const char *device_name,
                                   const uint8_t *ble_addr, uint8_t ble_addr_type);

/**
 * @brief Process incoming advertisements from ring buffer
 *
 * Must be called from LVGL timer context (main thread).
 * Drains ring buffer, manages keyboards[], calculates rates,
 * checks timeouts, and updates scanner battery.
 */
void scanner_process_incoming(void);

/**
 * @brief Trigger timeout check for keyboards
 *
 * Checks if any keyboards have timed out and updates display accordingly.
 *
 * @return 0 on success, negative error code on failure
 */
int scanner_msg_send_timeout_check(void);

/**
 * @brief Get keyboard data by index
 *
 * @param index Keyboard index
 * @param data Output: advertisement data
 * @param rssi Output: signal strength
 * @param name Output: keyboard name
 * @param name_len Size of name buffer
 * @return true if keyboard found, false otherwise
 */
bool scanner_get_keyboard_data(int index, struct zmk_status_adv_data *data,
                               int8_t *rssi, char *name, size_t name_len);

/**
 * @brief Get the count of active keyboards
 *
 * @return Number of active keyboards
 */
int scanner_get_active_keyboard_count(void);

/**
 * @brief Copy keyboard status by index (thread-safe snapshot)
 *
 * Copies the slot under the data mutex. keyboards[] is mutated on the
 * system workqueue while the display runs on its own thread, so callers
 * must never hold a pointer into the array - always use this copy API.
 *
 * @param index Keyboard index (0 to MAX_KEYBOARDS-1)
 * @param out Destination for the snapshot
 * @return true if the slot is active and copied, false otherwise
 */
bool scanner_copy_keyboard_status(int index, struct zmk_keyboard_status *out);

/**
 * @brief Get the selected keyboard index
 *
 * @return Selected keyboard index
 */
int scanner_get_selected_keyboard(void);

/**
 * @brief Set the selected keyboard index
 *
 * @param index Keyboard index to select
 */
void scanner_set_selected_keyboard(int index);

/**
 * @brief Send display refresh request
 *
 * Triggers immediate display update after screen transitions to MAIN.
 *
 * @return 0 on success, negative error code on failure
 */
int scanner_msg_send_display_refresh(void);

/**
 * @brief Request ambient brightness sensor processing on the display thread.
 *
 * Called from the brightness control work queue; scanner_stub.c provides the
 * display-thread handoff.
 *
 * @return 0 on success, negative error code on failure
 */
int scanner_msg_send_brightness_sensor_read(void);

bool scanner_get_pending_update(struct scanner_pending_display_data *out);
bool scanner_is_signal_pending(void);
bool scanner_get_pending_battery(int *level);
bool scanner_get_kb_version(uint8_t *major, uint8_t *minor, uint8_t *patch,
                            bool *is_dev, char *name, size_t name_len);
