#include "cc_hal_os.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <inttypes.h> // 如果需要用 PRIu32，可以保留
#include "cc_log.h"

#define TAG "HAL_OS_TASK"

cc_os_semphr_handle_t cc_hal_os_semphr_create_binary(void){
    return xSemaphoreCreateBinary();
}

cc_os_semphr_handle_t cc_hal_os_semphr_create_mutex(void){
    return xSemaphoreCreateMutex();
}

cc_err_t cc_hal_os_semphr_delete(cc_os_semphr_handle_t handle){
    if(handle == NULL){
        return CC_FAIL;
    }
    vSemaphoreDelete(handle);
    return CC_OK;
}
cc_err_t cc_hal_os_semphr_take(cc_os_semphr_handle_t handle, cc_os_tick_t tick){
    if(handle == NULL){
        return CC_FAIL;
    }
    BaseType_t xStatus = xSemaphoreTake(handle, tick);
    if(xStatus != pdPASS){
        return CC_FAIL;
    }
    return CC_OK;
}
cc_err_t cc_hal_os_semphr_give(cc_os_semphr_handle_t handle){
    if(handle == NULL){
        return CC_FAIL;
    }
    BaseType_t xStatus = xSemaphoreGive(handle);
    if(xStatus != pdPASS){
        return CC_FAIL;
    }
    return CC_OK;
}

void cc_hal_os_task_delay(cc_os_tick_t tick){
    vTaskDelay(tick);
}

void cc_hal_os_task_delete(cc_os_task_handle_t handle){
    vTaskDelete(handle);
}

/* ---------------------------------------------------------------------------
 * 注意：
 *  - 如果 priority 是 uint8_t，请打印时强转为 (unsigned) 或 (uint32_t)。
 *  - 如果 stack_size 是 uint32_t，同理可用 %u + (unsigned) 强转。
 *  - 如果 free_heap 是 size_t，最好用 %zu。如果不想用 %zu，就用 %u + (unsigned)强转。
 * ---------------------------------------------------------------------------
 */
cc_err_t cc_hal_os_task_create(cc_os_task_t task,
                               const char *name,
                               uint32_t stack_size,
                               void *arg,
                               uint8_t priority,
                               cc_os_task_handle_t *handle)
{
    // 使用 %s 打印字符串
    CC_LOGI(TAG, "Attempting to create task: %s", name);

    // 如果想坚持用 PRIu32，也可以写成：
    // CC_LOGI(TAG, "Task parameters: stack_size=%" PRIu32 ", priority=%" PRIu32,
    //         (uint32_t)stack_size, (uint32_t)priority);

    // 这里直接用 %u + (unsigned) 强转，最简单直观
    CC_LOGI(TAG, "Task parameters: stack_size=%u, priority=%u",
            (unsigned)stack_size,
            (unsigned)priority);

    if (task == NULL) {
        CC_LOGE(TAG, "Error: Task function pointer is NULL");
        return CC_FAIL;
    }
    if (name == NULL) {
        CC_LOGE(TAG, "Error: Task name is NULL");
        return CC_FAIL;
    }
    if (handle == NULL) {
        CC_LOGE(TAG, "Error: Task handle pointer is NULL");
        return CC_FAIL;
    }

    // Check available heap size
    size_t free_heap = xPortGetFreeHeapSize();

    // 打印 size_t，最好是用 %zu；若编译器不支持或报错，则改用 %u + (unsigned)
    CC_LOGI(TAG, "Current free heap size: %zu bytes", free_heap);
    // 或：CC_LOGI(TAG, "Current free heap size: %u bytes", (unsigned)free_heap);

    if (stack_size > free_heap) {
        // 同理，用 %zu 或 %u + (unsigned) 都行
        CC_LOGE(TAG,
                "Error: Insufficient heap for task stack (required: %u, available: %u)",
                (unsigned)stack_size,
                (unsigned)free_heap);
        return CC_FAIL;
    }

    BaseType_t xStatus = xTaskCreate((TaskFunction_t)task,
                                     name,
                                     stack_size,
                                     arg,
                                     priority,
                                     (TaskHandle_t *)handle);

    if (xStatus != pdPASS) {
        CC_LOGE(TAG, "Failed to create task: %s", name);
        switch (xStatus) {
            case errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY:
                CC_LOGE(TAG, "Error: Could not allocate required memory");
                break;
            default:
                CC_LOGE(TAG, "Error: Unknown (status code: %d)", (int)xStatus);
                break;
        }
        return CC_FAIL;
    }

    CC_LOGI(TAG, "Successfully created task: %s", name);

    // 打印任务句柄 (TaskHandle_t) 的值，通常是个指针
    // 可以做 (unsigned) 强转，然后用 0x%x 或 %p 也行
    CC_LOGI(TAG, "Task handle: 0x%x", (unsigned)*handle);

    // 再次打印剩余堆大小
    free_heap = xPortGetFreeHeapSize();
    CC_LOGI(TAG, "Remaining free heap size: %zu bytes", free_heap);
    // 或：CC_LOGI(TAG, "Remaining free heap size: %u bytes", (unsigned)free_heap);

    return CC_OK;
}
