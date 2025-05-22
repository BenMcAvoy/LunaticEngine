#include "console.h"

#include "utils.h"

using namespace Lunatic::Debug;

void Console::addLog(const Log& log) {
    std::scoped_lock lock(mutex);
    logs.push_back(log);
}