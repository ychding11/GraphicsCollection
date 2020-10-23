// MIT License.

#include "util.h"

#include <tiny_obj_loader.h> //TODO

#include <fstream>
#include <unordered_map>
#include <tuple>
#include <array>
#include <fstream>

// TODO

std::vector<char> util::readFile(const std::string & filename)
{
    std::ifstream file_stream(filename, std::ios::ate | std::ios::binary);

    if (!file_stream.is_open())
    {
        throw std::runtime_error("failed to open file!");
    }

    // starts reading at the end of file to determine file size (ate)
    size_t file_size = (size_t)file_stream.tellg();
    std::vector<char> buffer(file_size);

    file_stream.seekg(0);
    file_stream.read(buffer.data(), file_size);

    file_stream.close();
    return buffer;
}
static std::fstream g_DebugStream("effect_debug.txt", std::fstream::out);
std::string util::writeLog(const char* format, ...)
{
    char buffer[4096] = { 0 };
    va_list args;
    va_start(args, format);
    vsprintf_s(buffer, sizeof(buffer), format, args);
    std::cout << buffer << std::endl;
    return std::string(buffer);
}

