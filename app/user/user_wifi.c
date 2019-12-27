/*
 * user_wifi.c
 */

#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "os_type.h"
#include "mem.h"

#include "user_wifi.h"

//*****************************************************************************/


#define DEBUG	1


#define WIFI_CHECK_TIMER_INTERVAL	(2*1000)
#define WIFI_CLOSE_CHECK_TIMER_INTERVAL		(500)

//*****************************************************************************/
#define PR	os_printf

#ifdef DEBUG
#define debug(fmt, args...) PR(fmt, ##args)
#define debugX(level, fmt, args...) if(DEBUG>=level) PR(fmt, ##args);
#else
#define debug(fmt, args...)
#define debugX(level, fmt, args...)
#endif	/* DEBUG */

//*****************************************************************************/
// gloabl variable
static os_timer_t g_wifi_check_timer;




WifiCallback wifiCb = NULL;

static u8 wifiStatus = STATION_IDLE, lastWifiStatus = STATION_IDLE;


static void ICACHE_FLASH_ATTR
wifi_handle_event_cb(System_Event_t *evt) {

#if 0
	debug("event %x\n", evt->event);
	switch (evt->event) {
		case EVENT_STAMODE_CONNECTED:
		debug("connect to ssid %s, channel %d\n",
				evt->event_info.connected.ssid,
				evt->event_info.connected.channel);
		break;
		case EVENT_STAMODE_DISCONNECTED:
		debug("disconnect from ssid %s, reason %d\n",
				evt->event_info.disconnected.ssid,
				evt->event_info.disconnected.reason);
		break;
		case EVENT_STAMODE_AUTHMODE_CHANGE:
		debug("mode: %d -> %d\n", evt->event_info.auth_change.old_mode,
				evt->event_info.auth_change.new_mode);
		break;
		case EVENT_STAMODE_GOT_IP:
		debug("ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR,
				IP2STR(&evt->event_info.got_ip.ip),
				IP2STR(&evt->event_info.got_ip.mask),
				IP2STR(&evt->event_info.got_ip.gw));
		debug("\n");
		break;
		case EVENT_SOFTAPMODE_STACONNECTED:
		debug("station:	" MACSTR "join,	AID	=	%d\n",
				MAC2STR(evt->event_info.sta_connected.mac),
				evt->event_info.sta_connected.aid);
		break;
		case EVENT_SOFTAPMODE_STADISCONNECTED:
		debug("station:	" MACSTR "leave, AID	=	%d\n",
				MAC2STR(evt->event_info.sta_disconnected.mac),
				evt->event_info.sta_disconnected.aid);
		break;
		default:
		break;
	}
#endif
}


static void ICACHE_FLASH_ATTR
wifi_check_timer_cb(void) {
	struct ip_info ipConfig;

	wifi_get_ip_info(STATION_IF, &ipConfig);
	wifiStatus = wifi_station_get_connect_status();

	if (wifiStatus == STATION_GOT_IP && ipConfig.ip.addr != 0) {
		wifi_check_init(WIFI_CHECK_TIMER_INTERVAL);

	} else {
		debug("wifi connect fail!\r\n");
		wifi_check_init(WIFI_CLOSE_CHECK_TIMER_INTERVAL);
	}


	if (wifiStatus != lastWifiStatus) {
		lastWifiStatus = wifiStatus;
		if (wifiCb) {
			wifiCb(wifiStatus);
		}
	}
}


void ICACHE_FLASH_ATTR
wifi_check_init(u16 interval) {
	wifi_station_set_reconnect_policy(TRUE);
	wifi_set_event_handler_cb(wifi_handle_event_cb);

	os_timer_disarm(&g_wifi_check_timer);
	os_timer_setfn(&g_wifi_check_timer, (os_timer_func_t *) wifi_check_timer_cb, NULL);
	os_timer_arm(&g_wifi_check_timer, interval, 0);
}


void ICACHE_FLASH_ATTR
user_set_station_config(u8* ssid, u8* password) {
	struct station_config stationConf;
	stationConf.bssid_set = 0;
	os_memcpy(&stationConf.ssid, ssid, 32);
	os_memcpy(&stationConf.password, password, 64);
	wifi_station_set_config(&stationConf);
}



void ICACHE_FLASH_ATTR
wifi_connect(WifiCallback cb) {
	wifi_set_opmode(STATION_MODE);
	wifiCb = cb;

	wifi_station_set_auto_connect(TRUE);
	user_set_station_config(WIFI_SSID, WIFI_PASS);
	wifi_station_disconnect();
	wifi_station_connect();
	wifi_check_init(WIFI_CHECK_TIMER_INTERVAL);

}

