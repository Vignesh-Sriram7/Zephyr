#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/mqtt.h>
#include "wifi.h"
#include "mqtt_src.h"
extern struct mqtt_client client_ctx;
extern bool mqtt_connected;

// Custom libraries

// WiFi settings
#define WIFI_SSID "Vodafone-0F19"
#define WIFI_PSK "baxXcWaHLt9J4xj7"

// HTTP GET settings
#define HTTP_HOST "example.com"
#define HTTP_URL "/"

// Globals
//static char response[512];

// Print the results of a DNS lookup
void print_addrinfo(struct zsock_addrinfo **results)
{
    char ipv4[INET_ADDRSTRLEN];
    char ipv6[INET6_ADDRSTRLEN];
    struct sockaddr_in *sa;
    struct sockaddr_in6 *sa6;
    struct zsock_addrinfo *rp;

    // Iterate through the results
    for (rp = *results; rp != NULL; rp = rp->ai_next) {

        // Print IPv4 address
        if (rp->ai_addr->sa_family == AF_INET) {
            sa = (struct sockaddr_in *)rp->ai_addr;
            zsock_inet_ntop(AF_INET, &sa->sin_addr, ipv4, INET_ADDRSTRLEN);
            printk("IPv4: %s\r\n", ipv4);
        }

        // Print IPv6 address
        if (rp->ai_addr->sa_family == AF_INET6) {
            sa6 = (struct sockaddr_in6 *)rp->ai_addr;
            zsock_inet_ntop(AF_INET6, &sa6->sin6_addr, ipv6, INET6_ADDRSTRLEN);
            printk("IPv6: %s\r\n", ipv6);
        }
    }
}


int main(void)
{
    struct zsock_addrinfo hints;
    struct zsock_addrinfo *res;
    //char http_request[512];
    //int sock;
    //int len;
    //uint32_t rx_total;
    int ret;

    

    // Initialize WiFi
    wifi_init();

    // Connect to the WiFi network (blocking)
    ret = wifi_connect(WIFI_SSID, WIFI_PSK);
    if (ret < 0) {
        printk("Error (%d): WiFi connection failed\r\n", ret);
        return 0;
    }

    // Wait to receive an IP address (blocking)
    wifi_wait_for_ip_addr();

    mqtt_init();
    app_mqtt_connect(&client_ctx);

    // Clear and set address info
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;              // IPv4
    hints.ai_socktype = SOCK_STREAM;        // TCP socket

    // Perform DNS lookup
    printk("Performing DNS lookup...\r\n");
    ret = zsock_getaddrinfo(HTTP_HOST, "80", &hints, &res);
    if (ret != 0) {
        printk("Error (%d): Could not perform DNS lookup\r\n", ret);
        return 0;
    }
    else {
        print_addrinfo(&res);
        zsock_freeaddrinfo(res);
    }

    // Print the results of the DNS lookup
    print_addrinfo(&res);
    mqtt_process_loop();
    while(1)
    {
        mqtt_input(&client_ctx);
        mqtt_live(&client_ctx);
        
        if(mqtt_connected)
        {
            app_mqtt_publish(&client_ctx);
        }
        static uint32_t last_publish = 0;
        if (mqtt_connected && (k_uptime_get_32() - last_publish > 5000)) {
            app_mqtt_publish(&client_ctx);
            last_publish = k_uptime_get_32();
        }
        k_msleep(5000);
    }

    return 0;
}
