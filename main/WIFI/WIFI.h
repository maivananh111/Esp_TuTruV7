/*
 * WIFI.h
 *
 *  Created on: 1 thg 4, 2022
 *      Author: A315-56
 */

#ifndef COMPONENTS_WIFI_INCLUDE_WIFI_H_
#define COMPONENTS_WIFI_INCLUDE_WIFI_H_


#include "esp_netif.h"
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif



typedef enum wifi_state{
	WIFI_CONNECT_FAILED = 0,
	WIFI_CONNECTED = 1,
}wifi_state_t;


esp_err_t WiFi_STA_Connect(char *SSID, char *PASSWORD);
esp_err_t WiFi_STA_Disconnect(void);

wifi_state_t WiFi_GetState(void);

esp_netif_t *WiFi_STA_get_netif(void);
esp_netif_t *WiFi_STA_get_netif_from_desc(const char *desc);

esp_err_t WiFi_STA_Set_IPV4(char *LocalIP, char *GateWay, char *NetMask);
char *LocalIP(esp_netif_t *WiFi_Netif);
uint8_t ScanWiFi(void);
char *Scan_Get_SSID(uint8_t Number);



#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_WIFI_INCLUDE_WIFI_H_ */
