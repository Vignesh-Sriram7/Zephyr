#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/logging/log.h>
#include <zephyr/types.h>
#include <stdint.h>
#include <arpa/inet.h>

LOG_MODULE_REGISTER(mqtt_module, LOG_LEVEL_DBG);

// Define MQTT broker name and details 
#define MQTT_BROKER_HOSTNAME "broker.emqx.io"
#define MQTT_BROKER_PORT 1883
#define MQTT_CLIENT_ID "esp32_test"

#define MSECS_WAIT_RECONNECT 5000 
//Buffers for MQTTT client
// used to store incoming messages before processing and outgoing messages before being sent

static uint8_t rx_buffer[128];
static uint8_t tx_buffer[128];

// CLient context. Holds the state and cofig of the MQTT client
struct mqtt_client client_ctx;

// MQTT Broker address information.
static struct sockaddr_storage broker;

bool mqtt_connected = false;

static void mqtt_event_handler(struct mqtt_client *const client,
                               const struct mqtt_evt *evt)
{
    if (evt->type == MQTT_EVT_CONNACK && evt->result == 0) {
        mqtt_connected = true;
        printk("MQTT connected (CONNACK)\n");
    }
}



void app_mqtt_connect(struct mqtt_client *client_ctx)
{
	int sock = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
if (sock < 0) {
    LOG_ERR("Socket create failed: %d", errno);
    return;
}

int res = zsock_connect(sock, (struct sockaddr *)&broker, sizeof(struct sockaddr_in));
if (res == 0) {
    LOG_INF("TCP connection to broker succeeded!");
} else {
    LOG_ERR("TCP connection to broker failed: errno=%d", errno);
}
zsock_close(sock);
    int rc = mqtt_connect(client_ctx);
    if (rc != 0) {
        LOG_ERR("MQTT Connect failed [%d]", rc);
        return;
    }

    LOG_INF("Waiting for CONNACK...");

    // Wait until CONNACK event sets mqtt_connected = true
    while (!mqtt_connected) {
        k_msleep(100);
    }

    LOG_INF("MQTT Connected and ready to publish!");
}


int app_mqtt_publish(struct mqtt_client *client_ctx)
{
	int rc;
	struct mqtt_publish_param param;
	struct mqtt_topic topic = {
		.topic = {
			.utf8 = (uint8_t *)"esp32/test",
			.size = strlen("esp32/test")
		},
		.qos = 0
	};

	param.message.topic = topic;
	param.message.payload.data = (uint8_t *)"Hello from ESP32";
    param.message.payload.len = strlen("Hello from ESP32");
	param.message_id = k_uptime_get_32();
	param.dup_flag = 0;
	param.retain_flag = 0;

	rc = mqtt_publish(client_ctx, &param);
	if (rc != 0) {
		LOG_ERR("MQTT Publish failed [%d]", rc);
	}

	LOG_INF("Published to topic '%s', QoS %d",
			param.message.topic.topic.utf8,
			param.message.topic.qos);

	return rc;
}


//Initialize MQTT
void mqtt_init(void){
    mqtt_client_init(&client_ctx);

    struct sockaddr_in *broker_test;

    /* MQTT client configuration */
    client_ctx.broker = &broker;
    client_ctx.evt_cb = mqtt_event_handler;
    client_ctx.client_id.utf8 = (uint8_t *)MQTT_CLIENT_ID;
    client_ctx.client_id.size = strlen(MQTT_CLIENT_ID);
    client_ctx.password = NULL;
    client_ctx.user_name = NULL;
    client_ctx.protocol_version = MQTT_VERSION_3_1_1;
    client_ctx.transport.type = MQTT_TRANSPORT_NON_SECURE;

    /* MQTT buffers configuration */
    client_ctx.rx_buf = rx_buffer;
    client_ctx.rx_buf_size = sizeof(rx_buffer);
    client_ctx.tx_buf = tx_buffer;
    client_ctx.tx_buf_size = sizeof(tx_buffer);

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