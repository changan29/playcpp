
#ifndef BUS_BUS_HPP_
#define BUS_BUS_HPP_


#include <stdint.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <assert.h>
#include <iostream>
#include <inttypes.h>
#include <string.h>
namespace Shmbus
{
enum BusErr
{
	SUCCESS = 0,
	GET_SHM_FAILED,
	EXITS_AND_NOT_ENOUGH,
	SIZE_TOO_SMALL,
	BUS_FULL,
	BUS_EMPTY,

};


class Bus
{
private:
    const static uint32_t MAGIC = 1024;
    struct BusInfo
    {
        uint16_t magic;
        uint32_t maxIndex;
        uint64_t readPos;
        uint64_t writePos;
        uint32_t busSize;
        char data[0];
    };
    struct BusPkgHead
    {
        uint32_t magic;
        uint32_t pkgLen;
    };



public:
    Bus():busInfo(NULL),pkgHeadLen(0),busBuf(0){};
    ~Bus(){};


    BusErr Init(uint32_t key, uint32_t size, bool needInit = false)
    {
        int shmid = 0;
        if (size <= (sizeof(BusInfo)) )
        {
        	return SIZE_TOO_SMALL;
        }

        shmid = shmget(key, size, 0644 | IPC_CREAT |IPC_EXCL);
        if (shmid < 0)
        {
            if (errno != EEXIST)
            {
                assert(0);
            	return GET_SHM_FAILED;
            }

            shmid = shmget(key, 0, 0);
            if (shmid < 0)
            {
                return GET_SHM_FAILED;
            }
        }

        busBuf = (char*)shmat(shmid, NULL, 0);

        if (busBuf == (char*)-1)
        {
            return GET_SHM_FAILED;
        }
        Init(busBuf, size, needInit);

        return SUCCESS;
    }

    void Init(char* mem, uint32_t size, bool needInit = false)
    {
        busInfo = reinterpret_cast<BusInfo*>(mem);

        if (needInit)
        {
            memset(mem, 0, size);
            busInfo->magic = MAGIC;
            busInfo->readPos = 0;
            busInfo->writePos = 0;
            busInfo->busSize = size - sizeof(BusInfo);
            busInfo->maxIndex = busInfo->busSize - 1;
        }
        pkgHeadLen = sizeof(BusPkgHead);
    }


    BusErr Write(const char* data, uint32_t size)
    {
        uint64_t rPos = busInfo->readPos;
        uint64_t wPos = busInfo->writePos;
        uint64_t remainLen = busInfo->busSize - (wPos - rPos);
        uint64_t expectLen = size + pkgHeadLen;

        if (remainLen < expectLen)
        {
            return BUS_FULL;
        }
        wPos = wPos % busInfo->busSize;
        rPos = rPos % busInfo->busSize;
        BusPkgHead pkgHead;
        char* pHead = reinterpret_cast<char*>(&pkgHead);

        pkgHead.magic = MAGIC;
        pkgHead.pkgLen = size;


        if (wPos + pkgHeadLen > busInfo->busSize)
        {
        	int32_t n = busInfo->busSize  - wPos;
        	memcpy(busInfo->data + wPos, pHead, n);
        	memcpy(busInfo->data, pHead + n, pkgHeadLen - n);
        }
        else
        {
        	memcpy(busInfo->data + wPos, pHead, pkgHeadLen);
        }
        wPos += pkgHeadLen;
        wPos = wPos % busInfo->busSize;

        if (wPos + size > busInfo->busSize)
        {
        	int32_t n = busInfo->busSize  - wPos;
        	memcpy(busInfo->data + wPos, data, n);
        	memcpy(busInfo->data, data + n, size - n);
        	wPos = size - n;
        }
        else
        {
        	memcpy(busInfo->data + wPos, data, size);
        	wPos += size;
        }

        busInfo->writePos += (pkgHeadLen + size);

        return SUCCESS;
    }
    BusErr Read(char* buf, uint32_t bufSize, uint32_t* readSize)
    {

        uint64_t rPos = busInfo->readPos;
        uint64_t wPos = busInfo->writePos;

        if (rPos == wPos)
        {
        	*readSize = 0;
            return BUS_EMPTY;
        }

        assert(wPos - rPos >= pkgHeadLen);

        wPos = wPos % busInfo->busSize;
        rPos = rPos % busInfo->busSize;

        BusPkgHead pkgHead;
        char* pHead = reinterpret_cast<char*>(&pkgHead);


        if (rPos + pkgHeadLen > busInfo->busSize)
        {
        	int32_t n = busInfo->busSize - rPos;
        	memcpy(pHead, busInfo->data + rPos, n);
        	memcpy(pHead + n, busInfo->data, pkgHeadLen - n);
        	rPos += pkgHeadLen;
        }
        else
        {
        	memcpy(pHead, busInfo->data + rPos, pkgHeadLen);
        	rPos += pkgHeadLen;
        }

        rPos = rPos % busInfo->busSize;
        assert(pkgHead.magic == MAGIC);

        if (pkgHead.pkgLen > bufSize)
        {
        	assert(0);
        }

        if (rPos + pkgHead.pkgLen > busInfo->busSize)
        {
        	int32_t n = busInfo->busSize - rPos;
        	memcpy(buf, busInfo->data + rPos, n);
        	memcpy(buf + n, busInfo->data, pkgHead.pkgLen - n);
        }
        else
        {
        	memcpy(buf, busInfo->data + rPos, pkgHead.pkgLen);

        }
        rPos += pkgHead.pkgLen;
        busInfo->readPos += (pkgHeadLen + pkgHead.pkgLen);
        *readSize = pkgHead.pkgLen;
        return SUCCESS;
    }


private:
    /* data */
    BusInfo *busInfo;
    uint32_t pkgHeadLen;
    char* busBuf;

};




class DupBus
{
public:
	BusErr Init(int32_t shmkey, int32_t shmSize, bool master = false)
	{
	    int shmid = 0;
	    shmid = shmget(shmkey, shmSize, 0644 | IPC_CREAT |IPC_EXCL);
	    if (shmid < 0)
	    {
	        if (errno != EEXIST)
	        {

	            assert(0);
	        	return GET_SHM_FAILED;
	        }
	        shmid = shmget(shmkey, 0, 0);

	        if (shmid < 0)
	        {
	        	assert(0);
	        	return GET_SHM_FAILED;
	        }
	    }

	    busBuf = (char*)shmat(shmid, NULL, 0);

	    if (busBuf == (char*)-1)
	    {
	    	assert(0);
	    	return GET_SHM_FAILED;
	    }

	    if (master)
	    {
	        int32_t recvBufLen = shmSize / 2;
	        int32_t sendBufLen = shmSize - recvBufLen;
	        in.Init(busBuf, recvBufLen, true);
	        out.Init(busBuf + recvBufLen, sendBufLen, true);
	    }
	    else
	    {
	        int32_t sendBufLen = shmSize / 2;
	        int32_t recvBufLen = shmSize - sendBufLen;
	        out.Init(busBuf, sendBufLen, false);
	        in.Init(busBuf + sendBufLen, recvBufLen, false);
	    }

	    return SUCCESS;
	}

	BusErr Write(const char* data, int32_t size)
	{
		return out.Write(data, size);
	}

	BusErr Read(char* data, int32_t bufLen, uint32_t* readSize)
	{
		return in.Read(data, bufLen, readSize);
	}

private:
	Bus in;
	Bus out;
	char* busBuf;
};

}

#endif /* BUS_BUS_HPP_ */
