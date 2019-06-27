/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-03-22     Murphy       the first version
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <rtthread.h>
#include <finsh.h>

#include "webclient.h"
#include <fal.h>
#include "http_ota.h"

#define DBG_ENABLE
#define DBG_SECTION_NAME "http_ota"
#ifdef OTA_DOWNLOADER_DEBUG
#define DBG_LEVEL DBG_LOG
#else
#define DBG_LEVEL DBG_INFO
#endif
#define DBG_COLOR
#include <rtdbg.h>

#ifdef PKG_USING_HTTP_OTA

#define HTTP_OTA_BUFF_LEN 4096
#define GET_HEADER_BUFSZ 1024
#define GET_RESP_BUFSZ 1024
#define HTTP_OTA_DL_DELAY (10 * RT_TICK_PER_SECOND)

#define HTTP_OTA_URL PKG_HTTP_OTA_URL
OTA_Struct_pt h_ota = NULL;
static void print_progress(OTA_Struct_pt h_ota)
{
    //   static unsigned char progress_sign[100 + 1];
    //    uint8_t i;

    h_ota->per = (IOT_OTA_Progress_t)(h_ota->size_fetched * 100 / h_ota->size_file);
    if (h_ota->per > IOT_OTAP_FETCH_PERCENTAGE_MAX)
    {
        h_ota->per = IOT_OTAP_FETCH_PERCENTAGE_MAX;
    }

    //    for (i = 0; i < 100; i++)
    //    {
    //        if (i < h_ota->per)
    //        {
    //            progress_sign[i] = '=';
    //        }
    //        else if (h_ota->per == i)
    //        {
    //            progress_sign[i] = '>';
    //        }
    //        else
    //        {
    //            progress_sign[i] = ' ';
    //        }
    //    }

    //  progress_sign[sizeof(progress_sign) - 1] = '\0';
    // mqtt_send_cmd();
    LOG_I("\033[2A");
    LOG_I("Download: [%d%%]", h_ota->per);
}

void *IOT_OTA_Init(void)
{
    OTA_Struct_pt h_ota = NULL;
    if (NULL == (h_ota = rt_malloc(sizeof(OTA_Struct_t))))
    {
        LOG_E("allocate failed");
        return NULL;
    }
    memset(h_ota, 0, sizeof(OTA_Struct_t));
    h_ota->state = IOT_OTAS_UNINITED;
    h_ota->per = IOT_OTAP_FETCH_PERCENTAGE_MIN;

    h_ota->state = IOT_OTAS_INITED;
    return h_ota;
}

