#ifndef __LOGGER_H_
#define __LOGGER_H_

#include <mutex>
#include "Color.h"

namespace NSUtils
{
	enum ELogType : unsigned char
	{
		Log_Message,
		Log_Warning,
		Log_Error,
		Log_Debug,
		Log_Fatal
	};

	class CLogger
	{
	public:
		static void Init();

	public:
		static void Log(ELogType iType, const char * pFormat, ...);

	private:
		static void LogActual(const char * pLine, FILE * pFile, Color cClr);

	private:
		static std::recursive_mutex ms_LogMutex;
		static char ms_cFullPath[256];
	};
};

#endif