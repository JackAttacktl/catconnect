#ifndef __ICATCONNECT_IFACE_INC_
#define __ICATCONNECT_IFACE_INC_

enum ECatState : uint8_t
{
	CatState_Default, //random player
	CatState_Cat, //cathook user / catbot
	CatState_FakeCat, //catconnect user
	CatState_Friend, //friend in steam
	CatState_Party, //party member
	CatState_VoteBack //player who must be kicked
};

class ICatConnect
{
public:
	virtual ~ICatConnect() {}

	virtual ECatState GetClientState(int iClient) = 0;
	virtual void Trigger() = 0;
};

#define CATCONNECT_IFACE_VERSION "CCatConnect001"

#endif