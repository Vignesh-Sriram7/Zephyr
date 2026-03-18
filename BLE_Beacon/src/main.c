#include <stdio.h>
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

static void alert_stop(void)
{
    int ret;
	printk("Alert stopped\n");
    ret = gpio_pin_set_dt(&led, 0);
    if (ret<0){
        return;
    }
}

static void alert_high_start(void)
{
    int ret;
	printk("High alert started\n");
    ret = gpio_pin_set_dt(&led, 1);
    if (ret<0){
        return;
    }
}

BT_IAS_CB_DEFINE(ias_callbacks) = {
	.no_alert = alert_stop,
	.high_alert = alert_high_start,
};


int main(void)
{
	int ret;

	// Make sure that the GPIO was initialized
	if (!gpio_is_ready_dt(&led)) {
		return;
	}

	// Set the GPIO as output
	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
	if (ret < 0) {
		return;
	}
}