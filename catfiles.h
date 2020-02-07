#ifndef _CATFILES_INC_
#define _CATFILES_INC_

#include <memory>
#include <vector>
#include <map>
#include <optional>
#include <stdint.h>
#include "defs.h"

//WARNING: Also don't forget about versions struct in catfiles.cpp!!!
enum ESections : uint16_t
{
	Section_Cats = 0,
	Section_Settings,

	Sections_Max
};

class CCatFiles
{
	//private
	//save structs here

#pragma pack (push, 1)
	struct SSaveFileHeader
	{
		uint32_t m_iFileMagic;
		uint8_t m_iFileVersion;
		uint16_t m_iSectionsCount;
	};

	struct SSectionHeader
	{
		uint16_t m_iSectionID;
		uint32_t m_iDataOffset; //relative to file start
		uint32_t m_iSectionSize; //size of section in bytes
		uint8_t m_iSectionVersion; //version of section
	};
#pragma pack (pop)

public:
	static void SaveData();
	static bool LoadData();
	static inline std::optional<std::unique_ptr<std::vector<char>> *> GetSectionByID(ESections iSection) { if (iSection < 0 || iSection >= Sections_Max) return std::nullopt; return &ms_mSavedData[iSection]; }

private:
	static void FASTERCALL SaveData(std::map<uint16_t, std::unique_ptr<std::vector<char>>> &);
	static bool FASTERCALL LoadData(std::map<uint16_t, std::unique_ptr<std::vector<char>>> &);
	static void FASTERCALL SaveSectionByID(SSectionHeader * pHeader, std::unique_ptr<std::vector<char>> &, std::vector<char> & vToWrite);
	static void FASTERCALL WriteOut(const char * pData, uint32_t iSize);
	static void FASTERCALL ReadSavedFile(char ** pBuffer, uint32_t &iSize);
	static char * GetSavePath();

private:

	static bool ms_bLoaded;
	static std::map<uint16_t, std::unique_ptr<std::vector<char>>> ms_mSavedData;
};

#endif