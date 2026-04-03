#include <stdio.h>
#include <math.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/ias.h>

#define VND_MAX_LEN 20

static double x_est = -60;	// Estimated RSSI
static double p_error = 1.0;	// Error Covariance
static double q_noise = 0.01;	// Process noise
static double r_noise = 2.0;	// Measurement noise
static double k_gain;	// Kalman gain

// Configuration parameters for Bluetooth LE advertising
static struct bt_le_adv_param adv_param;

// Current connection variable
struct bt_conn *current_con;
bool is_connected;
char connected_addr_str[BT_ADDR_LE_STR_LEN] = {0};

// Adding a work scheduler for periodic update of the rssi
struct k_work_delayable rssi_work;

static int wdt_channel_id; // Declare the unique channel id variable for wdt to access in other functions

// Device initialization (LED, Buzzer, Watchdog timer)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);
static const struct gpio_dt_spec buzzer = GPIO_DT_SPEC_GET(DT_ALIAS(buzzer), gpios);
static const struct device *const wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));
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
                           BT_GATT_PERM_WRITE_AUTHEN,	// Let it remain as BT_GATT_PERM_WRITE and not BT_GATT_PERM_AUTH_WRITE 
                           NULL, write_callback, vnd_auth_value),
	BT_GATT_CHARACTERISTIC(&vnd_enc_uuid.uuid,	// Folder for the read function of the client
			       		   BT_GATT_CHRC_READ,	
			       		   BT_GATT_PERM_READ_AUTHEN,
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
// Specifically requires the bonding of the device
static void alert_high_start(void)
{
    int ret;
	struct bt_conn_info info;
    bt_conn_get_info(current_con, &info);

    if (info.security.level < BT_SECURITY_L3) {
        printk("Unauthorized alert attempt blocked!\n");
        return; 
    }
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
// Read the connected device's RSSI 
static void read_conn_rssi(uint16_t handle, int8_t *rssi)
{
	struct net_buf *buf, *rsp = NULL;
	struct bt_hci_cp_read_rssi *cp;
	struct bt_hci_rp_read_rssi *rp;

	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_READ_RSSI, sizeof(*cp));
	if (!buf) {
		printk("Unable to allocate command buffer\n");
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_RSSI, buf, &rsp);
	if (err) {
		printk("Read RSSI err: %d\n", err);
		return;
	}

	rp = (void *)rsp->data;
	*rssi = rp->rssi;

	net_buf_unref(rsp);
}


double kalman_filter(int8_t raw_rssi)
{
	// Since the time has passed uncertain of the old position
	p_error = p_error + q_noise;
	// Trust factor --> ratio between uncertainity p_error and the radio's nosie r_error
	k_gain = (p_error/(p_error+r_noise));
	// Take the old estimate and update with kalman gain
	x_est = x_est + k_gain*(raw_rssi - x_est);
	//Update the uncertainity
	p_error = (1-k_gain)*p_error;
	// return the filtered value
	return x_est;

}

// rssi: The value from the radio
// measure_power: RSSI at 1 meter (usually -59)
// n: Environmental factor (2.0 for air, 3.0 for a room)
double calculate_distance(double rssi) 
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

	// If we are connected, ignore everything except the connected phone
    if (is_connected) {
        if (strcmp(addr_str, connected_addr_str) != 0) {
            return; // It's not our phone, stay quiet
        }
	}

    double distance = calculate_distance(rssi);

    // Print the MAC, the Signal, and the estimated Meters
    printk("Device: %s | RSSI: %d | Est. Distance: %.2f m\n", 
            addr_str, rssi, distance);
}

// Signal isolation and deterministic proximity heartbeat
// Event driven and power efficient
void connected_rssi_poller(struct k_work *work)
{	
	// Feed the watchdog to prove that the bluetooth stack is alive. If not or thread is stuck device will restart
	wdt_feed(wdt, wdt_channel_id);
    if (!is_connected || !current_con) return;

    uint16_t handle;
    int8_t rssi = 0;

    if (bt_hci_get_conn_handle(current_con, &handle) == 0) {
        read_conn_rssi(handle, &rssi);

        if (rssi != 0) {
			double k_rssi = kalman_filter(rssi);
            double distance = calculate_distance(k_rssi);
            printk("[CONNECTED] RSSI: %d | Distance: %.2f m\n", rssi, distance);

            // Proximity trigger (Turn on LED if closer than 1.5 meters)
            if (distance > 0 && distance < 1.5) {
                gpio_pin_set_dt(&led, 1);
            } else {
                gpio_pin_set_dt(&led, 0);
            }
        }
    }
    k_work_reschedule(&rssi_work, K_MSEC(1000)); // Repeat every 1s
}

