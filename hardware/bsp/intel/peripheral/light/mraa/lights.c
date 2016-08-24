/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Based on htc/flounder/lights/lights.h
 */

#define LOG_TAG "lights"

#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <cutils/log.h>
#include <hardware/lights.h>
#include <hardware/hardware.h>
#include <gpio.h>

/* List of supported lights */
typedef enum {
    NOTIFICATIONS_TYPE,
    LIGHTS_TYPE_NUM
} light_type_t;

/* Light device data structure */
struct light_device_ext_t {
    /* Base device */
    struct light_device_t base_dev;
    /* Physical pin */
    int pin;
    /* Current state of the light device */
    struct light_state_t state;
    /* Number of device references */
    int refs;
    /* Synchronization attributes */
    pthread_t flash_thread;
    pthread_cond_t flash_cond;
    pthread_mutex_t flash_signal_mutex;
    pthread_mutex_t write_mutex;
    /* Transform function to apply on value */
    int (*transform)(int);
};

static int64_t const ONE_MS_IN_NS = 1000000LL;
static int64_t const ONE_S_IN_NS = 1000000000LL;

/*
 * Platform version strings used to identify board versions
 */
static char * const EDISON_ARDUINO_PLATFORM_VERSION = "arduino";
static char * const MINNOWBOARD_TURBOT_PLATFORM_VERSION = "Turbot";

/*
 * Pin constants
 * Please add a pin to EDISON_ARDUINO_PINS, EDISON_MINIBOARD_PINS &
 * MINNOWBOARD_MAX_PINS when you add a new light type
 */
static int const EDISON_ARDUINO_PINS[LIGHTS_TYPE_NUM] = {13};
static int const EDISON_MINIBOARD_PINS[LIGHTS_TYPE_NUM] = {31};
static int const MINNOWBOARD_MAX_PINS[LIGHTS_TYPE_NUM] = {21};
static int const MINNOWBOARD_TURBOT_PINS[LIGHTS_TYPE_NUM] = {27};

/*
 * Array of light devices with write_mutex statically initialized
 * to be able to synchronize the open_lights & close_lights functions
 */
struct light_device_ext_t light_devices[] = {
    [ 0 ... (LIGHTS_TYPE_NUM - 1) ] = { .write_mutex = PTHREAD_MUTEX_INITIALIZER }
};

/*
 * Set the GPIO value
 * @param pin physical pin of the GPIO
 * @param value what value to set
 * @return 0 if success, error code otherwise
 */
static int set_gpio_value(int pin, int value)
{
    mraa_gpio_context gpio = NULL;
    int rc = 0;

    if ((value != 0) && (value != 1)) {
        return EINVAL;
    }

    gpio = mraa_gpio_init(pin);
    if (gpio == NULL) {
        return EPERM;
    }

    if (mraa_gpio_dir(gpio, MRAA_GPIO_OUT) != MRAA_SUCCESS) {
        rc = EPERM;
        goto close_gpio;
    }

    if (mraa_gpio_write(gpio, value) != MRAA_SUCCESS) {
        rc = EPERM;
    }

close_gpio:
    if (mraa_gpio_close(gpio) != MRAA_SUCCESS) {
        rc = EPERM;
    }

    return rc;
}

/*
 * Invert value
 * @param value what value to invert
 * @return value inverted
 */
static int invert_value(int value) {
    return value ? 0 : 1;
}

/*
 * Get current timestamp in nanoseconds
 * @return time in nanoseconds
 */
int64_t get_timestamp_monotonic()
{
    struct timespec ts = {0, 0};

    if (!clock_gettime(CLOCK_MONOTONIC, &ts)) {
        return ONE_S_IN_NS * ts.tv_sec + ts.tv_nsec;
    }

    return -1;
}

/*
 * Populates a timespec data structure from a int64_t timestamp
 * @param out what timespec to populate
 * @param target_ns timestamp in nanoseconds
 */
