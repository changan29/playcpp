#ifndef ROUTER_ROUTER_H_
#define ROUTER_ROUTER_H_
#include "Bus.hpp"
#include <stdint.h>
#include <map>
#include <fstream>

const uint32_t BUf_SIZE = 128 * 1024;
using namespace Shmbus;
class Router
{
public:

    bool Init();
    bool AddChild(uint32_t key, uint32_t busSize, uint32_t busId);
    uint32_t Process();

private:
    std::ofstream cleanFile;
    std::map<uint32_t, DupBus*> busMap;
    char busBuf[BUf_SIZE];

};

#endif /* ROUTER_ROUTER_H_ */