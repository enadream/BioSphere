#ifndef FILE_IO_HPP
#define FILE_IO_HPP

#include <fstream>
#include <string>

static int LoadFile(const char* file_name, std::string &out_file){
    // read file
    std::ifstream file(file_name, std::ios::in | std::ios::ate);

    // check the file
    if (!file.is_open()){
        printf("[ERROR]: An error occurred while opening the file \n");
        return 0;
    }

    // find the file size
    std::streampos end_pos = file.tellg();
    uint64_t size = end_pos - file.seekg(0, std::ios::beg).tellg();
    out_file.resize(size);
    file.read(&out_file[0], size);
    file.close();

    return 1;
}

#endif