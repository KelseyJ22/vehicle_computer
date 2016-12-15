#include "lwip/opt.h"

#if LWIP_NETCONN

#include "lwip/sys.h"
#include "lwip/api.h"

#include "stm32f4xx.h"
#include "stm32f4x7_eth_bsp.h"


#define UDPRECV_THREAD_PRIO  ( tskIDLE_PRIORITY + 3 )


static struct netbuf *buf;

static void udprecv_thread(void *arg)
{
  struct netconn *conn = netconn_new(NETCONN_UDP);
  if (conn!= NULL) {
    err_t err = netconn_bind(conn, IP_ADDR_ANY, 7);
    if (err == ERR_OK) {
      while (1) {
        buf = netconn_recv(conn); 
      
        if (buf != NULL) {
          STM_EVAL_LEDOn(LED5);
          vTaskDelay(500);
          char data[10];        
          netbuf_copy(buf, data, 10);
          data[9] = '\0';
          printf("%s\n", data); /* for debugging purposes */
          STM_EVAL_LEDOff(LED5);
          vTaskDelay(500);
          netbuf_delete(buf);
        }
      }
      netconn_delete(conn);
    }
  }
}       

void udprecv_init(void)
{
  sys_thread_new("udprecv_thread", udprecv_thread, NULL, DEFAULT_THREAD_STACKSIZE, UDPRECV_THREAD_PRIO);
}
#endif /* LWIP_NETCONN */