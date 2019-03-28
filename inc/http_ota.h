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
#ifndef __HTTP_OTA_H_
#define __HTTP_OTA_H_
/* Private include -----------------------------------------------------------*/
#include "ota.h"
#include "utils_md5.h"
/* Private define ------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
/* State of OTA */
typedef enum
{
    IOT_OTAS_UNINITED = 0, /* Uninitialized State */
    IOT_OTAS_INITED,       /* Initialized State */
    IOT_OTAS_FETCHING,     /* Fetching firmware */
    IOT_OTAS_FETCHED       /* Fetching firmware finish */
} IOT_OTA_State_t;

typedef enum
{
    IOT_OTAE_GENERAL = -1,
    IOT_OTAE_INVALID_PARAM = -2,
    IOT_OTAE_INVALID_STATE = -3,
    IOT_OTAE_STR_TOO_LONG = -4,
    IOT_OTAE_FETCH_FAILED = -5,
    IOT_OTAE_NOMEM = -6,
    IOT_OTAE_OSC_FAILED = -7,
    IOT_OTAE_NONE = 0,
} IOT_OTA_Err_t;
/* Progress of OTA */
typedef enum
{
    /* Burn firmware file failed */
    IOT_OTAP_BURN_FAILED = -4,
    /* Check firmware file failed */
    IOT_OTAP_CHECK_FALIED = -3,
    /* Fetch firmware file failed */
    IOT_OTAP_FETCH_FAILED = -2,
    /* Initialized failed */
    IOT_OTAP_GENERAL_FAILED = -1,
    /* [0, 100], percentage of fetch progress */

    /* The minimum percentage of fetch progress */
    IOT_OTAP_FETCH_PERCENTAGE_MIN = 0,
    /* The maximum percentage of fetch progress */
    IOT_OTAP_FETCH_PERCENTAGE_MAX = 100
} IOT_OTA_Progress_t;

typedef struct
{
    const char *product_key; /* point to product key */
    const char *device_name; /* point to device name */

    uint32_t id;                /* message id */
    IOT_OTA_State_t state;      /* OTA state */
    uint32_t size_last_fetched; /* size of last downloaded */
    uint32_t size_fetched;      /* size of already downloaded */
    uint32_t size_file;         /* size of file */
    IOT_OTA_Progress_t per;
     MD5ctx_stt md5; /* MD5 handle */ 

    int err; /* last error code */

} OTA_Struct_t, *OTA_Struct_pt;
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
void *IOT_OTA_Init(void);

/* Private functions ---------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
#endif
