#include "firmware_loader.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <vector>

static int hexNibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (c >= 'A' && c <= 'F') return 10 + c - 'A';
    return -1;
}

static bool parseHexByte(const std::string& text, size_t pos, uint8_t& out) {
    if (pos + 1 >= text.size())
        return false;
    int hi = hexNibble(text[pos]);
    int lo = hexNibble(text[pos + 1]);
    if (hi < 0 || lo < 0)
        return false;
    out = static_cast<uint8_t>((hi << 4) | lo);
    return true;
}

bool FirmwareLoader::loadHex(const std::string& path, FlashMemory& flash) {
    error.clear();
    std::ifstream file(path);
    if (!file.is_open()) {
        error = "Cannot open HEX firmware file";
        return false;
    }

    FlashMemory candidate(flash.size());
    uint32_t baseAddress = 0;
    bool sawEof = false;
    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty())
            continue;
        if (line[0] != ':' || line.size() < 11 || (line.size() - 1) % 2 != 0) {
            error = "Malformed Intel HEX line " + std::to_string(lineNumber);
            return false;
        }

        std::vector<uint8_t> bytes;
        for (size_t pos = 1; pos < line.size(); pos += 2) {
            uint8_t value = 0;
            if (!parseHexByte(line, pos, value)) {
                error = "Invalid hexadecimal digit on line " + std::to_string(lineNumber);
                return false;
            }
            bytes.push_back(value);
        }
        if (bytes.size() < 5) {
            error = "Intel HEX record too short on line " + std::to_string(lineNumber);
            return false;
        }

        uint8_t count = bytes[0];
        if (bytes.size() != static_cast<size_t>(count) + 5) {
            error = "Intel HEX byte count mismatch on line " + std::to_string(lineNumber);
            return false;
        }

        uint8_t sum = 0;
        for (uint8_t b : bytes)
            sum = static_cast<uint8_t>(sum + b);
        if (sum != 0) {
            error = "Intel HEX checksum mismatch on line " + std::to_string(lineNumber);
            return false;
        }

        uint16_t address = static_cast<uint16_t>((bytes[1] << 8) | bytes[2]);
        uint8_t type = bytes[3];

        if (type == 0x00) {
            uint32_t absolute = baseAddress + address;
            if (absolute + count > candidate.size()) {
                error = "Firmware exceeds simulated Flash size on line " + std::to_string(lineNumber);
                return false;
            }
            for (uint32_t i = 0; i < count; ++i)
                candidate.write(absolute + i, bytes[4 + i]);
        } else if (type == 0x01) {
            if (count != 0) {
                error = "Invalid EOF record on line " + std::to_string(lineNumber);
                return false;
            }
            sawEof = true;
            break;
        } else if (type == 0x02) {
            if (count != 2) {
                error = "Invalid extended segment address record";
                return false;
            }
            uint16_t segment = static_cast<uint16_t>((bytes[4] << 8) | bytes[5]);
            baseAddress = static_cast<uint32_t>(segment) << 4;
        } else if (type == 0x04) {
            if (count != 2) {
                error = "Invalid extended linear address record";
                return false;
            }
            uint16_t upper = static_cast<uint16_t>((bytes[4] << 8) | bytes[5]);
            baseAddress = static_cast<uint32_t>(upper) << 16;
        } else {
            // Start-address and other standard records do not contain Flash
            // payload required by this simulator, so they are safely ignored.
        }
    }

    if (!sawEof) {
        error = "Intel HEX file is missing EOF record";
        return false;
    }

    flash = candidate;
    return true;
}
