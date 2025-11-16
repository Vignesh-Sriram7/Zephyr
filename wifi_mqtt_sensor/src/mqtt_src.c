#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/logging/log.h>
#include <zephyr/types.h>
#include <stdint.h>
#include <arpa/inet.h>

LOG_MODULE_REGISTER(mqtt_module, LOG_LEVEL_DBG);

/* MQTT broker info */
#define MQTT_BROKER_HOSTNAME "broker.emqx.io"
#define MQTT_BROKER_PORT     1883
#define MQTT_CLIENT_ID       "esp32_test"

/* MQTT RX/TX buffers */
static uint8_t rx_buffer[128];
static uint8_t tx_buffer[128];

struct mqtt_client client_ctx;
static struct sockaddr_storage broker;

bool mqtt_connected = false;


static void mqtt_event_handler(struct mqtt_client *const client,
                               const struct mqtt_evt *evt)
{
    switch (evt->type) {
    case MQTT_EVT_CONNACK:
        if (evt->result == 0) {
            mqtt_connected = true;
            printk("MQTT connected (CONNACK)\n");
        } else {
            printk("MQTT CONNACK error: %d\n", evt->result);
        }
        break;

    case MQTT_EVT_PUBACK:
        printk("PUBACK received\n");
        break;

    default:
        break;
    }
}


void mqtt_init(void)
{
    mqtt_client_init(&client_ctx);
    memset(&broker, 0, sizeof(broker));

    /* Basic client configuration */
    client_ctx.broker = &broker;
    client_ctx.evt_cb = mqtt_event_handler;

    client_ctx.client_id.utf8 = (uint8_t *)MQTT_CLIENT_ID;
    client_ctx.client_id.size = strlen(MQTT_CLIENT_ID);
    client_ctx.user_name = NULL;
    client_ctx.password = NULL;
    client_ctx.protocol_version = MQTT_VERSION_3_1_1;

    client_ctx.keepalive = 60;

    client_ctx.transport.type = MQTT_TRANSPORT_NON_SECURE;
    client_ctx.transport.tcp.sock = -1;

    client_ctx.rx_buf = rx_buffer;
    client_ctx.rx_buf_size = sizeof(rx_buffer);
    client_ctx.tx_buf = tx_buffer;
    client_ctx.tx_buf_size = sizeof(tx_buffer);

    /* DNS lookup */
    struct zsock_addrinfo *res;
    struct zsock_addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };

    int rc = zsock_getaddrinfo(MQTT_BROKER_HOSTNAME, NULL, &hints, &res);
    if (rc == 0 && res != NULL) {
        struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
        memcpy(&broker, addr, sizeof(struct sockaddr_in));
        ((struct sockaddr_in *)&broker)->sin_port = htons(MQTT_BROKER_PORT);

        zsock_freeaddrinfo(res);
        LOG_INF("Broker resolved: %s", MQTT_BROKER_HOSTNAME);
    } else {
        LOG_ERR("DNS resolution failed: %d", rc);
    }
}


void app_mqtt_connect(struct mqtt_client *client_ctx)
{
    int rc = mqtt_connect(client_ctx);
    if (rc != 0) {
        LOG_ERR("MQTT Connect failed [%d]", rc);
        return;
    }

    LOG_INF("Waiting for CONNACK...");

    /* Pump MQTT state machine until CONNACK arrives */
    while (!mqtt_connected) {
        mqtt_input(client_ctx);
        mqtt_live(client_ctx);
        k_msleep(50);
    }

    LOG_INF("MQTT Connected and ready to publish!");
}


int app_mqtt_publish(struct mqtt_client *client_ctx, const char *topic_str, const char *payload)
{
    struct mqtt_publish_param param;
    struct mqtt_topic topic = {
        .topic = {
            .utf8 = (uint8_t *)topic_str,
            .size = strlen(topic_str)
        },
        .qos = MQTT_QOS_0_AT_MOST_ONCE
    };

    param.message.topic = topic;
    param.message.payload.data = (uint8_t *)payload;
    param.message.payload.len = strlen(payload);
    param.message_id = k_uptime_get_32();
    param.dup_flag = 0;
    param.retain_flag = 0;

    int rc = mqtt_publish(client_ctx, &param);
    if (rc != 0) {
        LOG_ERR("MQTT Publish failed [%d]", rc);
    } else {
        LOG_INF("Published message to topic '%s'",
                param.message.topic.topic.utf8);
    }

    return rc;
}

void mqtt_process_loop(void)
{
    static uint32_t last_publish = 0;

    while (1) {
        mqtt_input(&client_ctx);
        mqtt_live(&client_ctx);

        uint32_t now = k_uptime_get_32();

        // Publish every 5 seconds
        if (mqtt_connected && (now - last_publish > 5000)) {
            const char *topic = "esp32/test";
            const char *payload = "Hello from ESP32";

            app_mqtt_publish(&client_ctx, topic, payload);

            last_publish = now;
        }

        k_msleep(100); // small sleep to avoid busy loop
    }
}