/*
 * aliyun_mqtt.c
 */

#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "mem.h"

#include "driver/uart.h"

//#include "user_config.h"
#include "aliyun_mqtt.h"

#include "md5.h"

/****************************************************************/
#define DEBUG	1

#ifdef DEBUG
#define debug(fmt, args...) os_printf(fmt, ##args)
#define debugX(level, fmt, args...) if(DEBUG>=level) os_printf(fmt, ##args);
#else
#define debug(fmt, args...)
#define debugX(level, fmt, args...)
#endif /* DEBUG */

/****************************************************************/
// global variable
s_mqtt g_aliyun_mqtt;

static u16 ICACHE_FLASH_ATTR
gen_mqtt_password(u8 input_timestamp_3[], u8 output_pass[]) {

#define HASH_STR	"clientId"DEVICE_ID"deviceName"DEVICE_NAME"productKey"PRODUCT_KEY"timestamp%s"
#define HASH_KEY	DEVICE_SECRET

	int i;
	u8 temp[8];
	u8 output[16];
	u8 *p = output_pass;
	u8 *pStr = os_malloc(os_strlen(HASH_STR)+os_strlen(input_timestamp_3));

	os_sprintf(pStr, HASH_STR, input_timestamp_3);
	debug("pStr:%s\r\n", pStr);

	HMAC_MD5(pStr, os_strlen(pStr), HASH_KEY, output);

	for (i = 0; i < 16; i++) {
		if (output[i] < 0x10) {
			os_memset(temp, 0, sizeof(temp));
			os_sprintf(temp, "%d", 0);
			os_strcat(p, temp);
		}
		os_memset(temp, 0, sizeof(temp));
		os_sprintf(temp, "%x", output[i]);
		os_strcat(p, temp);
	}
	debug("pass:%s\r\n", output_pass);

	os_free(pStr);

	return os_strlen(output_pass);
}

/*
 * function: aliyun_mqtt_init
 *
 */
void ICACHE_FLASH_ATTR
aliyun_mqtt_init(void) {

#ifdef SNTP_TIMESTAMP
	u8 *timestamp_str = TIMESTAMP_STR;
#elif defined(RAMDOM_TIMESTAMP)
	u8 timestamp_str[4] = { 0 };
	u32 number = os_random();
	os_sprintf(timestamp_str, "%d", number % 1000);
	timestamp_str[3] = '\0';
#else
	u8 *timestamp_str = TIMESTAMP_STR;
#endif

	os_strncpy(g_aliyun_mqtt.host, MQTT_HOST, BUF_SIZE);

	g_aliyun_mqtt.port = MQTT_PORT;

	os_sprintf(g_aliyun_mqtt.client_id, MQTT_CLIENT_ID, timestamp_str);

	os_strncpy(g_aliyun_mqtt.username, MQTT_USERNAME, BUF_SIZE);

	gen_mqtt_password(timestamp_str, g_aliyun_mqtt.password);

	g_aliyun_mqtt.keepalive = 120;


}

/*
 * function: test_hmac_md5
 * description: hmacmd5测试，成功
 */
void ICACHE_FLASH_ATTR
test_hmac_md5(void) {



}

