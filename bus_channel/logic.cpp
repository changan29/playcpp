#include "BusMgr.h"
#include "BusHead.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include <time.h>
#include "Singleton.h"


const int MAX_BUFF_SIZE = 1024 * 64;
const int LOGIC_ID = 10001;

int main()
{
    if (BusMgr::GetInst()->Init(LOGIC_ID, LOGIC_ID, BUS_SIZE) == false)
    {
        printf("init bus fail");
        return -1;
    }
        printf("init bus succ");

    while( true )
    {
        printf("logic recv...\n");
        for (uint32_t i = 0; i < 3; i++)
        {
            Shmbus::BusHead* pHead = NULL;
            Shmbus::BusErr ret;
            ret = BusMgr::GetInst()->Read(&pHead);
            if (ret == Shmbus::BUS_EMPTY)
            {
                break;
            }
            printf(" recv pkg|headid: %lu , len %d \n ", pHead->id, pHead->len);
        }

        sleep(2);

        Shmbus::BusHead* pBusHead = reinterpret_cast<Shmbus::BusHead*>(BusMgr::GetInst()->getBuf());
        srand((unsigned)time(NULL));
        pBusHead->id = 10000 +  rand() % 100 ;
        pBusHead->dst = 10002 ;
        pBusHead->innerCmd = Shmbus::DATA_TRANS;
        pBusHead->magic = Shmbus::BUS_MAGIC;
        pBusHead->src = LOGIC_ID;
        pBusHead->len = sizeof(Shmbus::CmdTconndConnect);
        Shmbus::CmdTconndConnect* pCmd = reinterpret_cast<Shmbus::CmdTconndConnect*>(pBusHead->data);
        pCmd->id = 101;
        pCmd->accountType = 2;
        memcpy(pCmd->openid,  "12345" , 6);
        snprintf(pCmd->url, sizeof(pCmd->url), "%s", "tcp://92:8096");
        Shmbus::BusErr ret = BusMgr::GetInst()->Write(pBusHead);
        if (ret != Shmbus::SUCCESS)
        {
            printf("write to bus fail, %d", ret);
        }
        printf("connect id = %lu, cmd = %u, len = %u \n", pBusHead->id, pBusHead->innerCmd, pBusHead->len);
        sleep(2);

    }

    return 0;
}
