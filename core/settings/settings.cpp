#include "defs.h"
#include "settings.h"
#include "catfiles.h"
#include "cmdwrapper.h"
#include "xorstr.h"
#include "printers.h"
#include "globals.h"
#include "logger.h"
#include <cstring>

#pragma warning (disable : 4996)

NSCore::CCatCommandSafe settingset(xorstr_("ccat_set"), [](const CCommand & rCmd)
{
	if (rCmd.ArgC() < 3)
	{
		NSUtils::PrintToClientConsole(Color(255, 0, 0, 255), xorstr_(MSG_PREFIX "Usage: ccatsettingset <setting> <value>"));
		return true;
	}

	const char * pSetting = rCmd.Arg(1);
	const char * pValue = rCmd.Arg(2);

	auto * pSett = NSCore::CSettingsCollector::FindSettingByName(pSetting);

	if (!pSett)
	{
		NSUtils::PrintToClientConsole(Color(255, 0, 0, 255), xorstr_(MSG_PREFIX "No setting with name \"%s\" found!"), pSetting);
		return true;
	}

	pSett->SetValue(pValue);

	NSUtils::PrintToClientConsole(Color(0, 255, 0, 255), xorstr_(MSG_PREFIX "Value of setting \"%s\" set to \"%s\"!"), pSetting, pValue);
	return true;
});

NSCore::CCatCommandSafe settingget(xorstr_("ccat_get"), [](const CCommand& rCmd)
{
	if (rCmd.ArgC() < 2)
	{
		NSUtils::PrintToClientConsole(Color(255, 0, 0, 255), xorstr_(MSG_PREFIX "Usage: ccatsettingget <setting>"));
		return true;
	}

	const char * pSetting = rCmd.Arg(1);

	auto * pSett = NSCore::CSettingsCollector::FindSettingByName(pSetting);

	if (!pSett)
	{
		NSUtils::PrintToClientConsole(Color(255, 0, 0, 255), xorstr_(MSG_PREFIX "No setting with name \"%s\" found!"), pSetting);
		return true;
	}

	const char * pValue = pSett->GetString();

	NSUtils::PrintToClientConsole(Color(0, 255, 0, 255), xorstr_(MSG_PREFIX "Value of setting \"%s\" = \"%s\"."), pSetting, pValue);
	return true;
});

NSCore::CCatCommandSafe settingslist(xorstr_("ccat_list"), [](const CCommand& rCmd)
{
	unsigned int iCountOfSettings = NSCore::CSettingsCollector::GetCountOfSettings();

	bool bSettingFound = false;

	NSUtils::PrintToClientConsole(Color(0, 255, 0, 255), xorstr_(MSG_PREFIX "List of settings:"));

	for (unsigned int iSetting = 0; iSetting < iCountOfSettings; iSetting++)
	{
		auto * pSetting = NSCore::CSettingsCollector::GetSettingByNumber(iSetting);
		NSUtils::PrintToClientConsole(Color(0, 255, 0, 255), xorstr_("Value of setting \"%s\" = \"%s\""), pSetting->GetName(), pSetting->GetString());
		bSettingFound = true;
	}

	if (!bSettingFound)
		NSUtils::PrintToClientConsole(Color(0, 255, 0, 255), xorstr_("No settings found."));

	return true;
});

NSCore::CSetting * NSCore::CSettingsCollector::FindSettingByName(const char * pSettingName)
{
	for (auto sSet : GetSettings())
	{
		if (!strcmp(sSet.first.c_str(), pSettingName))
			return sSet.second->m_pSettingInstance;
	}
	return nullptr;
}

NSCore::CSetting * NSCore::CSettingsCollector::GetSettingByNumber(unsigned int iNumber)
{
	if (iNumber >= GetCountOfSettings())
		return nullptr;
	auto iIter = GetSettings().begin();
	for (unsigned int iNum = 0; iNum != iNumber; iNum++)
		iIter++;
	return iIter->second->m_pSettingInstance;
}

bool NSCore::CSettingsCollector::IsSettingRegistered(const char * pSettingName) { if (!GetCountOfSettings()) return false; return (GetSettings().find(pSettingName) != GetSettings().end()); }
unsigned int NSCore::CSettingsCollector::GetCountOfSettings() { return GetSettings().size(); }

bool NSCore::CSettingsCollector::RegisterSetting(CSetting * pInst, const char * pSettingName, const char * pSettingsDefaultValue)
{
	if (IsSettingRegistered(pSettingName))
		return false;
	SSettingInfo * pSettingInfo = new SSettingInfo;
	pSettingInfo->m_pSettingInstance = pInst;
	strcpy(pSettingInfo->m_pSettingDefaultValue, pSettingsDefaultValue);
	strcpy(pSettingInfo->m_pSettingCurrentValue, pSettingsDefaultValue);
	GetSettings()[pSettingName] = pSettingInfo;
	return true;
}

