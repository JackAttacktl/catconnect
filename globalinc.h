#ifndef __IMPORTANT_INCLUDED_
#define __IMPORTANT_INCLUDED_

//In fact this file not even needed but working with it kinda more easy

//system
#include <windows.h>
#include <psapi.h>

//encrypt
#include "lazy.h"
#include "xorstr.h"

//multiplayer (mp/src) sdk, can be found here: https://github.com/ValveSoftware/source-sdk-2013
#include "public/globalvars_base.h" //this is special because I'm perfectionist
#include "cdll_int.h"
#include "icliententitylist.h"
#include "iachievementmgr.h"
#include "igameevents.h"

//internal
#include "settings/settings.h"
#include "defs.h"
#include "vmt.h"
#include "sigscan.h"
#include "timers.h"
#include "logger.h"
#include "ctfpartyclient.h"
#include "interfaces.h"
#include "hooksman.h"

//stl
#include <stdlib.h>
#include <stdio.h>
#include <cstring> //strcpy, memcpy, etc. In windows it also included in <string>, but in linux not
#include <string> //std::string
#include <memory>

#endif