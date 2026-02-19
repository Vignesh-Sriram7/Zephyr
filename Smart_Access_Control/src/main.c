#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/settings/settings.h>

#define VND_MAX_LEN 20

/////*Service and characteristics definition*/////

// Converts 128 Bit random string to a format esp32 understands
#define BT_UUID_CUSTOM_SERVICE_VAL \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)

// UUID Universally Unique Identifier 
// Uniquely identifies information without a central registration authority

// Vendor Service UUID - custom. Like a folder which has everything realted to this app
static const struct bt_uuid_128 vnd_uuid = BT_UUID_INIT_128(
	BT_UUID_CUSTOM_SERVICE_VAL);

// Vendor Encrypted UUID - Reading. The status characteristic
static const struct bt_uuid_128 vnd_enc_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1));

// Vendor Authenticated UUID - Writing. The action characteristic
static const struct bt_uuid_128 vnd_auth_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2));

// Actual memory storage in the RAM for the application
// Holds the values that the client reads or writes
// The incoming bluetooth data is saved in these arrays, which are initialized initially to vendor but change in the course of the program
static uint8_t vnd_value[VND_MAX_LEN + 1] = { 'V', 'e', 'n', 'd', 'o', 'r'};
static uint8_t vnd_auth_value[VND_MAX_LEN + 1] = { 'V', 'e', 'n', 'd', 'o', 'r'};

// Acts as a menu for the smart lock app
// Bundles the service and characteristics into a static array --> mostly same structure for most of the smart bluetooth applications
// CHARACTERISTIC -> links the callback function to the private read or write data
BT_GATT_SERVICE_DEFINE(lock_svc,	// Defines the variable name to track this service
    BT_GATT_PRIMARY_SERVICE(&vnd_uuid), // Defines the start of the service or the folder of this entire smart lock
    BT_GATT_CHARACTERISTIC(&vnd_auth_uuid.uuid, // Defines the function of the client, since the client writes to the server required write functions are added
                           BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE_AUTHEN,
                           NULL, write_callback, vnd_auth_value),
	BT_GATT_CHARACTERISTIC(&vnd_enc_uuid.uuid,	// Folder for the read function of the client
			       		   BT_GATT_CHRC_READ,	
			       		   BT_GATT_PERM_READ_ENCRYPT,
			               read_callback, NULL, vnd_value),); // Callback function to read 
/* Callback functions
respective attr->user_data access the specific private read() or write() data storage array*/

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

	memcpy(value + offset, buf, len);
	value[offset + len] = 0;

	return len;
}
