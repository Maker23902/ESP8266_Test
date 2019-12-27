/*
 * user_main.c
 */

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "user_interface.h"
#include "mem.h"
#include "driver/uart.h"

#include "mqtt/mqtt.h"
#include "mqtt/debug.h"

#include "user_config.h"
#include "aliyun_mqtt.h"
#include "user_wifi.h"

#include "json/exp.h"

//****************************************************************************/
// sdk v3.0.0

#if ((SPI_FLASH_SIZE_MAP == 0) || (SPI_FLASH_SIZE_MAP == 1))
#error "The flash map is not supported"
#elif (SPI_FLASH_SIZE_MAP == 2)
#define SYSTEM_PARTITION_OTA_SIZE 0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR 0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR 0xfb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR 0xfc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR 0xfd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR 0x7c000
#elif (SPI_FLASH_SIZE_MAP == 3)
#define SYSTEM_PARTITION_OTA_SIZE 0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR 0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR 0x1fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR 0x1fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR 0x1fd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR 0x7c000
#elif (SPI_FLASH_SIZE_MAP == 4)
#define SYSTEM_PARTITION_OTA_SIZE 0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR 0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR 0x3fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR 0x3fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR 0x3fd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR 0x7c000
#elif (SPI_FLASH_SIZE_MAP == 5)
#define SYSTEM_PARTITION_OTA_SIZE 0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR 0x101000
#define SYSTEM_PARTITION_RF_CAL_ADDR 0x1fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR 0x1fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR 0x1fd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR 0xfc000
#elif (SPI_FLASH_SIZE_MAP == 6)
#define SYSTEM_PARTITION_OTA_SIZE 0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR 0x101000
#define SYSTEM_PARTITION_RF_CAL_ADDR 0x3fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR 0x3fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR 0x3fd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR 0xfc000
#else
#error "The flash map is not supported"
#endif

#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM SYSTEM_PARTITION_CUSTOMER_BEGIN

uint32 priv_param_start_sec;

static const partition_item_t at_partition_table[] = {
	{SYSTEM_PARTITION_BOOTLOADER, 0x0, 0x1000},
	{SYSTEM_PARTITION_OTA_1, 0x1000, SYSTEM_PARTITION_OTA_SIZE},
	{SYSTEM_PARTITION_OTA_2, SYSTEM_PARTITION_OTA_2_ADDR, SYSTEM_PARTITION_OTA_SIZE},
	{SYSTEM_PARTITION_RF_CAL, SYSTEM_PARTITION_RF_CAL_ADDR, 0x1000},
	{SYSTEM_PARTITION_PHY_DATA, SYSTEM_PARTITION_PHY_DATA_ADDR, 0x1000},
	{SYSTEM_PARTITION_SYSTEM_PARAMETER, SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR, 0x3000},
	{SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM, SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR, 0x1000},
};

void ICACHE_FLASH_ATTR user_pre_init(void)
{
	if (!system_partition_table_regist(
			at_partition_table,
			sizeof(at_partition_table) / sizeof(at_partition_table[0]),
			SPI_FLASH_SIZE_MAP))
	{
		while (1)
			;
	}
}

//****************************************************************************/

MQTT_Client mqttClient;

// topic
#define BASE_TOPIC "/" PRODUCT_KEY "/" DEVICE_NAME
#define GET_TOPIC BASE_TOPIC "/get"
#define UPDATE_TOPIC BASE_TOPIC "/update"


#define ALINK_BODY_FORMAT "{\"id\":\"123\",\"version\":\"1.0\",\"method\":\"%s\",\"params\":%s}"
#define ALINK_TOPIC_PROP_POST "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/event/property/post"
#define ALINK_TOPIC_PROP_SET "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/service/property/set"
#define ALINK_METHOD_PROP_POST "thing.event.property.post"


os_timer_t uploadataTimer;
void wifiConnectCb(uint8_t status)
{
	if (status == STATION_GOT_IP)
	{
		MQTT_Connect(&mqttClient);
	}
	else
	{
		MQTT_Disconnect(&mqttClient);
	}
}

void mqttConnectedCb(uint32_t *args)
{
	MQTT_Client *client = (MQTT_Client *)args;
	INFO("MQTT: Connected\r\n");

	MQTT_Subscribe(client, GET_TOPIC, 0);

	os_timer_arm(&uploadataTimer, 5000, true);
	INFO("timer 3000 start!\r\n");

}

