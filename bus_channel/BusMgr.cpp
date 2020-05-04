
#include "BusMgr.h"

bool BusMgr::Init(uint32_t key, uint32_t busid, uint32_t busSize)
{
	headLen = sizeof(Shmbus::BusHead);
	BusErr ret = bus.Init(key, busSize);
	if (ret != SUCCESS)
	{
		return false;
	}
	buf = new char[BUS_BUFF_SIZE];
	return true;
}

BusErr BusMgr::Read(Shmbus::BusHead** pHead)
{
	uint32_t len = 0;
	BusErr ret = bus.Read(buf, BUS_BUFF_SIZE, &len);
	if (ret != SUCCESS)
	{
		*pHead = NULL;
	}
	else
	{
		*pHead = reinterpret_cast<Shmbus::BusHead*>(buf);
	}
	return ret;

}

BusErr BusMgr::Write(BusHead* pHead)
{
	return bus.Write((char*)pHead, pHead->len + headLen);
}
