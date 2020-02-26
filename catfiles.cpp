#include "catfiles.h"
#include "logger.h"
#include "lazy.h"
#include "xorstr.h"
#include <Shlobj.h>
#include <cstring>
#include <vector>
#include <stdio.h>

#define FILE_MAGIC 0x43435356 //CCSV - CatConnect Save File
#define CURRENT_FILE_VERSION 1

struct SSectionVersions
{
	uint16_t iSection;
	uint8_t iVersion;
};

//change version if new saving format incompatible with old
static const SSectionVersions gs_SectionVersions[] =
{
	{Section_Cats, 1},
	{Section_Settings, 2},

	{Sections_Max, 0}
};

void CCatFiles::SaveData()
{
	if (ms_bLoaded)
		SaveData(ms_mSavedData);
}

bool CCatFiles::LoadData()
{
	if (ms_bLoaded)
		return true;
	
	//first init our container

	for (uint16_t iSec = 0; iSec < Sections_Max; iSec++)
		ms_mSavedData.insert(std::make_pair(iSec, std::unique_ptr<std::vector<char>>(new std::vector<char>())));

	ms_bLoaded = true;

	//now load it
	return LoadData(ms_mSavedData);
}

void CCatFiles::SaveData(std::map<uint16_t, std::unique_ptr<std::vector<char>>> & mDataBuffers)
{
	std::vector<char> vData; vData.clear();
	SSaveFileHeader sHeader;
	sHeader.m_iFileMagic = FILE_MAGIC;
	sHeader.m_iFileVersion = CURRENT_FILE_VERSION;
	sHeader.m_iSectionsCount = Sections_Max;
	vData.insert(vData.begin(), (char *)&sHeader, (char *)&sHeader + sizeof(SSaveFileHeader));

	for (uint16_t iSection = 0; iSection < Sections_Max; iSection++)
	{
		SSectionHeader sSectionHeader;
		sSectionHeader.m_iSectionID = iSection;
		sSectionHeader.m_iSectionSize = 0; //Will be set later
		sSectionHeader.m_iDataOffset = 0; //Will be set later
		vData.insert(vData.end(), (char*)&sSectionHeader, (char*)&sSectionHeader + sizeof(SSectionHeader));
	}

	for (uint16_t iSection = 0; iSection < Sections_Max; iSection++)
	{
		SSectionHeader * pSectionHdr = (SSectionHeader *)((char *)vData.data() + sizeof(SSaveFileHeader) + sizeof(SSectionHeader) * iSection);
		pSectionHdr->m_iSectionVersion = gs_SectionVersions[iSection].iVersion;
		SaveSectionByID(pSectionHdr, mDataBuffers[iSection], vData);
	}

	WriteOut(vData.data(), (uint32_t)vData.size());
}

bool CCatFiles::LoadData(std::map<uint16_t, std::unique_ptr<std::vector<char>>> & mDataBuffers)
{
	char * pBuf = nullptr;
	uint32_t iSize = 0;
	ReadSavedFile(&pBuf, iSize);
	if (!iSize) return false; //no data

	SSaveFileHeader * pHeader = (SSaveFileHeader *)pBuf;
	if (pHeader->m_iFileMagic != FILE_MAGIC)
	{
		delete[] pBuf;
		return false;
	}

	if (pHeader->m_iFileVersion != CURRENT_FILE_VERSION)
	{
		delete[] pBuf;
		return false;
	}

	SSectionHeader * pSectionHeaders = (SSectionHeader *)(++pHeader);

	for (uint16_t iSection = 0; iSection < Sections_Max; iSection++)
	{
		SSectionHeader sSectionHdr = pSectionHeaders[iSection];

		mDataBuffers[iSection]->clear();

		if (sSectionHdr.m_iSectionSize == 0 || sSectionHdr.m_iDataOffset == 0)
			continue; //section has no data

		if (sSectionHdr.m_iSectionVersion != gs_SectionVersions[iSection].iVersion)
		{
			NSUtils::CLogger::Log(NSUtils::ELogType::Log_Warning, xorstr_("Section %i didn't load because of incompatible version. Current version: %i, section version: %i"), iSection, gs_SectionVersions[iSection].iVersion, sSectionHdr.m_iSectionVersion);
			continue;
		}

		//write saved data
		mDataBuffers[iSection]->insert(mDataBuffers[iSection]->begin(), pBuf + sSectionHdr.m_iDataOffset, pBuf + sSectionHdr.m_iDataOffset + sSectionHdr.m_iSectionSize);
	}

	delete[] pBuf;
	return true;
}

void CCatFiles::SaveSectionByID(SSectionHeader * pHeader, std::unique_ptr<std::vector<char>> & vFrom, std::vector<char> & vTo)
{
	pHeader->m_iDataOffset = vTo.size();
	pHeader->m_iSectionSize = vFrom->size();
	vTo.insert(vTo.end(), vFrom->begin(), vFrom->end());
}

void CCatFiles::WriteOut(const char * pData, uint32_t iSize)
{
	char * pSavePath = GetSavePath();
	if (!pSavePath || !*pSavePath)
	{
		NSUtils::CLogger::Log(NSUtils::ELogType::Log_Error, xorstr_("Unknown error: unable to get save path!"));
		return;
	}

	FILE * pFile = fopen(pSavePath, xorstr_("wb"));
	if (!pFile)
	{
		NSUtils::CLogger::Log(NSUtils::ELogType::Log_Error, xorstr_("Unable to open file for write: %s"), pSavePath);
		memset(pSavePath, 0, strlen(pSavePath));
		return;
	}

	memset(pSavePath, 0, strlen(pSavePath));

	fwrite(pData, 1, iSize, pFile);
	fclose(pFile);
}

void CCatFiles::ReadSavedFile(char ** pBuffer, uint32_t & iSize)
{
	iSize = 0; //just to be sure
	char * pSavePath = GetSavePath();
	if (!pSavePath || !*pSavePath)
	{
		NSUtils::CLogger::Log(NSUtils::ELogType::Log_Error, xorstr_("Unknown error: unable to get save path!"));
		return;
	}

	FILE * pFile = fopen(pSavePath, xorstr_("rb"));
	if (!pFile)
	{
		//there is no saved file
		memset(pSavePath, 0, strlen(pSavePath));
		return;
	}

	memset(pSavePath, 0, strlen(pSavePath));

	fseek(pFile, 0, SEEK_END);
	size_t szSize = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	if (!szSize)
		return; //file is empty?

	iSize = (uint32_t)szSize;
	*pBuffer = new char[iSize];
	fread(*pBuffer, 1, iSize, pFile);
	fclose(pFile);
}

char * CCatFiles::GetSavePath()
{
	static char s_cRetPath[256] = { 0 };
	if (*s_cRetPath) return s_cRetPath; //this must be always null but ok
	if (SUCCEEDED(LI_FN(SHGetFolderPathA)(nullptr, CSIDL_APPDATA | CSIDL_FLAG_CREATE, nullptr, 0, s_cRetPath)))
	{
		//it returns path to appdata/roaming
		strcat(s_cRetPath, xorstr_("\\cc.save"));
		return s_cRetPath;
	}
	return const_cast<char *>(""); //heck! Be aware
}

bool CCatFiles::ms_bLoaded = false;
std::map<uint16_t, std::unique_ptr<std::vector<char>>> CCatFiles::ms_mSavedData;