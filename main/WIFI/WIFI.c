#include "WIFI.h"

#include <stdio.h>
#include "string.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/gpio.h"


#define MAX_RECONNECT 7
#define NR_OF_IP_ADDRESSES_TO_WAIT_FOR (active_interfaces*2)

static int active_interfaces = 0;
static xSemaphoreHandle semph_get_ip_addrs;
static esp_netif_t *sta_esp_netif = NULL;
static esp_ip4_addr_t s_ip_addr;
static esp_ip6_addr_t ipv6_addr;
static uint16_t number = 20;
static wifi_ap_record_t ap_info[20];
static int reconnect_num = 0;
static const char *TAG = "WIFI STATION";
static const char *ipv6_addr_types[] = {
    "IP6_ADDR_UNKNOWN",
    "IP6_ADDR_GLOBAL",
    "IP6_ADDR_LINK_LOCAL",
    "IP6_ADDR_SITE_LOCAL",
    "IP6_ADDR_UNIQUE_LOCAL",
    "IP6_ADDR_IPV4_MAPPED_IPV6"
};


static wifi_state_t WiFi_State = WIFI_CONNECT_FAILED;

/* ********** FUNCTION PROTOTYPE ********** */
static esp_netif_t *WiFi_STA_Init(char *SSID, char *PASSWORD);
static void WiFi_STA_Denit(void);
static void WiFi_STA_Start(char *SSID, char *PASSWORD);
static void WiFi_STA_Stop(void);
static bool WiFi_STA_is_our_netif(const char *prefix, esp_netif_t *netif);
static void WiFi_STA_event_got_ip(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void WiFi_STA_event_got_ipv6(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void WiFi_STA_event_disconnect(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void WiFi_STA_event_connect(void *esp_netif, esp_event_base_t event_base, int32_t event_id, void *event_data);
/* ********** FUNCTION SOURCE ********** */
static bool WiFi_STA_is_our_netif(const char *prefix, esp_netif_t *netif){
    return strncmp(prefix, esp_netif_get_desc(netif), strlen(prefix) - 1) == 0;
}

static void WiFi_STA_Start(char *SSID, char *PASSWORD){
    sta_esp_netif = WiFi_STA_Init(SSID, PASSWORD);
    active_interfaces++;
    semph_get_ip_addrs = xSemaphoreCreateCounting(NR_OF_IP_ADDRESSES_TO_WAIT_FOR, 0);
}

static void WiFi_STA_Stop(void){
	WiFi_STA_Denit();
    active_interfaces--;
}

static void WiFi_STA_event_got_ip(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    if (!WiFi_STA_is_our_netif(TAG, event->esp_netif)) {
        ESP_LOGW(TAG, "Got IPv4 from another interface \"%s\": ignored", esp_netif_get_desc(event->esp_netif));
        return;
    }
    ESP_LOGI(TAG, "Got IPv4 event: Interface \"%s\" address: " IPSTR, esp_netif_get_desc(event->esp_netif), IP2STR(&event->ip_info.ip));
    memcpy(&s_ip_addr, &event->ip_info.ip, sizeof(s_ip_addr));
    xSemaphoreGive(semph_get_ip_addrs);
}

static void WiFi_STA_event_got_ipv6(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
    ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
    if (!WiFi_STA_is_our_netif(TAG, event->esp_netif)) {
        ESP_LOGW(TAG, "Got IPv6 from another netif: ignored");
        return;
    }
    esp_ip6_addr_type_t ipv6_type = esp_netif_ip6_get_addr_type(&event->ip6_info.ip);
    ESP_LOGI(TAG, "Got IPv6 event: Interface \"%s\" address: " IPV6STR ", type: %s", esp_netif_get_desc(event->esp_netif),
             IPV62STR(event->ip6_info.ip), ipv6_addr_types[ipv6_type]);
    if (ipv6_type == ESP_IP6_ADDR_IS_LINK_LOCAL) {
        memcpy(&ipv6_addr, &event->ip6_info.ip, sizeof(ipv6_addr));
        xSemaphoreGive(semph_get_ip_addrs);
    }
}

static void WiFi_STA_event_disconnect(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
    esp_err_t err = esp_wifi_connect();
    reconnect_num++;
    if(reconnect_num > MAX_RECONNECT) esp_restart();
    ESP_LOGW(TAG, "Wi-Fi connect failed, trying to reconnect... Err: %d", WiFi_State);

    if (err == ESP_ERR_WIFI_NOT_STARTED) {
    	WiFi_State = WIFI_CONNECT_FAILED;
        return;
    }
    ESP_ERROR_CHECK(err);
}

static void WiFi_STA_event_connect(void *esp_netif, esp_event_base_t event_base, int32_t event_id, void *event_data){
    esp_netif_create_ip6_linklocal(esp_netif);
    WiFi_State = WIFI_CONNECTED;
}

static esp_netif_t *WiFi_STA_Init(char *SSID, char *PASSWORD){
    char *desc;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();

    asprintf(&desc, "%s: %s", TAG, esp_netif_config.if_desc);
    esp_netif_config.if_desc = desc;
    esp_netif_config.route_prio = 128;
    esp_netif_t *netif = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);
    free(desc);
    esp_wifi_set_default_wifi_sta_handlers();

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &WiFi_STA_event_disconnect, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WiFi_STA_event_got_ip, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &WiFi_STA_event_connect, netif));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_GOT_IP6, &WiFi_STA_event_got_ipv6, NULL));

//    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold.rssi = -127,
            .threshold.authmode = WIFI_AUTH_OPEN,