/* Connection Callbacks*/
// Triggered the moment the radio handshake is successful --> not authenticated
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed, err 0x%02x %s\n", err, bt_hci_err_to_str(err));
	} else {
		is_connected = true;
		current_con = bt_conn_ref(conn);
		char addr[BT_ADDR_LE_STR_LEN];
		// bt_conn_get_dst --> gets the destination adddress of the connection
		bt_addr_le_to_str(bt_conn_get_dst(conn), connected_addr_str, sizeof(connected_addr_str));	// Convert the obtained address to readable string
		printk("Connected to: %s\n Stopping Scan\n", connected_addr_str);
		bt_le_scan_stop();
		k_work_schedule(&rssi_work, K_MSEC(100));
	}
}

// When the client goes out of range or the app is closed
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected, reason 0x%02x %s\n", reason, bt_hci_err_to_str(reason));
	is_connected = false;
    k_work_cancel_delayable(&rssi_work);
    // Safety: Turn off LED and Buzzer when phone leaves
    gpio_pin_set_dt(&led, 0);
    gpio_pin_set_dt(&buzzer, 0);
	memset(connected_addr_str, 0, sizeof(connected_addr_str));
	if(current_con)
	{
		bt_conn_unref(current_con);
		current_con=NULL;
	}
	// Restart the advertisement broadcasting after disconnection
	bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	// Restart the RSSI scan for all available devices
	bt_le_scan_start(&scan_param, device_found);
}

// Hooks the functions into the system --> callback structure for connection events
BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected
};

/* Authorization functions*/
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = auth_passkey_display,
	.cancel = auth_cancel,
};

// Define the function to initialize the bluetooth
static void bt_ready(int err)
{
	
	printk("Bluetooth initialized\n");
	
	// Store the bonding data of the device to remember for the power cycle fo the ESP32
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
        settings_load();
        printk("Settings loaded (Bonds restored)\n");
    }

	adv_param = *BT_LE_ADV_CONN_FAST_1;
	err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
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
		printk("LED not ready\n");
		return 0;
	}

	if (!gpio_is_ready_dt(&buzzer)) {
		printk("Buzzer not ready\n");
		return 0;
	}

	if(!device_is_ready(wdt))
	{
		printk("%s: device not ready\n", wdt->name);
		return 0;
	}

	// Watchdog timer configuration- Define the wdt behaviour
	struct wdt_timeout_cfg wdt_config = {
		.window.min = 0U,
		.window.max = 10000U, // 
		.callback = NULL,
		.flags = WDT_FLAG_RESET_SOC,
	};
	// Set the GPIO as output
	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
	if (ret < 0) {
		return 0;
	}
	ret = gpio_pin_configure_dt(&buzzer, GPIO_OUTPUT);
	if (ret < 0) {
		return 0;
	}
	
	// Initialize the work scheduler
	k_work_init_delayable(&rssi_work, connected_rssi_poller);
	
	//bt_passkey_set(943507);	// If a fixed password is being used---> not secure generally
	
	// Install the timeout configuration and get a Channel ID
	wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
	if (wdt_channel_id < 0) {
		printk("Watchdog install error\n");
		return 0;
	}

	// Start the hardware timer
	err = wdt_setup(wdt, WDT_OPT_PAUSE_IN_SLEEP);
	if (err < 0) {
		printk("Watchdog setup error\n");
		return 0;
	}
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	// Register the authentication functions for randomness
	bt_conn_auth_cb_register(&auth_cb_display);


	while (1) {
		// Feed the watchdog during idle/scanning periods
		wdt_feed(wdt, wdt_channel_id); // Keeps it alive while waiting for connection
		k_sleep(K_MSEC(2000));
	}
	return 0;
}
