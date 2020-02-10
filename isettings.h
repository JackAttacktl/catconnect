#ifndef __SETTINGS_IFACE_INC
#define __SETTINGS_IFACE_INC

class ISetting
{
public:
	virtual ~ISetting() {}

	virtual void SetValue(const char * pValue) = 0;
	virtual void ResetValue() = 0;

	virtual const char * GetName() const = 0;
	virtual float GetFloat() const = 0;
	virtual int GetInt() const = 0;
	virtual unsigned int GetUInt() const = 0;
	virtual const char * GetString() const = 0;
	virtual bool GetBool() const = 0;
};

class ISettingsCollector
{
public:
	virtual bool IsSettingRegistered(const char * pSettingName) = 0;
	virtual ISetting * FindSettingByName(const char * pSettingName) = 0;
	virtual ISetting * GetSettingByNumber(unsigned int iNumber) = 0;
	virtual unsigned int GetCountOfSettings() = 0;
};

#define SETTINGS_IFACE_VERSION "CSettings001"

#endif