
#include <utility>
#include <memory>
#include <mutex>
#include "HookerFactory.h"
#include "HookerX64.h"

#include "HookerError.h"

std::unique_ptr<hooker::HookerFactory> hooker::HookerFactory::getInstance() {
	using namespace utils::design_pattern;
	HookerFactory* result =  Singleton<HookerFactory,MultiThreadPolicy>::getInstance();
	return std::unique_ptr<HookerFactory>(result);
}

hooker::Hooker* hooker::HookerFactory::createHooker() {
    return new hooker::HookerX64;
}

const hooker::Hooker& hooker::HookerFactory::getHooker() const {
	return *hooker;
}

