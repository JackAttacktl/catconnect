#ifndef GLOBALVARS_BASE_H
#define GLOBALVARS_BASE_H

class CSaveRestoreData;

class CGlobalVarsBase
{
public:
	float					m_flRealTime; //absolute time (per frame still - Use Plat_FloatTime() for a high precision real time perf clock, but not that it doesn't obey host_timescale/host_framerate)
	int						m_iFrameCount; //absolute frame counter
	float					m_flAbsoluteFrameTime; //non-paused frametime
	/*
	Current time:
	On the client, this (along with tickcount) takes a different meaning based on what
	piece of code you're in:
	 
	- While receiving network packets (like in PreDataUpdate/PostDataUpdate and proxies),
	  this is set to the SERVER TICKCOUNT for that packet. There is no interval between
	  the server ticks.
	  [server_current_Tick * tick_interval]
	
	- While rendering, this is the exact client clock 
	  [client_current_tick * tick_interval + interpolation_amount]
	
	- During prediction, this is based on the client's current tick:
	  [client_current_tick * tick_interval]
	*/
	float					m_flCurTime; //aka engine time, full explanation above
	float					m_flFrameTime; //time spent on last server or client frame (has nothing to do with think intervals)
	int						m_iMaxClients; //in tf2 value between 1 and 33. Depends on server setting
	int						m_iTickCount; //simulation ticks
	float					m_flIntervalPerTick; //simulation tick interval
	float					m_flInterpolationAmount; //interpolation amount ( client-only ) based on fraction of next tick which has elapsed
	int						m_iSimulatedTicksThisFrame; //used in ProcessUserCmds function on server-side
	int						m_iNetProtocol; //???
	CSaveRestoreData *		m_pSaveData; //current saverestore data

//private:
	bool					m_bClient; //in our case always true
	int						m_nTimestampNetworkingBase;
	int						m_nTimestampRandomizeWindow;
};

#endif