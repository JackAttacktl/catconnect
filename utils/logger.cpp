#include <Windows.h>
#include "printers.h"
#include "logger.h"
#include "lazy.h"
#include "xorstr.h"
#include "autolock.h"
#include "settings/settings.h"

#pragma warning (disable : 4996)

#include <stdarg.h>
#define __STDC_WANT_LIB_EXT1__ 1
#include <time.h>
#include <stdio.h>
#include <string>

extern NSCore::CSetting debug_show;

void NSUtils::CLogger::Init()
{
	char cTemp[MAX_PATH];
	LI_FN(GetModuleFileNameA)(nullptr, cTemp, MAX_PATH);
	std::string sName = cTemp;
	size_t iFound = sName.find_last_of(xorstr_("/\\"));
	if (iFound != std::string::npos)
		sName[iFound + 1] = 0;
	strcpy(ms_cFullPath, sName.c_str());
	strcat(ms_cFullPath, xorstr_("catconnect.log"));
}

void NSUtils::CLogger::Log(ELogType iType, const char * pFormat, ...)
{
#ifndef _DEBUG
	if (iType == NSUtils::ELogType::Log_Debug)
		return;
#endif
	NSUtils::CAutoLocker<std::recursive_mutex> cAutolock(&ms_LogMutex);
	char cLogBuffer[1024];

	va_list ap;
	va_start(ap, pFormat);
	vsnprintf(cLogBuffer, sizeof(cLogBuffer), pFormat, ap);
	va_end(ap);

	char cLogMessage[2048],
		cTimeStamp[64];

	struct tm tmTimeOut;
	time_t tCurTime = time(nullptr);
	tmTimeOut = *localtime(&tCurTime);
	sprintf(cTimeStamp, xorstr_("[%02i/%02i/%04i %02i:%02i:%02i]"), tmTimeOut.tm_mday, tmTimeOut.tm_mon + 1, tmTimeOut.tm_year + 1900, tmTimeOut.tm_hour, tmTimeOut.tm_min, tmTimeOut.tm_sec);

	//file pointer will be closed inside LogActual function
	FILE * pFile = fopen(ms_cFullPath, xorstr_("a+"));

	switch (iType)
	{
	case NSUtils::ELogType::Log_Message:
		sprintf(cLogMessage, xorstr_("%s [MESSAGE] %s\n"), cTimeStamp, cLogBuffer);
		LogActual(cLogMessage, pFile, Color(0, 255, 0, 255));
		break;
	case NSUtils::ELogType::Log_Warning:
		sprintf(cLogMessage, xorstr_("%s [WARNING] %s\n"), cTimeStamp, cLogBuffer);
		LogActual(cLogMessage, pFile, Color(255, 106, 0, 255));
		break;
	case NSUtils::ELogType::Log_Error:
		sprintf(cLogMessage, xorstr_("%s [ERROR] %s\n"), cTimeStamp, cLogBuffer);
		LogActual(cLogMessage, pFile, Color(255, 96, 96, 255));
		break;
#ifdef _DEBUG
	case NSUtils::ELogType::Log_Debug:
		sprintf(cLogMessage, xorstr_("%s [DEBUG] %s\n"), cTimeStamp, cLogBuffer);
		LogActual(cLogMessage, pFile, Color(0, 0, 255, debug_show.GetBool() ? 255 : 0));
		break;
#endif
	case NSUtils::ELogType::Log_Fatal:
		sprintf(cLogMessage, xorstr_("%s [FATAL] %s\n"), cTimeStamp, cLogBuffer);
		LogActual(cLogMessage, pFile, Color(255, 0, 0, 255));
		LI_FN(ExitProcess)(1);
		break;
	default:
		if (pFile)
			fclose(pFile);
		break;
	}
}

void NSUtils::CLogger::LogActual(const char * pLine, FILE * pFile, Color cClr)
{
	if (pFile)
	{
		fwrite(pLine, 1, strlen(pLine), pFile);
		fclose(pFile);
	}

	if (cClr.a())
		NSUtils::PrintToClientConsole(cClr, xorstr_(MSG_PREFIX "%s"), pLine);
}

std::recursive_mutex NSUtils::CLogger::ms_LogMutex;
char NSUtils::CLogger::ms_cFullPath[256];