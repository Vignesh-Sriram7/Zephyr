#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/time_units.h>

// Custom libraries
#include "wifi.h"

// WiFi settings
#define WIFI_SSID "MySSID"
#define WIFI_PSK "MyPassword"

#define MAX_TICKS 10000 // Threshold to move on from waiting for the reply ping of the ultrasonic sensor

static const int32_t wait_time_ms = 10; // Sleep settings 

uint32_t counter=0;     // Counter for the start ping
uint32_t counter2=0;    // Counter for the end ping
uint32_t start_time = 0;    // Timer start time    
uint32_t stop_time = 0;     // Timer stop time
uint32_t duration = 0;      // Total duration from sending the singal to getting it back
uint64_t duration_us = 0;
uint32_t pulse_ns = 0;      // Duty Cycle Pulse
uint32_t distance_cm = 0;
int angle = 0;


// Get devicetree configurations 
static const struct pwm_dt_spec servo = PWM_DT_SPEC_GET(DT_ALIAS(motor_0));
static const struct gpio_dt_spec trig = GPIO_DT_SPEC_GET(DT_ALIAS(hc_trig),gpios);
static const struct gpio_dt_spec echo = GPIO_DT_SPEC_GET(DT_ALIAS(hc_echo),gpios);

int radar_clockwise(int client_sock){

    for(int angle = 0; angle<=180; angle+=5)
        {
            char buf[32];

            pulse_ns = 500000 + (angle * 2000000 / 180);

            pwm_set_pulse_dt(&servo, pulse_ns);
            k_msleep(50);

            // Fire the trigger pulse
            gpio_pin_set_dt(&trig, 1);
            k_busy_wait(10);
            gpio_pin_set_dt(&trig, 0);

            // Check for the echo pin to go high
            counter = 0;
            while(gpio_pin_get_dt(&echo) == 0){
                counter++;
                if(counter>MAX_TICKS) // Condition to break the detection
                    break;
            }
            start_time = k_cycle_get_32();  // Record the cycle time

            // Check for the echo pin to go low
            counter2 = 0;
            while(gpio_pin_get_dt(&echo) == 1){
                counter2++;
                if(counter2>MAX_TICKS)
                    break;
            }
            stop_time = k_cycle_get_32();
            
            // If the counter does not max out calculate the distance
            if(counter < MAX_TICKS && counter2 < MAX_TICKS)
            {
                duration = stop_time - start_time;
                duration_us = (uint64_t)duration * 1000000 / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
                distance_cm = (duration_us * 34) / 2000;
                printk("Angle: %d, Distance: %u cm\n", angle, distance_cm);
                snprintf(buf, sizeof(buf), "<script>d(%d,%u)</script>", angle, distance_cm);
                int ret = zsock_send(client_sock, buf, strlen(buf), 0);
                if (ret < 0) {
                    break;
                }
            }
            else
                printk("No object detected");
        }
        k_msleep(wait_time_ms);
        return 0; 
}

int radar_aclockwise(int client_sock){

    for(int angle = 180; angle>=0; angle-=5)

        {
            char buf[32];
            pulse_ns = 500000 + (angle * 2000000 / 180);

            pwm_set_pulse_dt(&servo, pulse_ns);
            k_msleep(50);
            // HC-SR04 code
            gpio_pin_set_dt(&trig, 1);
            k_busy_wait(10);
            gpio_pin_set_dt(&trig, 0);

            counter = 0;
            while(gpio_pin_get_dt(&echo) == 0){
                counter++;
                if(counter>MAX_TICKS)
                    break;
            }
            start_time = k_cycle_get_32();  

            counter2 = 0;
            while(gpio_pin_get_dt(&echo) == 1){
                counter2++;
                if(counter2>MAX_TICKS)
                    break;
            }
            stop_time = k_cycle_get_32();
            
            if(counter < MAX_TICKS && counter2 < MAX_TICKS)
            {
                duration = stop_time - start_time;
                duration_us = (uint64_t)duration * 1000000 / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
                distance_cm = (duration_us * 34) / 2000;
                printk("Angle: %d, Distance: %u cm\n", angle, distance_cm);
                snprintf(buf, sizeof(buf), "<script>d(%d,%u)</script>", angle, distance_cm);
                int ret = zsock_send(client_sock, buf, strlen(buf), 0);
                if (ret < 0) {
                    break;
                }
            }
            else
                printk("No object detected");
        }         
        k_msleep(wait_time_ms);    
        return 0;
}



