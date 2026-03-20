#include <stdio.h>
#include <math.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/ias.h>

#define VND_MAX_LEN 20
// Configuration parameters for Bluetooth LE advertising
static struct bt_le_adv_param adv_param;

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);
static const struct gpio_dt_spec buzzer = GPIO_DT_SPEC_GET(DT_ALIAS(buzzer), gpios);
/////*Service and characteristics definition*/////

// Converts 128 Bit random string to a format esp32 understands
#define BT_UUID_CUSTOM_SERVICE_VAL \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)

// Vendor Service UUID
static const struct bt_uuid_128 vnd_uuid = BT_UUID_INIT_128(
	BT_UUID_CUSTOM_SERVICE_VAL);

//Vendor Encrypted UUID
static const struct bt_uuid_128 vnd_enc_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1));

// Vendor Authenticated UUID
static const struct bt_uuid_128 vnd_auth_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2));

// Memory storage to store the incoming data for the application
static uint8_t vnd_value[VND_MAX_LEN + 1] = { 'V', 'e', 'n', 'd', 'o', 'r'};
static uint8_t vnd_auth_value[VND_MAX_LEN + 1] = { 'V', 'e', 'n', 'd', 'o', 'r'};

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
}; // Advertising data, to let the clients nearby know that the device is a LE looking for a connection.

// The name part coould be split to the below to have it efficiently advertised but this works. Just use the ad.

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL),
}; // Scan response which is sent as more onformation once the ad is received and information requested

// type: Passive, asks the ESP32 to just over hear
// interval: how often the ESP32 starts looking
// window: how long is it listening during that time
// options: defines the behaviour 
static struct bt_le_scan_param scan_param = {
    .type       = BT_LE_SCAN_TYPE_PASSIVE,
    .options    = BT_LE_SCAN_OPT_NONE,
    .interval   = BT_GAP_SCAN_FAST_INTERVAL,
    .window     = BT_GAP_SCAN_FAST_WINDOW,
};

// Read callback function taht retrieves the data that is stored in the vnd_value and display it to the client
static ssize_t read_callback(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 strlen(value));
}

// Write callback function receives the command and stores it in the vnd_auth_value
static ssize_t write_callback(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset + len > VND_MAX_LEN) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	// Obtain the value of the write and store the command
	memcpy(value + offset, buf, len);
	value[offset + len] = 0;
	return len;
}

// CHARACTERISTIC -> links the callback function to the private read or write data
BT_GATT_SERVICE_DEFINE(beacon_svc,	// Defines the variable name to track this service
    BT_GATT_PRIMARY_SERVICE(&vnd_uuid), // Defines the start of the service or the folder of this entire smart lock
    BT_GATT_CHARACTERISTIC(&vnd_auth_uuid.uuid, // Defines the function of the client, since the client writes to the server required write functions are added
                           BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE,	// Let it remain as BT_GATT_PERM_WRITE and not BT_GATT_PERM_AUTH_WRITE 
                           NULL, write_callback, vnd_auth_value),
	BT_GATT_CHARACTERISTIC(&vnd_enc_uuid.uuid,	// Folder for the read function of the client
			       		   BT_GATT_CHRC_READ,	
			       		   BT_GATT_PERM_READ,
			               read_callback, NULL, vnd_value)); // Callback function to read 

/* Define the IAS functions*/
// Helps find the device
static void alert_stop(void)
{
    int ret;
	printk("Alert stopped\n");
    ret = gpio_pin_set_dt(&buzzer, 0);
    if (ret<0){
        return;
    }
}

// When the device writes the alert comand the buzzer is triggered
static void alert_high_start(void)
{
    int ret;
	printk("High alert started\n");
    ret = gpio_pin_set_dt(&buzzer, 1);
    if (ret<0){
        return;
    }
}

BT_IAS_CB_DEFINE(ias_callbacks) = {
	.no_alert = alert_stop,
	.high_alert = alert_high_start,
};

/* RSSI used to find the proximity of the device via the BLE signal*/
// rssi: The value from the radio
// measure_power: RSSI at 1 meter (usually -59)
// n: Environmental factor (2.0 for air, 3.0 for a room)
double calculate_distance(int8_t rssi) 
{
    if (rssi == 0) {
        return -1.0; // Error or unknown
    }

    double measured_power = -59.0; 
    double n = 3.0; 

    // Formula: d = 10 ^ ((Measured Power - RSSI) / (10 * n))
    return pow(10, (double)(measured_power - rssi) / (10 * n));
}


// Callbac function to measure the RSSI
// Hardware measures the signal strength anf feeds to this function as int8
static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
    char addr_str[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

    double distance = calculate_distance(rssi);

    // Print the MAC, the Signal, and the estimated Meters
    printk("Device: %s | RSSI: %d | Est. Distance: %.2f m\n", 
            addr_str, rssi, distance);

    // Proximity trigger (Turn on LED if closer than 1.5 meters)
    if (distance > 0 && distance < 1.5) {
        gpio_pin_set_dt(&led, 1);
    } else {
        gpio_pin_set_dt(&led, 0);
    }
}

// Define the function to initialize the bluetooth
static void bt_ready(void)
{
	int err;

	printk("Bluetooth initialized\n");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	// Links the scan parameters and the callback function to measure RSSI
	err = bt_le_scan_start(&scan_param, device_found);
    if (err) {
        printk("Scanning failed to start (err %d)\n", err);
        return;
    }

	printk("Advertising successfully started\n");
}

int main(void)
{
	int ret;
	int err;

	// Make sure that the GPIO was initialized
	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	if (!gpio_is_ready_dt(&buzzer)) {
		return 0;
	}

	// Set the GPIO as output
	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
	if (ret < 0) {
		return 0;
	}
	ret = gpio_pin_configure_dt(&buzzer, GPIO_OUTPUT);
	if (ret < 0) {
		return 0;
	}

	
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	while (1) {
		k_sleep(K_FOREVER);
	}
	return 0;
}