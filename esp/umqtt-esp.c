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
#include <espconn.h>
#include "umqtt-esp.h"

static void umqtt_esp_connect_cb(void *arg)
{
	struct espconn *conn = arg;
	struct umqtt_esp_config *cfg = conn->reverse;
	umqtt_connect(&cfg->umqtt);
}

static void umqtt_esp_recv_cb(void *arg, char *pdata, unsigned short len)
{
	struct espconn *conn = arg;
	struct umqtt_esp_config *cfg = conn->reverse;
	umqtt_circ_push(&cfg->umqtt.rxbuff, (uint8_t *) pdata, (int) len);
	umqtt_process(&cfg->umqtt);
}

static void umqtt_esp_new_packet_cb(struct umqtt_connection *umqtt)
{
	struct umqtt_esp_config *cfg = umqtt->private;
	uint8_t buff[512];
	int len;

	len = umqtt_circ_pop(&umqtt->txbuff, buff, sizeof(buff));

	espconn_send(&cfg->conn, buff, len);
}

static void umqtt_esp_ping_timer_cb(void *_cfg)
{
	struct umqtt_esp_config *cfg = _cfg;
	umqtt_ping(&cfg->umqtt);
}

static void umqtt_esp_connected_cb(struct umqtt_connection *umqtt)
{
	struct umqtt_esp_config *cfg = umqtt->private;
	os_timer_setfn(&cfg->ping_timer, umqtt_esp_ping_timer_cb, cfg);
	os_timer_arm(&cfg->ping_timer, cfg->umqtt.kalive * 1000, 1);
	if (cfg->connected_callback)
		cfg->connected_callback(umqtt);
}

static void umqtt_esp_sent_cb(void *arg)
{
	struct espconn *conn = arg;
	struct umqtt_esp_config *cfg = conn->reverse;
	if (!umqtt_circ_is_empty(&cfg->umqtt.txbuff))
		umqtt_esp_new_packet_cb(&cfg->umqtt);
}

static void umqtt_esp_disconnect_cb(void *arg)
{
	struct espconn *conn = arg;
	os_printf("umqtt: Disconnected, reconnecting...\n");
	umqtt_esp_connect(conn->reverse);
}

static void umqtt_esp_dns_cb(const char *name, ip_addr_t *ip, void *_cfg)
{
	struct umqtt_esp_config *cfg = _cfg;
	if (ip == NULL) {
		os_printf("umqtt: DNS resolve failed! Retrying...\n");
		umqtt_esp_connect(cfg);
		return;
	}

	cfg->umqtt.private = cfg;
	cfg->umqtt.connected_callback = umqtt_esp_connected_cb;
	cfg->umqtt.new_packet_callback = umqtt_esp_new_packet_cb;

	os_printf("umqtt: connecting to " IPSTR "...\n", IP2STR(ip));

	cfg->conn.type = ESPCONN_TCP;
	cfg->conn.state = ESPCONN_NONE;
	cfg->conn.proto.tcp = &cfg->tcp;
	cfg->conn.proto.tcp->local_port = espconn_port();
	cfg->conn.proto.tcp->remote_port = cfg->port;
	cfg->conn.reverse = cfg;
	os_memcpy(&cfg->conn.proto.tcp->remote_ip, &ip->addr, 4);
	espconn_regist_connectcb(&cfg->conn, umqtt_esp_connect_cb);
	espconn_regist_recvcb(&cfg->conn, umqtt_esp_recv_cb);
	espconn_regist_sentcb(&cfg->conn, umqtt_esp_sent_cb);
	espconn_regist_disconcb(&cfg->conn, umqtt_esp_disconnect_cb);
	espconn_connect(&cfg->conn);
}

void umqtt_esp_connect(struct umqtt_esp_config *cfg)
{
	os_printf("umqtt: resolving hostname...\n");
	cfg->connected_callback = cfg->umqtt.connected_callback;
	espconn_gethostbyname((struct espconn *)cfg, cfg->hostname, &cfg->ip, umqtt_esp_dns_cb);
}
