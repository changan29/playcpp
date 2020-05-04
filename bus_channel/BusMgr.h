
#ifndef BUSMGR_H_
#define BUSMGR_H_
#include "Bus.hpp"
#include "BusHead.h"
#include "Singleton.h"
const unsigned int BUS_BUFF_SIZE = 64 * 1024;
using namespace Shmbus;
class BusMgr:public Singleton<BusMgr>
{
public:
	bool Init(uint32_t key, uint32_t busid, uint32_t busSize);
	BusErr Read(Shmbus::BusHead** pHead);
	BusErr Write(Shmbus::BusHead* pHead);

	char* getBuf()
	{
		return buf;
	}

private:
	Shmbus::DupBus bus;
	uint32_t headLen;
	char *buf;
};


#endif /* BUSMGR_H_ */
