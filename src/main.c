#include "mgos.h"
#include "mgos_http_server.h"
#include "tcp_echo_server.h"

static void cb_api_count(struct mg_connection *c, int ev, void *p, void *user_data){
  (void) p;
  if (ev != MG_EV_HTTP_REQUEST) return;
  LOG(LL_INFO, ("API count requested"));  
  mg_send_response_line(c, 200, 
                        "Content-Type: application/json\r\n"
                        "Access-Control-Allow-Origin: *\r\n"
                        "Connection: close\r\n");

  //mg_printf(c, "%s\r\n", "Fooooo");
  mg_printf(c, "{\"count\":%d}", mgos_get_active_conn_cnt());
  c->flags |= MG_F_SEND_AND_CLOSE;  
}

static void cb_api_slots(struct mg_connection *c, int ev, void *p, void *user_data){
  (void) p;
  if (ev != MG_EV_HTTP_REQUEST) return;

  int tmp = 0;
  LOG(LL_INFO, ("API info requested"));  
  mg_send_response_line(c, 200, 
                        "Content-Type: application/json\r\n"
                        "Access-Control-Allow-Origin: *\r\n"
                        "Connection: close\r\n");
  
  struct mg_connection *client;
  struct client_connection *slot = mgos_get_conn_slots();
  char addr[32];

  mg_printf(c, "[");
  for (int i=0; i<MAX_ALLOWED_ACTIVE_CONNECTIONS; i++){
    if(!slot->in_use){
      slot++;
      continue;
    }

    if(tmp > 0){
      mg_printf(c, ",");
    }
    mg_printf(c, "{");
    mg_printf(c, "\"in_use\":%s", (slot->in_use ? "true" : "false"));
    mg_printf(c, ",\"in_cnt\":%d", slot->msgs_received);
    mg_printf(c, ",\"out_cnt\":%d", slot->msgs_sended);
    if(slot->conn != NULL){
      client = slot->conn;
      mg_sock_addr_to_str(&client->sa, addr, sizeof(addr), MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
      mg_printf(c, ",\"client\":\"%s\"", addr);
    } else {
      mg_printf(c, ",\"client\":null");
    }
    mg_printf(c, "}");
    tmp++;
    slot++;
    
  }
  mg_printf(c, "]");
  c->flags |= MG_F_SEND_AND_CLOSE;
  (void) user_data;
}

enum mgos_app_init_result mgos_app_init(void) {

  struct mg_mgr * mgr = mgos_get_mgr();

  // Init API url handlers
  mgos_register_http_endpoint("/api/conn-count", cb_api_count, NULL);
  mgos_register_http_endpoint("/api/conn-slots", cb_api_slots, NULL);

  return MGOS_APP_INIT_SUCCESS;
}
