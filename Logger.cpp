#include "stdafx.h"
#include "Logger.h"
#include <cstdarg>

const size_t MAX_LOG_SIZE = 4096;

cLogger* cLogger::s_instance(nullptr);

//TODO 
// - severity and timestamp
// - custom tags (e.g. [Mircea] blabla)
// - write to file non-blocking
// - write to console

cLogger::cLogger()
{
}

cLogger::~cLogger()
{
}

cLogger* cLogger::GetInstance()
{
	if( !s_instance )
		s_instance = new cLogger();

	return s_instance;
}

void cLogger::DestroyInstance()
{
	if (s_instance)
	{
		delete s_instance;
		s_instance = nullptr;
	}
}

void cLogger::Print(const std::string& fmt, ... )
{
	char szBuffer[MAX_LOG_SIZE] = { 0 };

	va_list args;
	va_start(args, fmt.c_str());
	_vsnprintf_s( szBuffer, MAX_LOG_SIZE - 2u, _TRUNCATE, fmt.c_str(), args);//-3 to leave room for \r\n at the very end
	size_t bufferLen = strnlen_s(szBuffer, _TRUNCATE);
	_vsnprintf_s( szBuffer + bufferLen, MAX_LOG_SIZE - bufferLen - 1, MAX_LOG_SIZE - bufferLen - 1, "\n", args);
	va_end(args);

#ifdef _WIN32
	OutputDebugStringA( szBuffer );
#endif
}
