/**
 * @file sync_data.h
 * @author Micls
 * @brief
 * @version 0.1
 * @date 2023-04-17
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef INC_SYNC_DATA_H
#define INC_SYNC_DATA_H

#include <stdbool.h>
#include "esp_err.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void task_sync_data(void);
    void init_sync_data_func(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_SYNC_DATA_H */
