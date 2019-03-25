/**
 ****************************************************************************
 * @Warning :Without permission from the author,Not for commercial use
 * @File    :
 * @Author  :Seblee
 * @date    :2019-3-22 15:09:32
 * @version :V1.0.0
 *************************************************
 * @brief   :
 ****************************************************************************
 * @Last Modified by: Seblee
 * @Last Modified time: 2019-3-22 15:09:45
 ****************************************************************************
**/
#ifndef __OTA_H_
#define __OTA_H_
/* Private include -----------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
/* Private define ------------------------------------------------------------*/
#define FLASH_APP_FLAG_WORD 0xa5a5
/* Private typedef -----------------------------------------------------------*/
typedef struct
{
    uint16_t app_flag;
    uint32_t size;
    char md5[33];
    char version[10];
    char url[256];
} app_struct;
typedef app_struct *app_struct_t;
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
int ota_start(app_struct_t app_info_p);
void http_ota_fw_download_entry(void *parameter);
/*----------------------------------------------------------------------------*/
#endif