bool NSCore::CSettingsCollector::UnregisterSetting(const char * pName)
{
	if (!IsSettingRegistered(pName))
		return false;
	auto pInfo = GetSettings()[pName];
	GetSettings().erase(pName);
	delete pInfo;
	return true;
}

void NSCore::CSettingsCollector::SetSettingValue(const char * pName, const char * pValue)
{
	if (!IsSettingRegistered(pName))
		return;
	auto pInfo = GetSettings()[pName];
	strcpy(pInfo->m_pSettingCurrentValue, pValue);
	SaveSettings();
	CCatFiles::SaveData();
}

const char * NSCore::CSettingsCollector::GetSettingCurrentValue(const char * pName)
{
	if (!IsSettingRegistered(pName))
		return nullptr;
	auto pInfo = GetSettings()[pName];
	return pInfo->m_pSettingCurrentValue;
}

void NSCore::CSettingsCollector::ResetSettingValueToDefault(const char * pName)
{
	if (!IsSettingRegistered(pName))
		return;
	auto pInfo = GetSettings()[pName];
	strcpy(pInfo->m_pSettingCurrentValue, pInfo->m_pSettingDefaultValue);
}

void NSCore::CSettingsCollector::LoadSettings()
{
	auto oSettings = CCatFiles::GetSectionByID(Section_Settings);
	if (!oSettings.has_value())
	{
		NSUtils::CLogger::Log(NSUtils::Log_Error, xorstr_("Unable to load settings: Invalid setting container!"));
		return; //should never happen
	}
	auto& vSettings = oSettings.value();
	if (!(*vSettings)->size())
		return; //there are no settings. Is this first run?
	const char * pActualData = (*vSettings)->data();
	uint32_t iCountOfSettings = uint32_t((*vSettings)->size() / sizeof(SSavedSetting));
	for (uint32_t iSetting = 0; iSetting < iCountOfSettings; iSetting++)
	{
		SSavedSetting * pSetting = (SSavedSetting *)((char*)pActualData + sizeof(SSavedSetting) * iSetting);
		if (!IsSettingRegistered(pSetting->m_pSettingName))
			continue;
		auto pSett = GetSettings()[pSetting->m_pSettingName];
		strcpy(pSett->m_pSettingCurrentValue, pSetting->m_pSettingValue);
		pSett->m_pSettingInstance->RefreshValue();
	}

	if (!g_pSettingsExposer)
	{
		//create exposer here
		static CSettingsExposer cExposer;
		g_pSettingsExposer = &cExposer;
		NSGlobals::g_mMyInterfaces[xorstr_(SETTINGS_IFACE_VERSION)] = g_pSettingsExposer;
	}
}

void NSCore::CSettingsCollector::SaveSettings()
{
	auto oSettings = CCatFiles::GetSectionByID(Section_Settings);
	if (!oSettings.has_value())
	{
		NSUtils::CLogger::Log(NSUtils::Log_Error, xorstr_("Unable to save settings: Invalid setting container!"));
		return; //should never happen
	}
	auto& vSettings = oSettings.value();
	(*vSettings)->clear();
	SSavedSetting sSetting;
	for (auto pSet : GetSettings())
	{
		memset(&sSetting, 0, sizeof(sSetting));
		strcpy(sSetting.m_pSettingName, pSet.first.c_str());
		strcpy(sSetting.m_pSettingValue, pSet.second->m_pSettingCurrentValue);
		(*vSettings)->insert((*vSettings)->end(), (char*)&sSetting, (char*)&sSetting + sizeof(SSavedSetting));
	}
}

NSCore::CSetting::CSetting(const char * pName, const char * pDefValue) : m_cValue(nullptr), m_flValue(0.0)
{
	mu_Value.i = 0;
	if (!CSettingsCollector::RegisterSetting(this, pName, pDefValue))
	{
		NSUtils::CLogger::Log(NSUtils::Log_Warning, xorstr_("Unable to register setting: Setting \"%s\" (def. value: \"%s\") already registered!"), pName, pDefValue);
		m_cMyName[0] = 0;
		return;
	}
	strcpy(m_cMyName, pName);
	RefreshValue();
}

NSCore::CSetting::~CSetting()
{
	if (*m_cMyName)
		CSettingsCollector::UnregisterSetting(m_cMyName);
}

void NSCore::CSetting::RefreshValue()
{
	if (!*m_cMyName)
		return;
	const char * pActualValue = CSettingsCollector::GetSettingCurrentValue(m_cMyName);
	mu_Value.i = atoi(pActualValue);
	char * pDummy;
	m_flValue = strtof(pActualValue, &pDummy);
	m_cValue = pActualValue;
}

NSCore::CSettingsExposer * NSCore::g_pSettingsExposer = nullptr;