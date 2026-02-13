#pragma once
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    float score;          // anomaly distance
    float threshold; 
    const char *status;   // "FAULT" or "OK"
} ei_anomaly_msg;
void mqtt_init(const char *client_id);
void mqtt_connect_broker(void);
void mqtt_publish_anomaly(ei_anomaly_msg result);
#ifdef __cplusplus
}
#endif
