/* This tool contains various transmission methods that can be easily accessed and
 * remembered. 
 */

#include "tool-transmission.h"
#define TRANSMISSION_PROTOCOL "mesh"

/*---------------------------------------------------------------------------*/
#if TRANSMISSION_PROTOCOL == "mesh"
#include "net/rime/mesh.h"

static struct mesh_conn mesh;

static void
sent(struct mesh_conn *c)
{
	printf("packet sent\n");
}

static void
timedout(struct mesh_conn *c)
{
	printf("packet timedout\n");
}

static void
recv(struct mesh_conn *c, const rimeaddr_t *from, uint8_t hops)
{
	printf("Mesh data received from %d.%d: %s (%d)\n",
			from->u8[0], from->u8[1],
			(char *)packetbuf_dataptr(), packetbuf_datalen());
	
//	packetbuf_copyfrom("Hop", strlen("Hop"));
//	mesh_send(&mesh, from):
}

const static struct mesh_callbacks callbacks = {recv, sent, timedout};

int transmit_mesh(char *message, uint8_t addr_one, uint8_t addr_two)
{
	rimeaddr_t addr;
	packetbuf_copyfrom(message, sizeof(message));
	addr.u8[0] = addr_one;
	addr.u8[1] = addr_two;
	return mesh_send(&mesh, &addr):
}

void open_mesh()
{
	mesh_open(&mesh, 132, &callbacks);
}

void close_mesh()
{
	mesh_close(&mesh);
}


/*---------------------------------------------------------------------------*/
#elif TRANSMISSION_PROTOCOL == "broadcast"
#include "net/rime/broadcast.h"

static void
broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
	printf("Broadcast data received from %d.%d: %s (%d)\n",
			from->u8[0], from->u8[1],
			(char *)packetbuf_dataptr(), packetbuf_datalen());
}

static const broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;

int transmit_broadcast(char *message)
{
	packetbuf_copyfrom(message, sizeof(message));
	broadcast_send(&broadcast);
}

void open_broadcast()
{
	broadcast_open(&broadcast, 129, &broadcast_call);
}

void close_broadcast()
{
	broadcast_close(&broadcast)
}


/*---------------------------------------------------------------------------*/
#elif TRANSMISSION_PROTOCOL == "unicast"
#include "net/rime/unicast.h"

static void
recv_uc(struct unicast_conn *c, const rimeaddr_t *from)
{
	printf("Unicast data received from %d.%d: %s (%d)\n",
			from->u8[0], from->u8[1],
			(char *)packetbuf_dataptr(), packetbuf_datalen());
}

static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct unicast_conn uc;

int transmit_unicast(char *message, uint8_t addr_one, uint8_t addr_two)
{
	rimeaddr_t addr;
	packetbuf_copyfrom(message, sizeof(message));
	addr.u8[0] = addr_one;
	addr.u8[1] = addr_two;
	if (!rimeaddr_cmp(&addr, &rimeaddr_node_addr)) 
		unicast_send(&uc, &addr);
}

void open_unicast()
{
	unicast_open(&uc, 146, &unicast_callbacks);
}

void close_unicast()
{
	unicast_close(&uc);
}

#endif
