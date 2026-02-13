#include "mqtt.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/atomic.h>
#include <net/mqtt_helper.h>
#include <string.h>
#include <stdio.h>

LOG_MODULE_REGISTER(mqtt_app, LOG_LEVEL_INF);

static uint8_t client_id[32];
static atomic_t mqtt_connected  = ATOMIC_INIT(0);
static atomic_t mqtt_connecting = ATOMIC_INIT(0);
static K_SEM_DEFINE(mqtt_connected_sem, 0, 1);
static void mqtt_reconnect_work_fn(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(mqtt_reconnect_work, mqtt_reconnect_work_fn);

static void on_connack(enum mqtt_conn_return_code return_code,
                       bool session_present)
{
    ARG_UNUSED(session_present);
    atomic_clear(&mqtt_connecting);
    if (return_code == MQTT_CONNECTION_ACCEPTED) {
        LOG_INF("MQTT connected [%s]", client_id);
        atomic_set(&mqtt_connected, 1);
        k_sem_give(&mqtt_connected_sem);
    } else {
        LOG_ERR("MQTT connack failed: %d", return_code);
    }
}

static void on_disconnect(int result)
{
    LOG_WRN("MQTT disconnected: %d", result);
    atomic_clear(&mqtt_connected);
    atomic_clear(&mqtt_connecting);

    k_work_schedule(&mqtt_reconnect_work, K_SECONDS(5));
}
static void mqtt_reconnect_work_fn(struct k_work *work)
{
    ARG_UNUSED(work);
    if (!atomic_get(&mqtt_connected) && !atomic_get(&mqtt_connecting)) {
        LOG_INF("MQTT reconnect...");
        mqtt_connect_broker();
    }
}

void mqtt_init(const char *id)
{
    struct mqtt_helper_cfg cfg = {
        .cb = {
            .on_connack    = on_connack,
            .on_disconnect = on_disconnect,
        },
    };
    mqtt_helper_init(&cfg);
    if (id && strlen(id)) {
        strncpy((char *)client_id, id, sizeof(client_id) - 1);
    } else {
        snprintf((char *)client_id,
                 sizeof(client_id),
                 "NRF5340-%08X",
                 sys_rand32_get());
    }
    client_id[sizeof(client_id) - 1] = '\0';
}

void mqtt_connect_broker(void)
{
    if (atomic_get(&mqtt_connected) || atomic_get(&mqtt_connecting))
        return;
    atomic_set(&mqtt_connecting, 1);
    struct mqtt_helper_conn_params params = {
        .hostname.ptr  = CONFIG_MQTT_SAMPLE_BROKER_HOSTNAME,
        .hostname.size = strlen(CONFIG_MQTT_SAMPLE_BROKER_HOSTNAME),
        .device_id.ptr  = (char *)client_id,
        .device_id.size = strlen((char *)client_id),
        .user_name.ptr  = CONFIG_MQTT_SAMPLE_BROKER_USERNAME,
        .user_name.size = strlen(CONFIG_MQTT_SAMPLE_BROKER_USERNAME),
        .password.ptr  = CONFIG_MQTT_SAMPLE_BROKER_PASSWORD,
        .password.size = strlen(CONFIG_MQTT_SAMPLE_BROKER_PASSWORD),
    };
    if (mqtt_helper_connect(&params)) {
        atomic_clear(&mqtt_connecting);
        k_work_schedule(&mqtt_reconnect_work, K_SECONDS(5));
        return;
    }

    k_sem_take(&mqtt_connected_sem, K_SECONDS(10));
}

void mqtt_publish_anomaly(ei_anomaly_msg result)
{
    if (!atomic_get(&mqtt_connected))
        return;
    char payload[128];
    snprintf(payload, sizeof(payload),
             "{\"score\":%.2f,\"threshold\":%.2f,\"status\":\"%s\"}",
             result.score,
             result.threshold,
             result.status);
    struct mqtt_publish_param param = {0};
    param.message.topic.qos = MQTT_QOS_1_AT_LEAST_ONCE;
    param.message.topic.topic.utf8 =
        (uint8_t *)CONFIG_MQTT_SAMPLE_PUB_TOPIC;
    param.message.topic.topic.size =
        strlen(CONFIG_MQTT_SAMPLE_PUB_TOPIC);
    param.message.payload.data = (uint8_t *)payload;
    param.message.payload.len  = strlen(payload);
    param.message_id = mqtt_helper_msg_id_get();
    mqtt_helper_publish(&param);
}
