#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

#define T_FREEZE_DANGER_DEFAULT_C 0
#define K 1.5
#define RELAY_ON_TIME_S 60

#define RELAY_PIN 33
#define TEMP_SENSOR_PIN 34

#define RELAY_SAMPLE_PERIOD_MS 1000 / portTICK_PERIOD_MS
#define TEMP_SAMPLE_PERIOD_MS 1000 / portTICK_PERIOD_MS

#define LED_PIN 2
#define LED_ON_PERIOD_MS 250 / portTICK_PERIOD_MS
#define NORMAL_HEARTBEAT_PERIOD_MS 5000 / portTICK_PERIOD_MS
#define FREEZE_DANGER_HEARTBEAT_PERIOD_MS 1000 / portTICK_PERIOD_MS
#define RELAY_ACTIVATED_HEARTBEAT_PERIOD_MS 500 / portTICK_PERIOD_MS

#define MDNS_HOSTNAME antifreeze
#define MDNS_SERVICENAME antifreeze

#define TIME_ZONE "EST5EDT,M3.2.0,M11.1.0"

#endif
