#include "Router.h"
#include "BusHead.h"
#include <unistd.h>
#include <stdio.h>
int main()
{
    //daemon(1, 0);
    Router router;
    router.Init();

    if (!router.AddChild(10001, Shmbus::BUS_SIZE, 10001))
    {
        printf("init shmbus fail");
        return -1;
    }
    if (!router.AddChild(10002, Shmbus::BUS_SIZE, 10002))
    {
        printf("init shmbus fail");
        return -1;
    }

    do
    {
        uint32_t count = router.Process();
        if (count == 0)
        {
            usleep(1000);
        }
    }while(true);


    return 0;
}
