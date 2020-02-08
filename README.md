# CatConnect

CatConnect is a tool made for communicate with catbots and make them ignore you if possible

## How to compile

To build this you need use Visual Studio 2019 or later
Simply clone it to your local machine and compile it for x86 architecture
If you are getting this error: "...\catconnect\public\tf2sdk\mp\src\public\tier0\memalloc.h(385,11): fatal error C1083: Cannot open include file: typeinfo.h: No such file or directory" while building in debug mode then simply replace #include <typeinfo.h> to #include <typeinfo>

## Credits

* [nullworks](https://github.com/nullworks) - I used a lot of code from cathook
* [LightCat](https://github.com/BenCat07) - for explaining to me how this works
* [cademtz](https://github.com/cademtz) - for his menu code I used to make catconnect menu
* [Altimor](https://gist.github.com/AltimorTASDK) - for his netvar manager cuz I was lazy
* [JustasMasiulis](https://github.com/JustasMasiulis) - for xorstr and lazy importer
* [alliedmodders](https://alliedmods.net) - for fixed bitbuffers
* [Valve](https://www.valvesoftware.com) - for outdated sdk and broken compiled libraries (p.s. I you will modify this tool use lib/tier1_recompiled.lib instead of tier1.lib from ssdk if you don't want to have crashes in tier0.dll && tier1.dll due to KeyValues)