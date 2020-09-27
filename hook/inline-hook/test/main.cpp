
#include "../hooker/HookerFactory.h"
#include <iostream>
#include <cstring>
#include <future>
#include <cstdio>
#include <random>
#include <memory>

void print(const char *s) {
    std::cout << s << std::endl;
}

int (*old_strcmp)(const char *, const char *);


int test_strcmp(const char* str1, const char* str2) 
{
   // std::cout << " raw function test_strcmp"  << std::endl;  加上这一句 或者别的函数调用 会 core 
    if (str1 == NULL || str2 == NULL )
	{
		return -1;
	}
	
    while (*str1 == *str2)
	{
		
		if (*str1 == '\0')
			return 0;
		str1++;
		str2++;
	}
	return *str1 - *str2;
}


int fake_strcmp(const char *s1, const char *s2) 
{
    std::cout << s1 << " " << s2 << ", fake_strcmp" << std::endl;

	if (old_strcmp) {
		std::cout << "old result is: " << old_strcmp(s1, s2) << std::endl;
	}
    return 0;
}

int main() {
    const char *s1 = "hello";
    const char *s2 = "world";

    using namespace hooker;
	std::unique_ptr<HookerFactory> factory = HookerFactory::getInstance();
    const Hooker& hooker = factory->getHooker();
    hooker.hook(reinterpret_cast<void*>(test_strcmp), reinterpret_cast<void*>(fake_strcmp), reinterpret_cast<void**>(&old_strcmp));

    if (test_strcmp(s1,s2) == 0) {
        print("equal");
    } else {
        print("not equal");
    }

    hooker.unhook(reinterpret_cast<void*>(test_strcmp), reinterpret_cast<void*>(old_strcmp));

    if (test_strcmp(s1,s2) == 0) {
        print("equal");
    } else {
        print("not equal");
    }

    return 0;
}
