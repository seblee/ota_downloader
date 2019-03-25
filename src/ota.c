/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-3-25      Seblee       the first version
 */
#include "ota.h"
#include <rtthread.h>

#define DBG_ENABLE
#define DBG_SECTION_NAME "ota"
#define DBG_LEVEL DBG_INFO
#define DBG_COLOR
#include <rtdbg.h>
#ifndef LOG_D
#error "Please update the 'rtdbg.h' file to GitHub latest version (https://github.com/RT-Thread/rt-thread/blob/master/include/rtdbg.h)"
#endif

int ota_start(app_struct_t app_info_p)
{
    static int g_ota_is_initialized = 0;

    if (1 == g_ota_is_initialized)
    {
        LOG_E("iot ota has been initialized");
        return -RT_ERROR;
    }

    rt_thread_t tid;
    tid = rt_thread_create("mbm_fsm", http_ota_fw_download_entry, app_info_p,
                           0x3000, RT_THREAD_PRIORITY_MAX / 3, 2);
    RT_ASSERT(tid != RT_NULL);
    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
        return RT_EOK;
    }
    return -RT_ERROR;
}
