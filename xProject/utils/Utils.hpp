#pragma once

#include "xProject_pch.hpp"

#define JsonMSG(key, text) JSON::parse("{\"" + std::string(key) + "\":\"" + std::string(text) + "\"}")
#define JsonMSGDump(key, text) JSON::parse("{\"" + std::string(key) + "\":\"" + std::string(text) + "\"}").dump()

#define AssertMSG(expr, text, format)  if(expr){ spdlog::error(text, format); }

namespace Utils 
{
	inline DWORD GetCountCPU()
	{
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		return sysinfo.dwNumberOfProcessors;
	}

	inline bool ValidateExistsVarJSON(JSON _json, ...)
	{
		va_list args;
		va_start(args, _json);

		const char* variable;
		while ((variable = va_arg(args, const char*)) != nullptr) {
			if (!_json.contains(variable)) {
				va_end(args);
				return false;
			}
		}

		va_end(args);

		return true;
	}

	inline static void ConvertUTF8toUNOCODE(wchar_t* _target, const char* _source, const DWORD& _len)
	{
		MultiByteToWideChar(CP_ACP, 0, _source, -1, _target, _len);
	}

	inline static void ConvertUNICODEtoUTF8(char* _target, const wchar_t* _source, const DWORD& _len)
	{
		WideCharToMultiByte(CP_UTF8, 0, _source, -1, _target, _len, NULL, NULL);
	}
}