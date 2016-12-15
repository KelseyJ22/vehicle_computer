#include "lwip/opt.h"

#if LWIP_NETCONN

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "lwip/api.h"
#include "lwip/sys.h"


#define UDPCLIENT_THREAD_PRIO  ( tskIDLE_PRIORITY + 3 )

#define BOARD_PORT 6

err_t
udp_send_data(unsigned int addr_dest, struct ip_addr *board_dest,
          struct netconn *conn, void *data, unsigned int data_size)
{
  /* assumes conn will point to valid conn state and that it's UDP */
  err_t err;
  err = netconn_bind (conn, IP_ADDR_ANY, BOARD_PORT);
  if (err == ERR_OK)
  {
    board_dest->addr = htonl(addr_dest);
    err = netconn_connect(conn, board_dest, 7);
    if (err == ERR_OK)
    {
      struct netbuf *buf = netbuf_new();
      char *message;
      
      message = netbuf_alloc(buf, data_size);
      memcpy(message, (char *)data, data_size);
      
      netconn_send(conn, buf); 
      netbuf_delete(buf);
    }
  }
  return err;
}


void udpsend_thread(void *args)
{
    err_t err;
    LWIP_UNUSED_ARG(args);
    
    struct netconn *conn = netconn_new(NETCONN_UDP);
    if (conn == NULL)
      return;
    
    struct ip_addr destIPaddr;
    while (1)
    {
      char *message = "This test\n";
      /* send to 192.168.0.11 */
      err = udp_send_data(0xc0a8000b, &destIPaddr, conn, (void *)message, 10);
      if (err == ERR_OK)
      {
        vTaskDelay(50);
      }
    }
    netconn_delete(conn);
}

void udpsend_init(void) {
  sys_thread_new("udpsend_thread", udpsend_thread, NULL, DEFAULT_THREAD_STACKSIZE, UDPCLIENT_THREAD_PRIO);  
}

#endif /* LWIP_NETCONN */