#pragma once

#include "xProject_pch.hpp"

#include "PathStruct.hpp"

namespace FileS
{

	class FileSystemManager
	{
	public:
		explicit FileSystemManager() = default;
		~FileSystemManager() = default;

		PathStruct GetWorkingDir() const
        {
            std::wstring pathW;
            pathW.resize(MAX_PATH);
            DWORD writeLen = GetCurrentDirectoryW(MAX_PATH, pathW.data());

            std::string pathA;
            pathA.resize(writeLen);
            Utils::ConvertUNICODEtoUTF8(pathA.data(), pathW.c_str(), writeLen);

            return FileS::PathStruct(pathA);
        }

		bool SetWorkingDir(const PathStruct& _path) const
        {
            return SetCurrentDirectoryW(_path.GetPathW().c_str());
        }

		static bool DirectoryExists(const PathStruct& _path)
        {
            return PathIsDirectoryW(_path.GetPathW().c_str());
        }

		static bool FileExists(const PathStruct& _path)
        {
            return !_path.IsDirectory() && PathFileExistsW(_path.GetPathW().c_str());
        }
	};
}