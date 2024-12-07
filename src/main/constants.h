/*
 * Program wide constants/defaults in one easily accessible place
 */
#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

#define DEFAULT_FREEZE_DANGER_TEMP_C 0
#define RELAY_PERIOD_SCALING_CONSTANT 0.1
#define CIRC_ON_TICKS 60 * configTICK_RATE_HZ  // 1 min
#define MAX_CIRC_INTERVAL_S 60.0 * 60.0        // 60 min

#define RELAY_PIN 33
#define RELAY_TASK_INTERVAL_TICKS 1000 / portTICK_PERIOD_MS

#define TEMP_SENSOR_PIN 15

#define SAMPLE_PERIOD_TICKS 60 * configTICK_RATE_HZ  // 1 min

#define TEMP_SAMPLE_PERIOD_TICKS 60 * configTICK_RATE_HZ  // 1 min

#define LED_PIN 2
#define LED_ON_TICKS 250 / portTICK_PERIOD_MS
#define NORMAL_HEARTBEAT_TICKS 5000 / portTICK_PERIOD_MS
#define FREEZE_DANGER_HEARTBEAT_TICKS 1000 / portTICK_PERIOD_MS

#define RELAY_ACTIVATED_LED_ON_TICKS 125 / portTICK_PERIOD_MS
#define RELAY_ACTIVATED_HEARTBEAT_TICKS 250 / portTICK_PERIOD_MS

#define MDNS_HOSTNAME "antifreeze"
#define MDNS_SERVICENAME "antifreeze"
#define MDNS_INSTANCE_NAME "antifreeze"

#define TIME_ZONE "EST5EDT,M3.2.0,M11.1.0"
#define THE_END_OF_TIME LONG_MAX
#define NTP_SERVER "pool.ntp.org"

// Overrides for test
#if CONFIG_TEST_MODE

#define CIRC_ON_TICKS 15 * configTICK_RATE_HZ  // 15s
#define MAX_CIRC_INTERVAL_S 60.0               // 30s
#define TEMP_SAMPLE_PERIOD_TICKS 30 * configTICK_RATE_HZ

#else

// If set to false in the menu, it is left undefined, so we have to do this
#define CONFIG_TEST_MODE false

#endif

#endif
