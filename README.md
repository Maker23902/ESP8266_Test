# Support Policy for ESP8266 NonOS

Starting from December 2019, 

* We will not add any new features to the ESP8266 NonOS SDK.
* We will only fix critical bugs in the ESP8266 NonOS SDK.
* We will only maintain the master branch of ESP8266 NonOS SDK, which is a continuously bug-fix version based on v3.0. This means:
	* All other released branches will not be updated.
	* All the future versions will be released from only the master branch mentioned above.
* It is suggested that the [ESP8266_RTOS_SDK](https://github.com/espressif/ESP8266_RTOS_SDK), instead of ESP8266 NonOS SDK, be used for your projects.

本例实现了通过wifi连接到阿里云平台，通过app可以接收模块上传的数据，另外也可以通过app下发指令控制模块