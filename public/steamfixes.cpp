#include "isteamfriends.h"
#include "strtools.h"
#include "xorstr.h"

const char * CSteamID::Render() const
{
	constexpr int k_cBufLen = 37;
	constexpr int k_cBufs = 4;
	static char rgchBuf[k_cBufs][k_cBufLen];
	static int nBuf = 0;
	char * pchBuf = rgchBuf[nBuf];
	nBuf++; nBuf %= k_cBufs;

	if (k_EAccountTypeAnonGameServer == m_steamid.m_comp.m_EAccountType)
		V_snprintf(pchBuf, k_cBufLen, xorstr_("[A:%u:%u:%u]"), m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID, m_steamid.m_comp.m_unAccountInstance);
	else if (k_EAccountTypeGameServer == m_steamid.m_comp.m_EAccountType)
		V_snprintf(pchBuf, k_cBufLen, xorstr_("[G:%u:%u]"), m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
	else if (k_EAccountTypeMultiseat == m_steamid.m_comp.m_EAccountType)
		V_snprintf(pchBuf, k_cBufLen, xorstr_("[M:%u:%u:%u]"), m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID, m_steamid.m_comp.m_unAccountInstance);
	else if (k_EAccountTypePending == m_steamid.m_comp.m_EAccountType)
		V_snprintf(pchBuf, k_cBufLen, xorstr_("[P:%u:%u]"), m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
	else if (k_EAccountTypeContentServer == m_steamid.m_comp.m_EAccountType)
		V_snprintf(pchBuf, k_cBufLen, xorstr_("[C:%u:%u]"), m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
	else if (k_EAccountTypeClan == m_steamid.m_comp.m_EAccountType)
		V_snprintf(pchBuf, k_cBufLen, xorstr_("[g:%u:%u]"), m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
	else if (k_EAccountTypeChat == m_steamid.m_comp.m_EAccountType)
	{
		if (m_steamid.m_comp.m_unAccountInstance & k_EChatInstanceFlagClan)
			V_snprintf(pchBuf, k_cBufLen, xorstr_("[c:%u:%u]"), m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
		else if (m_steamid.m_comp.m_unAccountInstance & k_EChatInstanceFlagLobby)
			V_snprintf(pchBuf, k_cBufLen, xorstr_("[L:%u:%u]"), m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
		else
			V_snprintf(pchBuf, k_cBufLen, xorstr_("[T:%u:%u]"), m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
	}
	else if (k_EAccountTypeInvalid == m_steamid.m_comp.m_EAccountType)
		V_snprintf(pchBuf, k_cBufLen, xorstr_("[I:%u:%u]"), m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
	else if (k_EAccountTypeIndividual == m_steamid.m_comp.m_EAccountType)
		V_snprintf(pchBuf, k_cBufLen, xorstr_("[U:%u:%u]"), m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
	else if (k_EAccountTypeAnonUser == m_steamid.m_comp.m_EAccountType)
		V_snprintf(pchBuf, k_cBufLen, xorstr_("[a:%u:%u]"), m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
	else
		V_snprintf(pchBuf, k_cBufLen, xorstr_("[i:%u:%u]"), m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
	return pchBuf;
}