int main(void)
{
    struct sockaddr_in serv_addr;  // Defines the IPv4 internet address structure for the server to listen on
    
    struct sockaddr_in client_addr;

    socklen_t client_addr_len = sizeof(client_addr);

    
    int sock;
    int ret;

    char *header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    
    // Check if the trigger is ready
    if(!gpio_is_ready_dt(&trig))
        return 0;
    // Check if the echo is ready
    if(!gpio_is_ready_dt(&echo))
        return 0;
    // Check if the servo is ready
    if(!pwm_is_ready_dt(&servo))
        return 0;

    // Configure the trigger to output
    ret = gpio_pin_configure_dt(&trig, GPIO_OUTPUT);
    if(ret<0)
        return 0;

    // Configure the echo to output
    ret = gpio_pin_configure_dt(&echo, GPIO_INPUT);
    if(ret<0)
        return 0;

    

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
    
    memset(&serv_addr, 0, sizeof(serv_addr));   // Clear memory to prevent garbage values
    serv_addr.sin_family = AF_INET;              // IPv4
    serv_addr.sin_port = htons(80);        // htons -> converts the port number from host byte order to network byte order little endian to big endian
    serv_addr.sin_addr.s_addr = INADDR_ANY;    // Accept connection on any local network interface


    // Create a new socket
    sock = zsock_socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printk("Error (%d): Could not create socket\r\n", errno);
        return 0;
    }

    ret = zsock_bind(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if(ret < 0){
        printk("Error (%d): Could not bind the socket\r\n", errno);
        return 0;
    }

    ret = zsock_listen(sock, 1);    // Puts the socket in listening mode with a queue size of 1
    if (ret < 0) {
        printk("Error (%d): Could not listen to the socket\r\n", errno);
        return 0;
    }
    
    // Set the trigger to 0 initially
    gpio_pin_set_dt(&trig, 0);

    while (1) {

        int client_sock = zsock_accept(sock, (struct sockaddr *) &client_addr, &client_addr_len);
        if (client_sock < 0) {
            printk("Error (%d): Could not connect to the browser\r\n", errno);
            return 0;
        }
        else
        {
        ret = zsock_send(client_sock, header, strlen(header), 0);
        if (ret < 0) {
            printk("Error (%d): Could not send request\r\n", errno);
            return 0;
        }

        char *html_top = "<html><body style='background:#000;color:#0f0;overflow:hidden'>"
                 "<canvas id='c'></canvas><script>"
                 "const v=document.getElementById('c'),x=v.getContext('2d');"
                 "v.width=v.height=600; x.translate(300,500);" // Center the radar
                 "function d(a,r){"
                 "const rad=a*Math.PI/180,px=Math.cos(rad)*r*2,py=-Math.sin(rad)*r*2;"
                 "x.fillStyle='rgba(0,20,0,0.05)';x.fillRect(-300,-500,600,600);" // Fade effect
                 "x.strokeStyle='#0f0';x.beginPath();x.moveTo(0,0);x.lineTo(px,py);x.stroke();"
                 "x.fillStyle='#fff';x.fillRect(px-2,py-2,4,4);}</script>";

        zsock_send(client_sock, html_top, strlen(html_top), 0);
        
        radar_clockwise(client_sock);
        radar_aclockwise(client_sock);
        zsock_close(client_sock);
        }
    }
}