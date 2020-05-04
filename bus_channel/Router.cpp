#include "Router.h"
#include "BusHead.h"
#include <unistd.h>
#include <stdio.h>

bool Router::Init()
{
    cleanFile.open("clean.sh", std::ios::trunc);
    cleanFile << "#!/bin/sh" << std::endl;

    return true;
}

bool Router::AddChild(uint32_t key, uint32_t busSize, uint32_t busId)
{
    DupBus* bus = new DupBus();
    BusErr ret = bus->Init(key, busSize, true);

    if (ret != Shmbus::SUCCESS)
    {
        return false;
    }
    busMap.insert(std::make_pair(busId, bus));

    cleanFile << "ipcrm -M " << key << std::endl;

    return true;
}

uint32_t Router::Process()
{
    std::map<uint32_t, DupBus*>::iterator it;
    uint32_t pkgCount = 0;
    for (it = busMap.begin(); it != busMap.end(); it++)
    {
        for (int i = 0; i < 10; i++)
        {
            uint32_t len = 0;
            Shmbus::BusErr ret = it->second->Read(busBuf, BUf_SIZE, &len);
            if (ret == Shmbus::BUS_EMPTY || len == 0)
            {
                break;
            }
            else
            {
                BusHead* head;
                head = reinterpret_cast<BusHead*>(busBuf);
                printf("recv a pkg, id %lu, from %u to %u, len %u \n",head->id, head->src, head->dst, head->len);
                std::map<uint32_t, DupBus*>::iterator dstIt = busMap.find(head->dst);
                if (dstIt != busMap.end())
                {
                    dstIt->second->Write(busBuf, len);
                    pkgCount++;
                }
            }
        }
    }
    return pkgCount;
}
