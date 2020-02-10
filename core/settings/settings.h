#ifndef _SETTINGS_INC_
#define _SETTINGS_INC_

#include "defs.h"
#include "isettings.h"
#include <map>
#include <string>

namespace NSCore
{
#define SETTING_MAX_NAME_LEN 128
#define SETTING_MAX_VALUE_LEN 256

	class CSetting;

	class CSettingsCollector
	{
	public:
		static bool FASTERCALL IsSettingRegistered(const char * pSettingName);
		static CSetting * FASTERCALL FindSettingByName(const char * pSettingName); //never delete returned value!
		static CSetting * FASTERCALL GetSettingByNumber(unsigned int iNumber);
		static unsigned int GetCountOfSettings();
		static inline void Init() { LoadSettings(); }

	private:
		friend class CSetting;

		struct SSettingInfo
		{
			CSetting * m_pSettingInstance;
			char m_pSettingCurrentValue[SETTING_MAX_VALUE_LEN];
			char m_pSettingDefaultValue[SETTING_MAX_VALUE_LEN];
		};

		struct SSavedSetting
		{
			char m_pSettingName[SETTING_MAX_NAME_LEN];
			char m_pSettingValue[SETTING_MAX_VALUE_LEN];
		};

		static bool FASTERCALL RegisterSetting(CSetting * pInst, const char * pSettingName, const char * pSettingsDefaultValue);
		static bool FASTERCALL UnregisterSetting(const char * pName);
		static void FASTERCALL SetSettingValue(const char * pName, const char * pValue);
		static const char * FASTERCALL GetSettingCurrentValue(const char * pName);
		static void FASTERCALL ResetSettingValueToDefault(const char * pName);

		static void SaveSettings();
		static void LoadSettings();

	private:
		inline static std::map<std::string, SSettingInfo *> & GetSettings()
		{
			//This is HACK! Don't remove this or you will have crashes
			//https://isocpp.org/wiki/faq/ctors#static-init-order-on-first-use
			static std::map<std::string, SSettingInfo *> s_mSettings;
			return s_mSettings;
		}
	};

	class CSetting : public ISetting
	{
	public:
		explicit CSetting(const char * pName, const char * pDefValue);
		virtual ~CSetting();

		virtual void SetValue(const char * pValue) { CSettingsCollector::SetSettingValue(m_cMyName, pValue); RefreshValue(); }
		virtual void ResetValue() { CSettingsCollector::ResetSettingValueToDefault(m_cMyName); RefreshValue(); }

		virtual const char * GetName() const { return m_cMyName; }
		virtual float GetFloat() const { return m_flValue; }
		virtual int GetInt() const { return mu_Value.i; }
		virtual unsigned int GetUInt() const { return mu_Value.u; }
		virtual const char * GetString() const { return m_cValue; }
		virtual bool GetBool() const { return !!mu_Value.i; }

	private:
		friend class CSettingsCollector;

		void RefreshValue();

	private:
		union UValue
		{
			int i;
			unsigned int u;
		}
		mu_Value;
		float m_flValue;
		const char * m_cValue;
		char m_cMyName[SETTING_MAX_NAME_LEN];
	};

	class CSettingsExposer : public ISettingsCollector
	{
	public:
		virtual bool IsSettingRegistered(const char * pSettingName) { return CSettingsCollector::IsSettingRegistered(pSettingName); }
		virtual ISetting * FindSettingByName(const char * pSettingName) { return CSettingsCollector::FindSettingByName(pSettingName); }
		virtual ISetting * GetSettingByNumber(unsigned int iNumber) { return CSettingsCollector::GetSettingByNumber(iNumber); }
		virtual unsigned int GetCountOfSettings() { return CSettingsCollector::GetCountOfSettings(); }
	};

	extern CSettingsExposer * g_pSettingsExposer;
};

#endif