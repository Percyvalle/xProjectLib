#pragma once

#include "xProject_pch.hpp"

#include "PathStruct.hpp"

namespace FileS
{

    class FileIO
    {
    private:
        PathStruct filePath;
        std::fstream fileStream;

    public:
        FileIO() = default;
        explicit FileIO(const FileS::PathStruct& _filePath, std::ios_base::openmode _mode)
            : filePath(_filePath), fileStream(_filePath.GetPath(), _mode) {}
        explicit FileIO(const std::string& _filePath, std::ios_base::openmode _mode)
            : filePath(_filePath), fileStream(_filePath, _mode) {}

        ~FileIO() { Close(); }

        bool Open(const PathStruct& _filePath, std::ios_base::openmode _mode)
        {
            fileStream.open(_filePath.GetPath(), _mode);
            return IsOpen();
        }

        bool Close()
        {
            if (IsOpen())
            {
                fileStream.close();
            }
            return !IsOpen();
        }

        bool IsOpen() const
        {
            return fileStream.is_open();
        }

        std::streampos Size()
        {
            fileStream.seekg(0, std::ios::end);
            std::streampos fileSize = fileStream.tellg();
            fileStream.seekg(0, std::ios::beg);
            return fileSize;
        }

        std::istream& Read(char* _str, std::streamsize _size)
        {
            return fileStream.read(_str, _size);
        }

        std::ostream& Write(const char* _str, std::streamsize _size)
        {
            return fileStream.write(_str, _size);
        }
    };
}