void mqttDisconnectedCb(uint32_t *args)
{
	MQTT_Client *client = (MQTT_Client *)args;
	INFO("MQTT: Disconnected\r\n");
}

void mqttPublishedCb(uint32_t *args)
{
	MQTT_Client *client = (MQTT_Client *)args;


	INFO("MQTT: Published\r\n");
}

void mqttDataCb(uint32_t *args, const char *topic, uint32_t topic_len,
				const char *data, uint32_t data_len)
{
	char *topicBuf = (char *)os_zalloc(topic_len + 1), *dataBuf =
														   (char *)os_zalloc(data_len + 1);

    cJSON *fmt,*fmt_params,*fmt_VerticalSwitch,*fmt_TargetTemperature,*fmt_PowerSwitch,*fmt_WindSpeed,*fmt_WorkMode,*fmt_Error;
	MQTT_Client *client = (MQTT_Client *)args;


	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;

	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;

	INFO("hello xxxxx\n");


	fmt = cJSON_Parse(data);INFO("006\n");
	if(!fmt) {
		INFO("no fmt!\n");
		return ;
		}
	INFO("007\n");
	//fmt_VerticalSwitch = cJSON_GetObjectItem( fmt , "VerticalSwitch" );

	fmt_params = cJSON_GetObjectItem( fmt , "params" );
	fmt_TargetTemperature = cJSON_GetObjectItem( fmt_params , "TargetTemperature" );INFO("008\n");
	//fmt_CurrentTemperature = cJSON_GetObjectItem( fmt , "CurrentTemperature" );INFO("008\n");
	fmt_PowerSwitch = cJSON_GetObjectItem( fmt_params , "PowerSwitch" );
	fmt_WindSpeed = cJSON_GetObjectItem( fmt_params , "WindSpeed" );
	fmt_WorkMode = cJSON_GetObjectItem( fmt_params , "WorkMode" );
	fmt_Error = cJSON_GetObjectItem( fmt_params , "Error" );


	cJSON_Delete(fmt);
	INFO("00a\n");


	os_free(topicBuf);
	os_free(dataBuf);
}


uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
	enum flash_size_map size_map = system_get_flash_size_map();
	uint32 rf_cal_sec = 0;

	switch (size_map)
	{
	case FLASH_SIZE_4M_MAP_256_256:
		rf_cal_sec = 128 - 5;
		break;

	case FLASH_SIZE_8M_MAP_512_512:
		rf_cal_sec = 256 - 5;
		break;

	case FLASH_SIZE_16M_MAP_512_512:
	case FLASH_SIZE_16M_MAP_1024_1024:
		rf_cal_sec = 512 - 5;
		break;

	case FLASH_SIZE_32M_MAP_512_512:
	case FLASH_SIZE_32M_MAP_1024_1024:
		rf_cal_sec = 1024 - 5;
		break;

	default:
		rf_cal_sec = 0;
		break;
	}

	return rf_cal_sec;
}

void udata(uint32_t *args)
{
	MQTT_Client *client = (MQTT_Client *)args;
	char param[128];
	char fmtBuf[256];
	uint8_t len;

	static uint8_t temp=16;
	if(temp>30) temp=16;

	os_sprintf(param, "{\"TargetTemperature\":%d}", temp++);
	len=os_sprintf(fmtBuf, ALINK_BODY_FORMAT, ALINK_METHOD_PROP_POST, param);
	INFO(fmtBuf);
	//MQTT_Publish(ALINK_TOPIC_PROP_POST, fmtBuf);

	MQTT_Publish(client, ALINK_TOPIC_PROP_POST, fmtBuf,len, 0, 0);

}
/*****************************************************************************/

void user_init(void)
{

	uart_init(BIT_RATE_115200, BIT_RATE_115200);

	aliyun_mqtt_init();
	MQTT_InitConnection(&mqttClient, g_aliyun_mqtt.host, g_aliyun_mqtt.port, 0);

	MQTT_InitClient(&mqttClient, g_aliyun_mqtt.client_id, g_aliyun_mqtt.username,
					g_aliyun_mqtt.password, g_aliyun_mqtt.keepalive, 1);
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	MQTT_OnData(&mqttClient, mqttDataCb);
	wifi_connect(wifiConnectCb);
	os_timer_disarm(&uploadataTimer);
	os_timer_setfn(&uploadataTimer, (os_timer_func_t *) udata,&mqttClient);

	INFO("\r\nSystem started ...\r\n");
}