void set_timestamp(struct timespec *out, int64_t target_ns)
{
    out->tv_sec  = target_ns / ONE_S_IN_NS;
    out->tv_nsec = target_ns % ONE_S_IN_NS;
}

/*
 * pthread routine which flashes an LED
 * @param flash_param light device pointer
 */
static void * flash_routine (void *flash_param)
{
    struct light_device_ext_t *dev = (struct light_device_ext_t *)flash_param;
    struct light_state_t *flash_state;
    int color = 0, rc = 0;
    struct timespec target_time;
    int64_t timestamp, period;

    if (dev == NULL) {
        ALOGE("%s: Cannot flash a NULL light device", __func__);
        return NULL;
    }

    flash_state = &dev->state;

    pthread_mutex_lock(&dev->flash_signal_mutex);

    color = flash_state->color;

    /* Light flashing loop */
    while (flash_state->flashMode) {
        rc = set_gpio_value(dev->pin, color);
        if (rc != 0) {
            ALOGE("%s: Cannot set light color", __func__);
            goto mutex_unlock;
        }

        timestamp = get_timestamp_monotonic();
        if (timestamp < 0) {
            ALOGE("%s: Cannot get time from monotonic clock", __func__);
            goto mutex_unlock;
        }

        if (color) {
            color = 0;
            period = flash_state->flashOnMS * ONE_MS_IN_NS;
        } else {
            color = 1;
            period = flash_state->flashOffMS * ONE_MS_IN_NS;
        }

        /* check for overflow */
        if (timestamp > LLONG_MAX - period) {
            ALOGE("%s: Timestamp overflow", __func__);
            goto mutex_unlock;
        }

        timestamp += period;

        /* sleep until target_time or the cond var is signaled */
        set_timestamp(&target_time, timestamp);
        rc = pthread_cond_timedwait(&dev->flash_cond, &dev->flash_signal_mutex, &target_time);
        if ((rc != 0) && (rc != ETIMEDOUT)) {
            ALOGE("%s: pthread_cond_timedwait returned an error", __func__);
            goto mutex_unlock;
        }
    }

mutex_unlock:
    pthread_mutex_unlock(&dev->flash_signal_mutex);

    return NULL;
}

/*
 * Check lights flash state
 * @param state pointer to the state to check
 * @return 0 if success, error code otherwise
 */
static int check_flash_state(struct light_state_t const *state)
{
    int64_t ns = 0;

    if ((state->flashOffMS < 0) || (state->flashOnMS < 0)) {
        return EINVAL;
    }

    if ((state->flashOffMS == 0) && (state->flashOnMS) == 0) {
        return EINVAL;
    }

    /* check for overflow in ns */
    ns = state->flashOffMS * ONE_MS_IN_NS;
    if (ns / ONE_MS_IN_NS != state->flashOffMS) {
        return EINVAL;
    }
    ns = state->flashOnMS * ONE_MS_IN_NS;
    if (ns / ONE_MS_IN_NS != state->flashOnMS) {
        return EINVAL;
    }

    return 0;
}

/*
 * Generic function for setting the state of the light
 * @param base_dev light device data structure
 * @param state what state to set
 * @return 0 if success, error code otherwise
 */
static int set_light_generic(struct light_device_t *base_dev,
        struct light_state_t const *state)
{
    struct light_device_ext_t *dev = (struct light_device_ext_t *)base_dev;
    struct light_state_t *current_state;
    int rc = 0;

    if (dev == NULL) {
        ALOGE("%s: Cannot set state for NULL device", __func__);
        return EINVAL;
    }

    current_state = &dev->state;

    pthread_mutex_lock(&dev->write_mutex);

    if (dev->refs == 0) {
        ALOGE("%s: The light device is not opened", __func__);
        pthread_mutex_unlock(&dev->write_mutex);
        return EINVAL;
    }

    ALOGV("%s: flashMode:%x, color:%x", __func__, state->flashMode, state->color);