/* check whether is downloading */
int IOT_OTA_IsFetching(void)
{
    if (NULL == h_ota)
    {
        LOG_E("handle is NULL");
        return 0;
    }

    if (IOT_OTAS_UNINITED == h_ota->state)
    {
        LOG_E("handle is uninitialized");
        h_ota->err = IOT_OTAE_INVALID_STATE;
        return 0;
    }

    return (IOT_OTAS_FETCHING == h_ota->state);
}
void mqtt_send_cmd(const char *send_str);
#include "network.h"
void http_ota_fw_download_entry(void *parameter)
{
    int ret = 0, resp_status;
    int file_size = 0, length, total_length;
    rt_uint8_t *buffer_read = RT_NULL;
    struct webclient_session *session = RT_NULL;
    const struct fal_partition *dl_part = RT_NULL;
    rt_uint32_t flash_check_temp = 0;
    static rt_uint8_t entry_is_running = 0;
    app_struct_t app_info = parameter;
    rt_uint32_t total, used, max_used;

    rt_tick_t tick_start = 0;
    rt_tick_t tick_used = 0;
    if (entry_is_running == 1)
    {
        LOG_D("ota thread is running!");
        if (app_info)
            rt_free(app_info);
        return;
    }
    entry_is_running = 1;
    if (ota_check_start() == 0) //check and wait until get resurt
    {
        entry_is_running = 0;
        if (app_info)
        {
            rt_free(app_info);
            app_info = RT_NULL
        }
        LOG_D("get check_start resurt");
        return;
    }
    LOG_I("release paho!");
    mqtt_send_cmd("DISCONNECT");
    rt_thread_delay(rt_tick_from_millisecond(500));
__retry:
    rt_memory_info(&total, &used, &max_used);
    LOG_I("\r\ntotal:%d,used:%d,max_used:%d\r\n", total, used, max_used);
    total_length = sizeof(app_struct);
    RT_ASSERT(app_info != RT_NULL);
    LOG_D("app_info not null!");
    if (h_ota)
    {
        rt_free(h_ota);
        h_ota = NULL;
    }
    h_ota = IOT_OTA_Init();
    if (h_ota == NULL)
    {
        LOG_E("IOT_OTA_Init failed!");
        ret = -RT_ERROR;
        goto __exit_retry;
    }
    LOG_D("IOT_OTA_Init ok");

    /* Get download partition information and erase download partition data */
    if ((dl_part = fal_partition_find("download")) == RT_NULL)
    {
        LOG_E("Firmware download failed! Partition (%s) find error!", "download");
        ret = -RT_ERROR;
        goto __exit_retry;
    }

    LOG_I("check and erase flash (%s) partition!", dl_part->name);

    if (fal_partition_read(dl_part, total_length, (uint8_t *)&flash_check_temp, sizeof(rt_uint32_t)) < 0)
    {
        LOG_E("Firmware download failed! Partition (%s) read error!", dl_part->name);
        ret = -RT_ERROR;
        goto __exit_retry;
    }
    rt_thread_delay(rt_tick_from_millisecond(3000));
    LOG_I("Flash (%s) partition check_temp:0x%08x,dl_part->len:0x%08x!", dl_part->name, flash_check_temp, dl_part->len);
    LOG_I("Flash (%s) partition check_temp:0x%08x,dl_part->offset:0x%08x!", dl_part->name, flash_check_temp, dl_part->offset);

    if (flash_check_temp != 0xffffffff)
    {
        if (fal_partition_erase_all(dl_part) < 0)
        {
            LOG_E("Firmware download failed! Partition (%s) erase error!", dl_part->name);
            ret = -RT_ERROR;
            goto __exit_retry;
        }
        LOG_I("Erase flash (%s) partition success!", dl_part->name);
    }
    else
        LOG_I("Flash (%s) partition is already erased!", dl_part->name);

    /* create webclient session and set header response size */
    session = webclient_session_create(GET_HEADER_BUFSZ);
    if (!session)
    {
        LOG_E("open uri failed.");
        ret = -RT_ERROR;
        goto __exit_retry;
    }
    LOG_I("#################request##############", dl_part->name);
    tick_start = rt_tick_get() - tick_used;
    /* send GET request by default header */
    if ((resp_status = webclient_get(session, app_info->url)) != 200)
    {
        LOG_E("webclient GET request failed, response(%d) error.", resp_status);
        ret = -RT_ERROR;
        goto __exit_retry;
    }
    LOG_I("################# request  sucess #############", dl_part->name);
    file_size = webclient_content_length_get(session);
    rt_kprintf("http file_size:%d\n", file_size);

    if (file_size == 0)
    {
        LOG_E("Request file size is 0!");
        ret = -RT_ERROR;
        goto __exit_retry;
    }
    else if (file_size < 0)
    {
        LOG_E("webclient GET request type is chunked.");
        ret = -RT_ERROR;
        goto __exit_retry;
    }

    buffer_read = web_malloc(HTTP_OTA_BUFF_LEN);
    if (buffer_read == RT_NULL)
    {
        LOG_E("No memory for http ota!");
        ret = -RT_ERROR;
        goto __exit_retry;
    }
    memset(buffer_read, 0x00, HTTP_OTA_BUFF_LEN);
    h_ota->size_file = file_size;
    app_info->size = file_size;
    LOG_I("OTA file size is (%d)\r\n", file_size);
    file_size += total_length;
    h_ota->state = IOT_OTAS_FETCHING;

    do
    {
        length = webclient_read(session, buffer_read, file_size - total_length > HTTP_OTA_BUFF_LEN ? HTTP_OTA_BUFF_LEN : file_size - total_length);
        if (length > 0)
        {
            /* Write the data to the corresponding partition address */
            if (fal_partition_write(dl_part, total_length, buffer_read, length) < 0)
            {
                LOG_E("Firmware download failed! Partition (%s) write data error!", dl_part->name);
                ret = -RT_ERROR;
                goto __exit_retry;
            }
            total_length += length;
            h_ota->size_last_fetched = length;
            h_ota->size_fetched += length;
            print_progress(h_ota);
        }
        else
        {
            LOG_E("Exit: server return err (%d)!", length);
            ret = -RT_ERROR;
            h_ota->state = IOT_OTAS_FETCHED;
            h_ota->err = IOT_OTAE_FETCH_FAILED;
            h_ota->per = IOT_OTAP_FETCH_FAILED;
            goto __exit_retry;
        }

    } while (total_length < file_size);

    ret = RT_EOK;
    tick_used = rt_tick_get() - tick_start;

    if (total_length == file_size)
    {
        h_ota->state = IOT_OTAS_FETCHED;
        LOG_D("check    md5 (%s)!", app_info->md5);
        char output_str[33] = {0};

        utils_md5_outstr((const unsigned char *)(dl_part->offset + FLASH_BASE + sizeof(app_struct)), h_ota->size_file, output_str);
        LOG_D("download md5 (%s)!", output_str);

        if (rt_strcasecmp(app_info->md5, output_str) != 0)
        {
            h_ota->per = IOT_OTAP_CHECK_FALIED;
            LOG_E("md5 check err");
            ret = -RT_ERROR;
            goto __exit_retry;
        }
        /* Write the data to the corresponding partition address */

        if (fal_partition_write(dl_part, 0, (const uint8_t *)app_info, sizeof(app_struct)) < 0)
        {
            LOG_E("Firmware download failed! Partition (%s) write data error!", dl_part->name);
            ret = -RT_ERROR;
            goto __exit_retry;
        }

        if (session != RT_NULL)
            webclient_close(session);
        if (buffer_read != RT_NULL)
            web_free(buffer_read);
        LOG_I("*****time.used:%d ms*****", tick_used);
        LOG_I("Download firmware to flash success.");
        LOG_I("System now will restart...");
        ota_done_cb(1); //failed
        rt_thread_delay(rt_tick_from_millisecond(1000));

        /* Reset the device, Start new firmware */
        ota_restart();
    }

__exit_retry:
    if (h_ota != NULL)
    {
        LOG_I("free h_ota....");
        rt_free(h_ota);
        h_ota = NULL;
    }

    if (session != RT_NULL)
    {
        LOG_I("free session....");
        webclient_close(session);
    }
    if (buffer_read != RT_NULL)
    {
        LOG_I("free buffer_read....");
        web_free(buffer_read);
        buffer_read = RT_NULL;
    }
    if (ret != RT_EOK)
    {
        int i = 0;
        if (i++ < 10)
            goto __retry;
    }
__exit:
    if (app_info)
    {
        rt_free(app_info);
        app_info = RT_NULL
    }
    entry_is_running = 0;
    ota_done_cb(0); //failed
    rt_memory_info(&total, &used, &max_used);
    LOG_D("http_ota thread exit");
    LOG_I("\r\ntotal:%d,used:%d,max_used:%d\r\n", total, used, max_used);

    ota_restart();
    mqtt_send_cmd("RESET_OTAFLAG");
}

void http_ota(uint8_t argc, char **argv)
{
    static app_struct_t app_info_p;
    if (argc < 2)
    {
        app_info_p = rt_calloc(1, sizeof(app_struct_t));
        rt_strncpy(app_info_p->url, HTTP_OTA_URL, sizeof(app_info_p->url));
        rt_strncpy(app_info_p->md5, "77FD3C275FFA9B8ACA8A4EA7C39BEA70", sizeof(app_info_p->md5));
        rt_strncpy(app_info_p->version, "01.01.01", sizeof(app_info_p->version));
        app_info_p->size = 374556;
        app_info_p->app_flag = FLASH_APP_FLAG_WORD;

        ota_start(app_info_p);
    }
    else
    {
        //  http_ota_fw_download(argv[1]);
    }
}
/**
 * msh />http_ota [url]
*/
MSH_CMD_EXPORT(http_ota, Use HTTP to download the firmware);

#endif /* PKG_USING_HTTP_OTA */
