#include "tcp_echo_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mgos_rpc.h"
#include "mgos_http_server.h"

static struct mg_connection *s_listen_conn;
volatile static int active_connection_count = 0;

static struct client_connection connections[MAX_ALLOWED_ACTIVE_CONNECTIONS];

int mgos_get_active_conn_cnt(void){
  return active_connection_count;
}

int mgos_get_message_count_per_conn(struct mg_connection *c){
  return 0;
}

struct client_connection *mgos_get_conn_slots(void){
  return (struct client_connection *)&connections;
}

/* Set state of all connection in not used */
static void conn_slot_init(void){
  for (int i = 0; i < MAX_ALLOWED_ACTIVE_CONNECTIONS; i++){
    // This is minimum to handle not used connections
    connections[i].in_use = false;
  }
}

/* Get first free connection slot */
static struct client_connection* conn_slot_get_one(void){
  for (int i = 0; i < MAX_ALLOWED_ACTIVE_CONNECTIONS; i++){
    if(!connections[i].in_use){
      return &connections[i];
    }
  } 
  return NULL;
}

static struct client_connection* conn_slot_get_by_connection(struct mg_connection *c){
  char addr_tmp[32];
  char addr_conn[32];
  struct mg_connection *client;

  mg_sock_addr_to_str(&c->sa, addr_conn, sizeof(addr_conn), MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);

  for (int i = 0; i < MAX_ALLOWED_ACTIVE_CONNECTIONS; i++){
    if(connections[i].in_use){
      client = connections[i].conn;
      mg_sock_addr_to_str(&client->sa, addr_tmp, sizeof(addr_tmp), MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
      if(strcmp((const char *)&addr_tmp, (const char *)&addr_conn) == 0){
        return &connections[i];
      }
    }
  } 
  return NULL;
}

// Connection event handler
static void mgos_tcp_ev(struct mg_connection *c, int ev, void *p, void *user_data) {

  char addr[32];
  int max_conn = mgos_sys_config_get_tcp_max_conn();
  struct client_connection *slot;
  struct mbuf *io = &(c->recv_mbuf);

  mg_sock_addr_to_str(&c->sa, addr, sizeof(addr), MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);

  switch (ev) {
    case MG_EV_ACCEPT: {
      // On new connection accepted
      if(active_connection_count >= max_conn){
        LOG(LL_INFO, ("%p ***TCP REJECT %s", c, addr));
        c->flags |= MG_F_CLOSE_IMMEDIATELY;
      } else {
        LOG(LL_INFO, ("%p ***TCP ACCEPT %s", c, addr));
        slot = conn_slot_get_one();
        if(slot != NULL){
          active_connection_count++;
          slot->in_use = true;
          slot->msgs_received = 0;
          slot->msgs_sended = 0;
          slot->bytes_sended = 0;
          slot->bytes_received = 0;
          slot->conn = c;
        } else {
          LOG(LL_INFO, ("%s Can not find slot in accept phase", addr));
        }
      }
      break;
    } 
    case MG_EV_RECV: {
      // On data receive
      int msg_sz = *((int *)p);
      LOG(LL_INFO, ("***TCP DATA %s : %d", addr, msg_sz));

      slot = conn_slot_get_by_connection(c);
      if(slot != NULL){
        mg_send(c, io->buf, io->len); 
        mbuf_remove(io, io->len);
        slot->msgs_received++;
        slot->bytes_received += msg_sz;
      } else {
        LOG(LL_INFO, ("%s Can not find slot in receive phase", addr));
      }
      break;
    } 
    case MG_EV_SEND: {
      // On data sended
      int bytes_sended = *((int *)p);
      LOG(LL_INFO, ("***TCP %s sended %d bytes", addr, bytes_sended));
      slot = conn_slot_get_by_connection(c);
      if(slot != NULL){
        slot->msgs_sended++;
        slot->bytes_sended += bytes_sended;
      } else {
        LOG(LL_INFO, ("%s Can not find slot in sended phase", addr));
      }
      break;
    } 
    case MG_EV_CLOSE: {
      // On closed connection
      LOG(LL_INFO, ("%p ***TCP CLOSE %s", c, addr));
      slot = conn_slot_get_by_connection(c);
      if(slot != NULL){
        slot->in_use = false;
        slot->conn = NULL;
        if(active_connection_count > 0){
          active_connection_count--;
        }
      } else {
        LOG(LL_INFO, ("%s Can not find slot in close phase", addr));
      }
      break;
    } 
  }

  (void) user_data;
}

static void cb_api_count(struct mg_connection *c, int ev, void *p, void *user_data){
  (void) p;
  if (ev != MG_EV_HTTP_REQUEST) return;
  LOG(LL_INFO, ("API count requested"));  
  mg_send_response_line(c, 200, "Content-Type: text/html\r\n");
  mg_printf(c, "%s\r\n", "Fooooo");
  c->flags |= (MG_F_SEND_AND_CLOSE | MG_F_USER_5);
  (void) user_data;
}

static void cb_api_info(struct mg_connection *c, int ev, void *p, void *user_data){
  (void) p;
  if (ev != MG_EV_HTTP_REQUEST) return;
  LOG(LL_INFO, ("API count requested"));  
  mg_send_response_line(c, 200, "Content-Type: text/html\r\n");
  mg_printf(c, "%s\r\n", "Fooooo");
  c->flags |= (MG_F_SEND_AND_CLOSE | MG_F_USER_5);
  (void) user_data;
}

bool mgos_tcp_echo_server_init(void) {
  LOG(LL_INFO, ("TCP Echo server init"));

  // Check for selected listening port
  if (mgos_sys_config_get_tcp_listen_address() == NULL) {
    LOG(LL_WARN, ("TCP Server disabled, invalid port / address"));
    return true;
  }

  if (mgos_sys_config_get_tcp_max_conn() > MAX_ALLOWED_ACTIVE_CONNECTIONS) {
    LOG(LL_WARN, ("TCP Server can handle max %d connections. Please correct \"tcp.max_conn\" settings.", MAX_ALLOWED_ACTIVE_CONNECTIONS));
    return true;
  }

  struct mg_bind_opts opts;
  memset(&opts, 0, sizeof(opts));

  struct mg_mgr * event_mgr = mgos_get_mgr();

  /* Init connection slots */
  conn_slot_init();

  s_listen_conn = mg_bind(event_mgr, mgos_sys_config_get_tcp_listen_address(), mgos_tcp_ev, NULL);
  if (!s_listen_conn) {
    LOG(LL_ERROR, ("Error binding to [%s]", mgos_sys_config_get_tcp_listen_address()));
    return false;
  }

  /* Limit incoming data buffer size */
  s_listen_conn->recv_mbuf_limit = 512;

  mgos_register_http_endpoint("/api/conn-count", cb_api_count, NULL);
  mgos_register_http_endpoint("/api/conn-info", cb_api_info, NULL);

  return true;
}
