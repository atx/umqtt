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
#include <espconn.h>
#include <osapi.h>
#include "umqtt.h"

struct umqtt_esp_config {
	char *cid;
	char *hostname;
	int port;
	int kalive;
	struct umqtt_connection umqtt;

	void (*connected_callback)(struct umqtt_connection *);
	struct espconn conn;
	esp_tcp tcp;
	struct ip_addr ip;
	os_timer_t ping_timer;
};

void umqtt_esp_connect(struct umqtt_esp_config *cfg);
