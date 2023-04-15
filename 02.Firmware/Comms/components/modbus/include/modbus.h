/**
 * @file modbus.h
 * @author Micls
 * @brief
 * @version 0.1
 * @date 2023-04-15
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef INC_MODBUS_H
#define INC_MODBUS_H

#include "esp_event.h"

#ifdef __cplusplus
extern "C"
{
#endif

    ESP_EVENT_DECLARE_BASE(TASK_EVENTS);

    void init_modbus_system(esp_event_loop_args_t *loop_args);

#ifdef __cplusplus
}
#endif

#endif /* INC_MODBUS_H */