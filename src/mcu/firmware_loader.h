#pragma once

#include <string>

#include "flash_memory.h"

class FirmwareLoader {
public:
    bool loadHex(const std::string& path, FlashMemory& flash);
    const std::string& lastError() const { return error; }

private:
    std::string error;
};
