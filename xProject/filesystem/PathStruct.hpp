#pragma once

#include "xProject_pch.hpp"

#include "utils/Utils.hpp"

namespace FileS
{

	const char kPathSeparator = '/';

	static bool IsPathSeparator(const char& c) { return c == kPathSeparator; }

	class PathStruct
	{
	private:
		std::string path;
		std::wstring pathW;
		std::size_t fileLength = 0;

	public:
		explicit PathStruct(const std::string& _path) : path(_path)
		{
			pathW.resize(path.size());
			PathProcessing();
		}

		~PathStruct() = default;

		const std::string& GetPath() const
		{
			return path;
		}

		void SetPath(const char* _path)
		{
			path = _path;
			PathProcessing();
		}

		const std::wstring& GetPathW() const
		{
			return pathW;
		}

		const std::size_t& GetFileLenght() const
		{
			return fileLength;
		}

		std::string GetPathFileName() const
		{
			if (!IsDirectory())
			{
				const std::wstring fileNameW = PathFindFileNameW(pathW.c_str());

				std::string fileNameA;
				fileNameA.resize(fileNameW.size());

				Utils::ConvertUNICODEtoUTF8(fileNameA.data(), fileNameW.c_str(), MAX_PATH);

				return fileNameA;
			}
			return "";
		}

		bool IsAbsolutePath() const
		{
			return !PathIsRelativeW(pathW.c_str());
		}

		bool IsDirectory() const
		{
			return PathIsDirectoryW(pathW.c_str());
		}

		bool IsRootDirectory() const
		{
			return PathIsRootW(pathW.c_str());
		}
	private:
		void PathProcessing()
		{
			Utils::ConvertUTF8toUNOCODE(pathW.data(), path.c_str(), path.size());

			Normalize();
			GettingFileLength();
		}

		void GettingFileLength()
		{
			HANDLE hFile = CreateFileW(pathW.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				return;
			}

			LARGE_INTEGER fileSize;
			if (!GetFileSizeEx(hFile, &fileSize))
			{
				return;
			}

			fileLength = fileSize.QuadPart;

			CloseHandle(hFile);
		}

		void Normalize()
		{
			auto outputIterator = path.begin();
			auto inputIterator = path.cbegin();

			if (path.end() - inputIterator >= 3 && IsPathSeparator(*inputIterator) &&
				IsPathSeparator(*(inputIterator + 1)) && !IsPathSeparator(*(inputIterator + 2))) {
				*(outputIterator++) = kPathSeparator;
				*(outputIterator++) = kPathSeparator;
			}

			while (inputIterator != path.end()) {
				const char character = *inputIterator;
				if (!IsPathSeparator(character)) {
					*(outputIterator++) = character;
				}
				else if (outputIterator == path.begin() || *std::prev(outputIterator) != kPathSeparator) {
					*(outputIterator++) = kPathSeparator;
				}
				++inputIterator;
			}

			path.erase(outputIterator, path.end());
		};
	};
}