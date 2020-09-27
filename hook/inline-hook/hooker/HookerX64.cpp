
#ifdef __x86_64__
#include <cstdlib>
#include <string>
#include <sys/mman.h>
#include "Hooker.h"
#include "HookerX64.h"
#include <cstring>
#include <iostream>

void hooker::HookerX64::doHook(void *func,void *newAddr,void **origFunc) const {

    char *f = (char *)func;
	if (origFunc) {
		// find the return instruction retq: c3
		int index = 0;
		while (true) {
			if (static_cast<uint8_t>(f[index++]) == 0xc3 || index >= 1024) {
				break;
			}
		}

		void *old = malloc(index);
		printf("HookerX64::doHookbackup : %p \n", old);
        if (old != nullptr) 
        {
			memcpy(old, func, index);
			changeCodeAttrs(old, CODE_READ_ONLY);
			*origFunc = old;
		}
	}

	/*
	x64位下使用的跳转是

	jmp 或者 call模式。
	jmp共使用14个字节，0xFF2500000000为6个字节，目标地址为00000000`00000000为8字节。

	call模式
	0xff1500000002为6个字节，目标地址为00000000`00000000为8字节。
	*/


    *(uint16_t *)&f[0] = 0x25ff;
    *(int *)&f[2] = 0x00000000;
    *(long *)&f[6] = (long)newAddr;

    printf("HookerX64::doHook newAddr: %p\n " ,  newAddr);
}
#endif