//			.channel = 0
        },
    };
    esp_wifi_set_ps(WIFI_PS_NONE);
    memcpy(wifi_config.sta.ssid, SSID, strlen(SSID));
    memcpy(wifi_config.sta.password, PASSWORD, strlen(PASSWORD));
    ESP_LOGI(TAG, "Connecting to %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_connect();
    return netif;
}

static void WiFi_STA_Denit(void){
    esp_netif_t *wifi_netif = WiFi_STA_get_netif_from_desc("sta");
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &WiFi_STA_event_disconnect));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &WiFi_STA_event_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_GOT_IP6, &WiFi_STA_event_got_ipv6));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &WiFi_STA_event_connect));
    esp_err_t err = esp_wifi_stop();
    if (err == ESP_ERR_WIFI_NOT_INIT) {
        return;
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(wifi_netif));
    esp_netif_destroy(wifi_netif);
    sta_esp_netif = NULL;
}

/* ************** USER FUNCTION ************* */
esp_err_t WiFi_STA_Connect(char *SSID, char *PASSWORD){
    if (semph_get_ip_addrs != NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    WiFi_STA_Start(SSID, PASSWORD);
    ESP_ERROR_CHECK(esp_register_shutdown_handler(&WiFi_STA_Stop));
    ESP_LOGI(TAG, "Waiting for IP(s)");
    for (int i = 0; i < NR_OF_IP_ADDRESSES_TO_WAIT_FOR; ++i) {
        xSemaphoreTake(semph_get_ip_addrs, portMAX_DELAY);
    }
    esp_netif_t *netif = NULL;
    esp_netif_ip_info_t ip;
    for (int i = 0; i < esp_netif_get_nr_of_ifs(); ++i) {
        netif = esp_netif_next(netif);
        if (WiFi_STA_is_our_netif(TAG, netif)) {
            ESP_LOGI(TAG, "Connected to %s Err: %d", esp_netif_get_desc(netif), WiFi_State);
            ESP_ERROR_CHECK(esp_netif_get_ip_info(netif, &ip));
            ESP_LOGI(TAG, "- IPv4 address: " IPSTR, IP2STR(&ip.ip));
            esp_ip6_addr_t ip6[5];
            int ip6_addrs = esp_netif_get_all_ip6(netif, ip6);
            for (int j = 0; j < ip6_addrs; ++j) {
                esp_ip6_addr_type_t ipv6_type = esp_netif_ip6_get_addr_type(&(ip6[j]));
                ESP_LOGI(TAG, "- IPv6 address: " IPV6STR ", type: %s", IPV62STR(ip6[j]), ipv6_addr_types[ipv6_type]);
            }
        }
    }
    return ESP_OK;
}

esp_err_t WiFi_STA_Disconnect(void){
    if (semph_get_ip_addrs == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    vSemaphoreDelete(semph_get_ip_addrs);
    semph_get_ip_addrs = NULL;
    WiFi_STA_Stop();
    ESP_ERROR_CHECK(esp_unregister_shutdown_handler(&WiFi_STA_Stop));
    return ESP_OK;
}

esp_netif_t *WiFi_STA_get_netif(void){
    return sta_esp_netif;
}

wifi_state_t WiFi_GetState(void){
	return WiFi_State;
}

esp_netif_t *WiFi_STA_get_netif_from_desc(const char *desc){
    esp_netif_t *netif = NULL;
    char *expected_desc;
    asprintf(&expected_desc, "%s: %s", TAG, desc);
    while ((netif = esp_netif_next(netif)) != NULL) {
        if (strcmp(esp_netif_get_desc(netif), expected_desc) == 0) {
            free(expected_desc);
            return netif;
        }
    }
    free(expected_desc);
    return netif;
}

esp_err_t WiFi_STA_Set_IPV4(char *LocalIP, char *GateWay, char *NetMask){
	esp_netif_ip_info_t ip_info = {0};
	if(sta_esp_netif){
		memset(&ip_info, 0, sizeof(esp_netif_ip_info_t));
		esp_netif_dhcpc_stop(sta_esp_netif);
        ip_info.ip.addr = esp_ip4addr_aton((const char *)LocalIP);
        ip_info.netmask.addr = esp_ip4addr_aton((const char *)NetMask);
        ip_info.gw.addr = esp_ip4addr_aton((const char *)GateWay);
        esp_err_t err = esp_netif_set_ip_info(sta_esp_netif,&ip_info);
	    if (err != ESP_OK) {
	        ESP_LOGE(TAG, "Failed to set ip info");
	        return err;

	    }
	}
	return ESP_OK;
}

char *LocalIP(esp_netif_t *WiFi_Netif){
	esp_netif_ip_info_t IP_info_t = {0};
	char *buf;
	buf = malloc(16*sizeof(char));
	esp_netif_get_ip_info(WiFi_Netif, &IP_info_t);
	esp_ip4addr_ntoa(&IP_info_t.ip, (char *)buf, 16);
	return (char *)buf;
}

uint8_t ScanWiFi(void){
	uint16_t ap_count = 0;
	memset(ap_info, 0, sizeof(ap_info));
	ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    return (uint8_t)ap_count;
}

char *Scan_Get_SSID(uint8_t Number){
	char *buffer;
	uint8_t len = sizeof(ap_info[Number].ssid);
	buffer = malloc(len * sizeof(uint8_t));
	memcpy(buffer, ap_info[Number].ssid, sizeof(ap_info[Number].ssid));
	return buffer;
}






