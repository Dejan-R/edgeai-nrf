/* 
   Edge AI – Industrial Motor Anomaly Detection

   Platform: nRF5340 + Zephyr RTOS + https://mlstudio.nordicsemi.com/ (Edge Impulse)

 Features:
 - Deterministically samples vibration data at 100 Hz
 - Acquires raw 3-axis (XYZ) acceleration data
 - Fills a linear buffer corresponding to a single AI inference window
 - Executes the Edge Impulse DSP + ML pipeline (FFT + K-means anomaly detection)
 - Performs on-device decision making (OK / FAULT)
 - Publishes the result via MQTT
 - Uses a hardware watchdog for system safety and reliability
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/watchdog.h>
#include "mpu6050/mpu6050.h"
#include "mqtt/mqtt.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

/* Zephyr logging modul */
LOG_MODULE_REGISTER(edgeai_motor, LOG_LEVEL_INF);

/*  EDGE IMPULSE settings:
    These values come directly from the Edge Impulse model:
    EI_CLASSIFIER_FREQUENCY        - Sampling frequency (Hz) expected by the model
    EI_CLASSIFIER_RAW_SAMPLE_COUNT - Number of raw samples per axis
    Total samples (X, Y, Z)        - 3 * RAW_SAMPLE_COUNT
*/
#define SAMPLE_RATE_HZ EI_CLASSIFIER_FREQUENCY
#define RAW_COUNT      EI_CLASSIFIER_RAW_SAMPLE_COUNT
#define RAW_SIZE       (RAW_COUNT * 3)

// FAULT POSTAVKE
#define FIXED_THRESHOLD      300.0f  // Distance threshold (anomaly limit)
#define CONSECUTIVE_FAULTS   3       // Number of consecutive detections required to confirm a fault
#define HEARTBEAT_SECONDS  10

//RAW BUFFER - linear - format: [x0,y0,z0, x1,y1,z1, ...] */
static float raw_buffer[RAW_SIZE];
static size_t raw_idx = 0;

/* Edge Impulse CALLBACK
   The model does not read the sensor directly — it requests data via get_data().
   Here we simply copy data from the buffer into the internal Edge Impulse memory.
*/
static int get_data(size_t offset, size_t length, float *out)
{
    memcpy(out, raw_buffer + offset, length * sizeof(float));
    return 0;
}

/* Deterministic sampling (100 Hz):
   Timer → semaphore → main loop
*/
K_SEM_DEFINE(sample_sem, 0, 1);
/* Timer interrupt - sample */
static void timer_cb(struct k_timer *t) { k_sem_give(&sample_sem); }
K_TIMER_DEFINE(sample_timer, timer_cb, NULL);

/*  WATCHDOG  */
#define WDT_FEED_INTERVAL_MS 1000
static const struct device *wdt_dev = nullptr;
static int wdt_channel_id = -1;

static void watchdog_init(void)
{
    wdt_dev = device_get_binding("WDT_0");
    if (!wdt_dev) {
        LOG_WRN("Watchdog device not found, continuing without it");
        return;
    }
    struct wdt_timeout_cfg wdt_cfg;
    wdt_cfg.window.min = 0;
    wdt_cfg.window.max = WDT_FEED_INTERVAL_MS;
    wdt_cfg.callback = nullptr;
    wdt_cfg.flags = WDT_FLAG_RESET_SOC;
    wdt_channel_id = wdt_install_timeout(wdt_dev, &wdt_cfg);
    if (wdt_channel_id < 0) {
        LOG_ERR("Watchdog install failed, continuing without it");
        return;
    }
    wdt_setup(wdt_dev, WDT_OPT_PAUSE_HALTED_BY_DBG);
}

int main(void)
{
    LOG_INF("EdgeAI Detecting Anomalies");

    mqtt_init("DC_Motor_185RPM");
    mqtt_connect_broker();
    mpu6050_init();   // accelerometer
    watchdog_init();  //Watchdog
  /* Start of periodic sampling */
    k_timer_start(&sample_timer, K_MSEC(1000 / SAMPLE_RATE_HZ),
                  K_MSEC(1000 / SAMPLE_RATE_HZ));

/* Linking the buffer with the Edge Impulse model */
    signal_t signal;
    signal.total_length = RAW_SIZE;
    signal.get_data = get_data;

    ei_impulse_result_t result;
    uint8_t fault_counter = 0;
    bool fault_active = false;
    int64_t last_pub = 0;

    while (1) {
        /* Wait for exactly one sample (100 Hz) */

        k_sem_take(&sample_sem, K_FOREVER);

      /* Reading data from the sensor */
        float x, y, z;
        if (!mpu6050_read(&x, &y, &z))
            continue;
      /* Values already in g units (scaled) */

        x /= 16384.f;
        y /= 16384.f;
        z /= 16384.f;
            /* Filling the RAW buffer */
        raw_buffer[raw_idx++] = x;
        raw_buffer[raw_idx++] = y;
        raw_buffer[raw_idx++] = z;

      /* No inference until the entire frame is filled */
              if (raw_idx < RAW_SIZE)
            continue;
        raw_idx = 0;

        /* RUN EDGE IMPULSE  - Infrence
          Pipeline:
           raw > FFT > features > anomaly model*/
        if (run_classifier(&signal, &result, false) != EI_IMPULSE_OK)
            continue;

      /* Anomaly detection:
   K-means provides a distance value
   Higher distance indicates a greater anomaly */
        float distance = result.anomaly;
        bool anomaly = (distance > FIXED_THRESHOLD);
        LOG_INF("score=%.2f %s", distance, anomaly ? "FAULT" : "OK");

        /* Fault confirmation (FAULT) */
        if (anomaly)
            fault_counter++;
        else
            fault_counter = 0;

        bool confirmed_fault = (fault_counter >= CONSECUTIVE_FAULTS);
        int64_t now = k_uptime_get();

        /* MQTT communication */
        if (confirmed_fault && !fault_active) {
            mqtt_publish_anomaly({
                .score     = distance,
                .threshold = FIXED_THRESHOLD,
                .status    = "FAULT"
            });
            fault_active = true;
            last_pub = now;
        }
        else if (!confirmed_fault && (now - last_pub > HEARTBEAT_SECONDS * 1000)) {
            mqtt_publish_anomaly({
                .score     = distance,
                .threshold = FIXED_THRESHOLD,
                .status    = "OK"
            });
            fault_active = false;
            last_pub = now;
        }

        /*  WATCHDOG FEED  */
        if (wdt_dev && wdt_channel_id >= 0)
            wdt_feed(wdt_dev, wdt_channel_id);
    }
}
