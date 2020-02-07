#ifndef _SETTINGS_INC_
#define _SETTINGS_INC_

#include "defs.h"
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

	class CSetting
	{
	public:
		explicit CSetting(const char * pName, const char * pDefValue);
		~CSetting();

		inline void SetValue(const char * pValue) { CSettingsCollector::SetSettingValue(m_cMyName, pValue); RefreshValue(); }
		inline void ResetValue() { CSettingsCollector::ResetSettingValueToDefault(m_cMyName); RefreshValue(); }

		inline const char * GetName() const { return m_cMyName; }
		inline float GetFloat() const { return m_flValue; }
		inline int GetInt() const { return mu_Value.i; }
		inline unsigned int GetUInt() const { return mu_Value.u; }
		inline const char * GetString() const { return m_cValue; }
		inline bool GetBool() const { return !!mu_Value.i; }

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
};

#endif