    if (current_state->flashMode) {
        /* destroy flashing thread */
        pthread_mutex_lock(&dev->flash_signal_mutex);
        current_state->flashMode = LIGHT_FLASH_NONE;
        pthread_cond_signal(&dev->flash_cond);
        pthread_mutex_unlock(&dev->flash_signal_mutex);
        pthread_join(dev->flash_thread, NULL);
    }

    *current_state = *state;
    if (dev->transform != NULL) {
        current_state->color = dev->transform(current_state->color);
    }

    if (current_state->flashMode) {
        /* start flashing thread */
        if (check_flash_state(current_state) == 0) {
            rc = pthread_create(&dev->flash_thread, NULL,
                    flash_routine, (void *)dev);
            if (rc != 0) {
                ALOGE("%s: Cannot create flashing thread", __func__);
                current_state->flashMode = LIGHT_FLASH_NONE;
            }
        } else {
            ALOGE("%s: Flash state is invalid", __func__);
            current_state->flashMode = LIGHT_FLASH_NONE;
        }
    } else {
        rc = set_gpio_value(dev->pin, current_state->color);
        if (rc != 0) {
            ALOGE("%s: Cannot set light color.", __func__);
        }
    }

    pthread_mutex_unlock(&dev->write_mutex);

    return rc;
}

/*
 * Initialize light synchronization resources
 * @param cond what condition variable to initialize
 * @param signal_mutex what mutex (associated with the condvar) to initialize
 * @return 0 if success, error code otherwise
 */
static int init_light_sync_resources(pthread_cond_t *cond,
        pthread_mutex_t *signal_mutex)
{
    int rc = 0;
    pthread_condattr_t condattr;

    rc = pthread_condattr_init(&condattr);
    if (rc != 0) {
        ALOGE("%s: Cannot initialize the pthread condattr", __func__);
        return rc;
    }

    rc = pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
    if (rc != 0) {
        ALOGE("%s: Cannot set the clock of condattr to monotonic", __func__);
        goto destroy_condattr;
    }

    rc = pthread_cond_init(cond, &condattr);
    if (rc != 0) {
        ALOGE("%s: Cannot intialize the pthread structure", __func__);
        goto destroy_condattr;
    }

    rc = pthread_mutex_init(signal_mutex, NULL);
    if (rc != 0) {
        ALOGE("%s: Cannot initialize the mutex associated with the pthread cond", __func__);
        goto destroy_cond;
    }

    pthread_condattr_destroy(&condattr);
    return rc;

destroy_cond:
    pthread_cond_destroy(cond);
destroy_condattr:
    pthread_condattr_destroy(&condattr);
    return rc;
}

/*
 * Free light synchronization resources
 * @param cond what condition variable to free
 * @param signal_mutex what mutex (associated with the condvar) to free
 */
static void free_light_sync_resources(pthread_cond_t *cond,
        pthread_mutex_t *signal_mutex)
{
    pthread_mutex_destroy(signal_mutex);
    pthread_cond_destroy(cond);
}

/*
 * Close the lights module
 * @param base_dev light device data structure
 * @return 0 if success, error code otherwise
 */
static int close_lights(struct light_device_t *base_dev)
{
    struct light_device_ext_t *dev = (struct light_device_ext_t *)base_dev;
    int rc = 0;

    if (dev == NULL) {
        ALOGE("%s: Cannot deallocate a NULL light device", __func__);
        return EINVAL;
    }

    pthread_mutex_lock(&dev->write_mutex);

    if (dev->refs == 0) {
        /* the light device is not open */
        rc = EINVAL;
        goto mutex_unlock;
    } else if (dev->refs > 1) {
        goto dec_refs;
    }

    if (dev->state.flashMode) {
        /* destroy flashing thread */
        pthread_mutex_lock(&dev->flash_signal_mutex);
        dev->state.flashMode = LIGHT_FLASH_NONE;
        pthread_cond_signal(&dev->flash_cond);
        pthread_mutex_unlock(&dev->flash_signal_mutex);
        pthread_join(dev->flash_thread, NULL);
    }

    free_light_sync_resources(&dev->flash_cond,
            &dev->flash_signal_mutex);

dec_refs:
    dev->refs--;

mutex_unlock:
    pthread_mutex_unlock(&dev->write_mutex);

    return rc;
}

