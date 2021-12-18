#ifndef _TCP_ECHO_SERVER_
#define _TCP_ECHO_SERVER_

#include "mongoose.h"
#include "mgos.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ALLOWED_ACTIVE_CONNECTIONS  5

struct client_connection {
    bool in_use;
    struct mg_connection *conn;
    int msgs_received;
    int msgs_sended;
    int bytes_sended;
    int bytes_received;
};

/**
  * @brief   Get count of active connections
  * @param   null
  * @return  count of active connections.
  */
int mgos_get_active_conn_cnt(void);

/**
  * @brief   Get count of messages in both direction by connection
  * @param   connection
  * @return  count of messages (-1 if connection is invalid)
  */
int mgos_get_message_count_per_conn(struct mg_connection *c);

/**
  * @brief   Get access to all connection slots
  * @param   null
  * @return  pointer to first slot
  */
struct client_connection *mgos_get_conn_slots(void);

#ifdef __cplusplus
}
#endif

#endif /* _TCP_ECHO_SERVER_ */