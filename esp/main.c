/* 
 * Copyright (c) 2015 Josef Gajdusek
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. 
 */

#include <user_interface.h>
#include <osapi.h>
#include "driver/uart.h"
#include "umqtt-esp.h"

#define SSID		"SSID"
#define PASSWORD	"PASS"
#define MQTT_SERVER "wherever.lan"

void message_callback(struct umqtt_connection *u, char *topic, uint8_t *data,
		int len)
{
	os_printf("umqtt: msg: %s %s\n", topic, (char *)data);
}

void connected_callback(struct umqtt_connection *u)
{
	os_printf("umqtt: connect handshake complete\n");
	umqtt_subscribe(u, "#");
}

uint8_t rxbuff[1024];
uint8_t txbuff[1024];

struct umqtt_esp_config umqttconfig = {
	.cid = "esp-umqtt",
	.hostname = MQTT_SERVER,
	.port = 1883,
	.kalive = 60,
	.umqtt = {
		.rxbuff = {
			.start = rxbuff,
			.length = sizeof(rxbuff),
		},
		.txbuff = {
			.start = txbuff,
			.length = sizeof(txbuff),
		},
		.connected_callback = connected_callback,
		.message_callback = message_callback,
	}
};

void on_wifi_event(System_Event_t *event)
{
	switch (event->event) {
	case EVENT_STAMODE_GOT_IP:
		umqtt_esp_connect(&umqttconfig);
		break;
	}
}

void user_init(void)
{
	uart_init(115200, 115200);
	struct station_config config = {
		.ssid = SSID,
		.password = PASSWORD,
	};

	wifi_set_event_handler_cb(on_wifi_event);

	wifi_set_opmode(STATION_MODE);
	wifi_station_set_config(&config);
	wifi_set_phy_mode(PHY_MODE_11B);
	wifi_station_connect();
}