/*
 * Module initialization routine which detects the LEDs' GPIOs
 * @param type light device type
 * @return 0 if success, error code otherwise
 */
static int init_module(int type)
{
    const char *platform_version = NULL;

    if (type < 0 || type >= LIGHTS_TYPE_NUM) {
        return EINVAL;
    }

    light_devices[type].transform = NULL;

    switch(mraa_get_platform_type()) {
        case MRAA_INTEL_EDISON_FAB_C:
            platform_version = mraa_get_platform_version(MRAA_MAIN_PLATFORM_OFFSET);
            if ((platform_version != NULL) && (strncmp(platform_version,
                    EDISON_ARDUINO_PLATFORM_VERSION,
                    strlen(EDISON_ARDUINO_PLATFORM_VERSION)) == 0)) {
                light_devices[type].pin = EDISON_ARDUINO_PINS[type];
            } else {
                light_devices[type].pin = EDISON_MINIBOARD_PINS[type];
            }
            break;
        case MRAA_INTEL_MINNOWBOARD_MAX:
            platform_version = mraa_get_platform_version(MRAA_MAIN_PLATFORM_OFFSET);
            if ((platform_version != NULL) && (strncmp(platform_version,
                    MINNOWBOARD_TURBOT_PLATFORM_VERSION,
                    strlen(MINNOWBOARD_TURBOT_PLATFORM_VERSION)) == 0)) {
                light_devices[type].pin = MINNOWBOARD_TURBOT_PINS[type];
                light_devices[type].transform = invert_value;
            } else {
                light_devices[type].pin = MINNOWBOARD_MAX_PINS[type];
            }
            break;
        default:
            ALOGE("%s: Hardware platform not supported", __func__);
            return EINVAL;
    }

    return 0;
}

/*
 * Open a new lights device instance by name
 * @param module associated hw module data structure
 * @param name lights device name
 * @param device where to store the pointer of the allocated device
 * @return 0 if success, error code otherwise
 */
static int open_lights(const struct hw_module_t *module, char const *name,
        struct hw_device_t **device)
{
    struct light_device_ext_t *dev;
    int rc = 0, type = -1;

    ALOGV("%s: Opening %s lights module", __func__, name);

    if (0 == strcmp(LIGHT_ID_NOTIFICATIONS, name)) {
        type = NOTIFICATIONS_TYPE;
    } else {
        return EINVAL;
    }

    dev = (struct light_device_ext_t *)(light_devices + type);

    pthread_mutex_lock(&dev->write_mutex);

    if (dev->refs != 0) {
        /* already opened; nothing to do */
        goto inc_refs;
    }

    rc = init_module(type);
    if (rc != 0) {
        ALOGE("%s: Failed to initialize lights module", __func__);
        goto mutex_unlock;
    }

    rc = init_light_sync_resources(&dev->flash_cond,
                &dev->flash_signal_mutex);
    if (rc != 0) {
        goto mutex_unlock;
    }

    dev->base_dev.common.tag = HARDWARE_DEVICE_TAG;
    dev->base_dev.common.version = 0;
    dev->base_dev.common.module = (struct hw_module_t *)module;
    dev->base_dev.common.close = (int (*)(struct hw_device_t *))close_lights;
    dev->base_dev.set_light = set_light_generic;

inc_refs:
    dev->refs++;
    *device = (struct hw_device_t *)dev;

mutex_unlock:
    pthread_mutex_unlock(&dev->write_mutex);
    return rc;
}

static struct hw_module_methods_t lights_methods =
{
    .open =  open_lights,
};

struct hw_module_t HAL_MODULE_INFO_SYM =
{
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = LIGHTS_HARDWARE_MODULE_ID,
    .name = "Edison lights module",
    .author = "Intel",
    .methods = &lights_methods,
};
