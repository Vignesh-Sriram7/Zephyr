#ifndef MQTT_SRC_H
#define MQTT_SRC_H


void app_mqtt_connect(struct mqtt_client *client_ctx);
int app_mqtt_publish(struct mqtt_client *client_ctx,
                     const char *topic_str,
                     const char *payload);
int wifi_connect(char *ssid, char *psk);
void mqtt_init(void);
void mqtt_process_loop(void);
#endif