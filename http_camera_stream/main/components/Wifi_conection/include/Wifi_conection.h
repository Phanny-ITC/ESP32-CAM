#ifndef _WIFI_CONNECTION_H
#define _WIFI_CONNECTION_H
/*WIFI configuration*/
//#define ESP_WIFI_SSID      "𝑷𝒉𝒂𝒏𝒏𝒚 ☾"
//#define ESP_WIFI_PASS      "thai@2022"
#define ESP_WIFI_SSID      "Pixel"
#define ESP_WIFI_PASS      "11111111"
#define ESP_MAXIMUM_RETRY  10

static const char *TAG1 = "wifi station";

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

void wifi_init_sta(void);


#endif
