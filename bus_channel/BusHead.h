
#ifndef ROUTER_BUSHEAD_H_
#define ROUTER_BUSHEAD_H_

#include <stdint.h>
namespace Shmbus
{

const int BUS_SIZE = 20 * 1024 *1024;
const uint16_t BUS_MAGIC = 1024;

enum BusProto
{
	TCONND_CONNECT = 0,
	DISCONNECT,
	CONNTECTED_NOTIFY,
	CONNTECT_FAIL_NOTIFY,
	DATA_TRANS,
    DATA_TRANS_UDP,
	DIS_CONNECTED,
};

struct BusHead
{
	uint16_t magic;
	uint32_t src;
	uint32_t dst;
	uint32_t len;
	uint64_t id; // 或许不需要
	uint32_t innerCmd;
	char data[0];
};

struct CmdTconndConnect
{
	uint64_t id;
	uint32_t accountType;
	char openid[256];
	char url[64];
};

struct CmdUdpConnect
{
	uint64_t id;
	uint16_t port;
	char ip[16];
};

struct CmdDisConnect
{
	uint64_t id;
};

}



#endif /* ROUTER_BUSHEAD_H_ */
