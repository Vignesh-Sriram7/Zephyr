#include <stdio.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>

#define VND_MAX_LEN 20
// Configuration parameters for Bluetooth LE advertising
static struct bt_le_adv_param adv_param;
// Set the lock and open pulse not to the extremes
// Dont set it tp the extremes 0.5ms and 2.5ms 
int lock_pulse_ns = 2000000;
int open_pulse_ns = 1000000;

/*Devicetree Configurations*/
static const struct pwm_dt_spec servo = PWM_DT_SPEC_GET(DT_ALIAS(motor_0));

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

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL),
}; // Advertising data, to let the clients nearby know that the device is a LE looking for a connection.

// The name part coould be split to the below to have it efficiently advertised but this works. Just use the ad.

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL),
	BT_DATA(BT_DATA_NAME_SHORTENED, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
}; // Scan response which is sent as more onformation once the ad is received and information requested

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

	// Obtain the value of the write and store the command
	memcpy(value + offset, buf, len);
	value[offset + len] = 0;
	// Compare the entered command to OPEN
	if (strcmp(value, "OPEN")==0){
		printk("Lock Opening\n");
		// If satisfied, set the pulse to open pulse
		pwm_set_pulse_dt(&servo, open_pulse_ns);
		// Allow the gears to move
		//k_msleep(500);			// Makes the connection crash so just pulse the open and close
		// Set the motor to rest to save energy
		//pwm_set_pulse_dt(&servo, 0);

		// Update the status for the next 'Read'
        snprintf(vnd_value, sizeof(vnd_value), "UNLOCKED");
	}
	// Compare the entered command to CLOSE
	else if (strcmp(value, "CLOSE")==0){
		printk("Lock Closing\n");
		// If satisfied, set the pulse to locked pulse
		pwm_set_pulse_dt(&servo, lock_pulse_ns);
		// Allow the gears to move
		//k_msleep(500);
		// Set the motor to rest to save energy
		//pwm_set_pulse_dt(&servo, 0);

		// Update the status for the next 'Read'
        snprintf(vnd_value, sizeof(vnd_value), "LOCKED");
	}
	return len;
}

// Acts as a menu for the smart lock app
// Bundles the service and characteristics into a static array --> mostly same structure for most of the smart bluetooth applications
// CHARACTERISTIC -> links the callback function to the private read or write data
BT_GATT_SERVICE_DEFINE(lock_svc,	// Defines the variable name to track this service
    BT_GATT_PRIMARY_SERVICE(&vnd_uuid), // Defines the start of the service or the folder of this entire smart lock
    BT_GATT_CHARACTERISTIC(&vnd_auth_uuid.uuid, // Defines the function of the client, since the client writes to the server required write functions are added
                           BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE,	// Let it remain as BT_GATT_PERM_WRITE and not BT_GATT_PERM_AUTH_WRITE 
                           NULL, write_callback, vnd_auth_value),
	BT_GATT_CHARACTERISTIC(&vnd_enc_uuid.uuid,	// Folder for the read function of the client
			       		   BT_GATT_CHRC_READ,	
			       		   BT_GATT_PERM_READ,
			               read_callback, NULL, vnd_value)); // Callback function to read 

/*Security and authentication*/

// Security guard for the application
// Tells the ESP32 what to do when a device tries to pair
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    // This prints the random code to your terminal
    printf("Passkey for %p: %06u\n", (void *)conn, passkey);
}

// Methods of Authentication
static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = auth_passkey_display,
	.cancel = NULL,
};

/* Connection Callbacks*/
// Triggered the moment the radio handshake is successful --> not authenticated
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed, err 0x%02x %s\n", err, bt_hci_err_to_str(err));
	} else {
		printk("Connected\n");
	}
}

// When the client goes out of range or the app is closed
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected, reason 0x%02x %s\n", reason, bt_hci_err_to_str(reason));
}

// Hooks the functions into the system --> callback structure for connection events
BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected
};

// If the bonded is true it means that the client and the esp have exchanged the keys and stored it in the FLASH
// When the client connects next time passkey is not neccessary
void pairing_complete(struct bt_conn *conn, bool bonded)
{
	printk("Pairing completed. Rebooting in 5 seconds...\n");

}

// Notification of the state
static struct bt_conn_auth_info_cb bt_conn_auth_info = {
	.pairing_complete = pairing_complete
};

/*Initializes the bluetooth module, 
checks for the errors on hardware startup, 
register the security rules and start advertising*/ 
static void bt_ready(int error)
{
	if(error)
		return;
	int err;
	// Stops anyone from using the write_callback until paired
	bt_conn_auth_cb_register(&auth_cb_display);
	bt_conn_auth_info_cb_register(&bt_conn_auth_info);

	printk("Bluetooth initialized\n");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}
	// time to wait before each advertisement
	adv_param = *BT_LE_ADV_CONN_FAST_1;

	err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
	} else {
		printk("Advertising successfully started\n");
	}
}

int main(void){
	int ret;
	
	// Check if the servo is ready
    if(!pwm_is_ready_dt(&servo)){
    printk("Error: PWM device not ready\n");
    return 0;
	}

	pwm_set_pulse_dt(&servo, lock_pulse_ns);
    k_msleep(500);
    pwm_set_pulse_dt(&servo, 0);

	ret = bt_enable(bt_ready);
	if (ret) {
		printk("Bluetooth init failed (err %d)\n", ret);
		return 0;
	}

	while (1) {
		k_sleep(K_FOREVER);
	}
	return 0;
}