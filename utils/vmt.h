/*
	Virtual Member Table Functions Manager

	Author: bluecutty
	This is x86 only! (cuz tf2 binaries has x86 bit capacity)

	Todo:
	Virtual Member Table manager (easy) - Done
	Implement RTTI information wrapper (easy): - Done
		DoRTTICheck - Done
		IsChildOf - Done
		GetRTTIClass - Done
	Implement custom dynamic cast (easy): - Done
		Implement FindCompleteThisPointer function (we can get parents / childs and their pointers from RTTI) (easy) - Done
		Implement FindInterfaceThisPointer function (and custom dyncast) (easy) - Done
	Add support to hook member function by its name (ala HookFunction(ThisPointer, Class, FuncName, Callback)) (easy) - Done but not fully tested yet
	Find function offset from its signature (easy)
	Implement per-base hooks (easy, simply copy whole table and edit it instead of patching .rodata section)
	Implement hooks sync (easy, but is this actually needed?)
	Port it to linux - WIP (90%, I need to recheck some things to make sure everything implemented correctly)
*/

#ifndef __VMT_INC
#define __VMT_INC

#ifdef _WIN32
#include <Windows.h>
#include "lazy.h"
#include <boost/algorithm/string.hpp>
//disable stupid warnings
#pragma warning (disable : 4996)
#pragma warning (disable : 4200)
#pragma warning (disable : 4700)
#else //linux todo
#include <cxxabi.h>
#include <stdexcept>
#include <segvcatch.h> //https://code.google.com/archive/p/segvcatch/
#include <sys/mman.h>
#include <signal.h>
#include <stdlib.h>
#ifndef PAGESIZE_X86
#define	PAGESIZE_X86 4096 //memory page size always 4096 in x86
#endif
#define ALIGN(alig) (((int)alig) & ~(PAGESIZE_X86 - 1))
//also redefine thiscall in linux
#define __thiscall
#endif
#include <stdint.h>
#include <map>
#include <vector>
#include <string>
#include "xorstr.h"

#define CALL_VFUNC_OFFS(funcdef, vthis, offset) ((funcdef)((*(void ***)vthis)[offset]))
#define CALL_VFUNC_NAME(instance, name) (instance->*name)
#define VMT_NOATTRIBS

namespace NSUtils
{
	//RTTI functions and structs (x86 only cuz source engine libraries in tf2 are x86)
	//Here we can work with rtti information: check class parents, check class name, etc
	//If you need x64 one you can do it by yourself
	//In linux it works similar, but there it will be __class_type_info blah blah blah
	//If you are is LightCat, here Linux example of working with rtti information: https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/libsupc%2B%2B/dyncast.cc#L45-L95
	//More info about vtables and rtti: https://yurichev.com/mirrors/RE/Recon-2012-Skochinsky-Compiler-Internals.pdf (english version, if you need russian one, just google it)

	namespace NSRTTI
	{
#ifdef _WIN32
#pragma pack(push, 1)
		struct STypeInfo
		{
			void * m_pVTable;											// STypeInfo class vftable
			void * _M_data;												// nullptr until loaded at runtime
			char _M_d_name[1];											// Mangled name (prefix: .?AV=classes, .?AU=structs)
		};

		typedef STypeInfo _RTTITypeDescriptor;

		//Base class "Pointer to Member Data"
		struct PMD
		{
			int m_mdisp;												// 00 Member displacement
			int m_pdisp;												// 04 Vftable displacement
			int m_vdisp;												// 08 Displacement inside vftable
		};

		//Describes all base classes together with information to derived class access dynamically attributes flags
		constexpr uint32_t BCD_NOTVISIBLE = 0x01;
		constexpr uint32_t BCD_AMBIGUOUS = 0x02;
		constexpr uint32_t BCD_PRIVORPROTINCOMPOBJ = 0x04;
		constexpr uint32_t BCD_PRIVORPROTBASE = 0x08;
		constexpr uint32_t BCD_VBOFCONTOBJ = 0x10;
		constexpr uint32_t BCD_NONPOLYMORPHIC = 0x20;
		constexpr uint32_t BCD_HASPCHD = 0x40;

		struct _RTTIBaseClassDescriptor
		{
			_RTTITypeDescriptor * m_pTypeDescriptor;					// 00 Type descriptor of the class
			uint32_t m_uNumContainedBases;								// 04 Number of nested classes following in the Base Class Array
			PMD m_pmd;													// 08 Pointer-to-member displacement info
			uint32_t m_uAttributes;										// 14 Flags
		};

		//"Class Hierarchy Descriptor" describes the inheritance hierarchy of a class; shared by all COLs for the class attributes flags
		constexpr uint32_t CHD_MULTINH = 0x01;							// Multiple inheritance (class C : public A, public B)
		constexpr uint32_t CHD_VIRTINH = 0x02;							// Virtual inheritance (class B : public virtual A)
		constexpr uint32_t CHD_AMBIGUOUS = 0x04;						// Ambiguous inheritance (class C : public A, public B) - but both A and B has function with same name or classes with diamond problem

		struct _RTTIBaseClassArray
		{
			_RTTIBaseClassDescriptor * m_pArrayOfBaseClassDescriptors[];
		};

		struct _RTTIClassHierarchyDescriptor
		{
			uint32_t m_uSignature;										// 00 Zero until loaded
			uint32_t m_uAttributes;										// 04 Flags
			uint32_t m_uNumBaseClasses;									// 08 Number of classes in the following 'm_uBaseClassArray'
			_RTTIBaseClassArray * m_pBaseClassArray;					// 0C _RTTIBaseClassArray *
		};

		//"Complete Object Locator" location of the complete object from a specific vftable pointer
		struct _RTTICompleteObjectLocator
		{
			uint32_t m_uSignature;										// 00 32bit zero, 64bit one, until loaded
			uint32_t m_uOffset;											// 04 Offset of this vftable in the complete class
			uint32_t m_cdOffset;										// 08 Constructor displacement offset
			_RTTITypeDescriptor * m_pTypeDescriptor;					// 0C (type info pointer) of the complete class
			_RTTIClassHierarchyDescriptor * m_pClassDescriptor;			// 10 (_RTTIClassHierarchyDescriptor *) Describes inheritance hierarchy
		};
#pragma pack(pop)
#elif defined __linux__ //experimental, might be wrong
#pragma pack(push)
#pragma pack(1)

		/*
			vtables in linux looks kinda different:
				`offset to complete this (0)`
				`rtti data`
				somefunc1
				somefunc2
				`offset to complete this (n)`
				`rtti data`
				somefunc3
				somefunc4
			https://i.imgur.com/cFuyEKF.png
			So to dyn cast this pointer to some interface you need check rtti data and find needed interface, after this do this + RTTI_GET_OFFSET(attrs)
			To cast to complete class you need simply do this (interface this) + (*this)[-2] and you are done
		*/

#define RTTI_GET_FLAGS(attributes) (attributes & 0xFF)
#define RTTI_GET_OFFSET(attributes) (attributes >> 8)
#define RTTI_IS_MULTICLASS(flags) (flags <= (MBCF_REPEAT_BASE | MBCF_AMBIGUOUS | MBCF_IDK_SUPERCOOL))

		//possible flags of _RTTIMultiBaseClass. All info got from ida so this might be incorrect
		constexpr uint32_t MBCF_REPEAT_BASE = 0x1;						// Non-diamin repeat mask
		constexpr uint32_t MBCF_DIAMOND_SHAPED = 0x2;					// Here we have diamond problem
		constexpr uint32_t MBCF_IDK_SUPERCOOL = 0x10;					// Idk what it means

		//possible flags of _RTTIMultiBaseClassesTypeInfo. All info again got from ida =|
		constexpr uint8_t MBCTIF_VIRTUAL = 0x1;							// Virtual inherit
		constexpr uint8_t MBCTIF_PUBLIC = 0x2;							// Public inherit
		constexpr uint8_t MBCTIF_IDK_SUPERCOOL = 0x8;					// Idk what it means

		struct STypeInfo
		{
			void * m_pCXXABIVTable;										// 00 Don't touch, this is actually cxxabi type_info vtable
			const char * m_pMangledClassName;							// 04 Mangled classname
		};

		typedef STypeInfo _RTTITypeDescriptor;

		struct _RTTIMultiBaseClassesTypeInfo
		{
			_RTTITypeDescriptor * m_pBaseClass;							// 00 Base class type info
			int m_iAttributes;											// 04 Contains offset and flags
		};

		union _RTTISharedBaseClass
		{
			struct _RTTISingleBaseClass : public _RTTITypeDescriptor	// Class without bases (si)
			{
				_RTTITypeDescriptor * m_pBaseClass;						// 08 Base class type info
			} m_SingleClass;

			struct _RTTIMultiBaseClass : public _RTTITypeDescriptor		// Multi base class (vmi)
			{
				uint32_t m_uFlags;										// 08 Some flags
				uint32_t m_uNumBases;									// 0C Count of base classes
				_RTTIMultiBaseClassesTypeInfo m_SBases[1];				// 10 Base classes array, array size == m_uNumBases
			} m_MultiClass;
		};

		struct _RTTIWholeInfo
		{
			int m_iCompleteOffset;										// 00 Offset to complete this, always negative
			_RTTISharedBaseClass * m_pSharedClass;						// 04 pointer to shared base class
		};
#pragma pack(pop)
#endif
	}

	class CVirtualMemberTableMan
	{
	public:
		struct SVFuncInfo
		{
			unsigned int m_iVTableOffset;
			unsigned int m_iFuncOffset;
			bool m_bIsValid;
		};

#define GetOriginalFunc(TReturn, TClass, TFuncName, TAttribs, ...) (static_cast<TReturn (TClass::*)(__VA_ARGS__) TAttribs>(&TClass::TFuncName))
#define GetOriginalFuncUnknown(TReturn, TClass, TFuncName, TAttribs) (reinterpret_cast<TReturn (TClass::*)() TAttribs>(&TClass::TFuncName)) //when we don't know count of arguments / varang func
#define GetVFuncInfo(pOriginalFunc) \
		NSUtils::CVirtualMemberTableMan::SVFuncInfo sInfo; \
		NSUtils::CVirtualMemberTableMan::GetFuncVInfo(pOriginalFunc, &sInfo);

	public:
		//Remember, classes may have several vtables and their offsets stored into rtti tables! Use RTTI functions below to get them before hooking
		CVirtualMemberTableMan(void * pBase, size_t iPresetSize = 0)
		{
			m_pThis = (void ***)pBase;
			if (iPresetSize)
				m_iSize = iPresetSize;
			else
			{
				void ** pVTable = *m_pThis;
				m_iSize = 0;
				while (pVTable[m_iSize] && IsValidFunction(pVTable[m_iSize])) m_iSize++;
			}
			if (!m_iSize)
			{
				m_bValidHook = false;
				return;
			}
			m_bValidHook = true;
		}

		~CVirtualMemberTableMan()
		{
			if (m_bValidHook && IsThisPointerValid())
			{
				//restore old vtable
				for (auto pFunc : m_mHookedFunctions)
					GetVTable()[pFunc.first] = pFunc.second;
				for (auto pChild : m_vChildTables) delete pChild;
			}
		}

		inline void DestroyNoRestore()
		{
			if (!m_bValidHook)
			{
				delete this;
				return;
			}
			//destroy this vtable
			m_bValidHook = false;
			for (auto pChild : m_vChildTables) pChild->DestroyNoRestore();
			delete this;
		}

		inline bool IsThisPointerValid() { return IsObjectPointerValid((void *)m_pThis); }
		inline void *** GetThis() { return m_pThis; }
		inline void ** GetVTable() { return *m_pThis; }
		inline bool IsVTableValid() { return m_bValidHook; }
		inline size_t GetVTableSize() { return m_iSize; }
		inline void AddChild(NSUtils::CVirtualMemberTableMan * pChild) { m_vChildTables.push_back(pChild); }
		inline NSUtils::CVirtualMemberTableMan * FindChild(void * pChildThis) { for (auto * pChild : m_vChildTables) if ((void *)pChild->GetThis() == pChildThis) return pChild; return nullptr; }

		template<typename TFunc = void *> inline TFunc GetOriginalFunction(const unsigned int iIndex)
		{
			if (!m_bValidHook) return (TFunc)nullptr;
			if (m_mHookedFunctions.find(iIndex) != m_mHookedFunctions.end())
				return reinterpret_cast<TFunc>(m_mHookedFunctions[iIndex]);
			return reinterpret_cast<TFunc>(GetVTable()[iIndex]);
		}

		inline void HookFunction(void * pFunc, const unsigned int iIndex)
		{
			if (!m_bValidHook)
				return;
			if (iIndex <= m_iSize && iIndex >= 0)
			{
				if (m_mHookedFunctions.find(iIndex) != m_mHookedFunctions.end())
					UnhookFunction(iIndex);
#ifdef _WIN32
				DWORD dwPrevProtection = 0;
				LI_FN(VirtualProtect)((LPVOID)&(GetVTable()[iIndex]), sizeof(void *), PAGE_EXECUTE_READWRITE, &dwPrevProtection);
#else //linux
				void * pAddr = (void *)ALIGN(&(GetVTable()[iIndex]));
				mprotect(pAddr, sysconf(_SC_PAGESIZE), PROT_READ | PROT_WRITE | PROT_EXEC);
#endif
				m_mHookedFunctions[iIndex] = GetVTable()[iIndex];
				GetVTable()[iIndex] = pFunc;
#ifdef _WIN32
				LI_FN(VirtualProtect)((LPVOID)&(GetVTable()[iIndex]), sizeof(void *), dwPrevProtection, (PDWORD)NULL);
#else //linux
				//we modified .rodata section, it was read only (ro), so restore old protection
				mprotect(pAddr, sysconf(_SC_PAGESIZE), PROT_READ);
#endif
			}
		}

		inline void UnhookFunction(const unsigned int iIndex)
		{
			if (!m_bValidHook)
				return;
			if (iIndex <= m_iSize && iIndex >= 0)
			{
				if (m_mHookedFunctions.find(iIndex) == m_mHookedFunctions.end())
					return;
#ifdef _WIN32
				DWORD dwPrevProtection = 0;
				LI_FN(VirtualProtect)((LPVOID)&(GetVTable()[iIndex]), sizeof(void *), PAGE_EXECUTE_READWRITE, &dwPrevProtection);
#else //linux
				//in linux we must align address before modify: http://man7.org/linux/man-pages/man2/mprotect.2.html
				void * pAddr = (void *)ALIGN(&(GetVTable()[iIndex]));
				mprotect(pAddr, sysconf(_SC_PAGESIZE), PROT_READ | PROT_WRITE | PROT_EXEC);
#endif
				GetVTable()[iIndex] = m_mHookedFunctions[iIndex];
				m_mHookedFunctions.erase(m_mHookedFunctions.find(iIndex));
#ifdef _WIN32
				LI_FN(VirtualProtect)((LPVOID)&(GetVTable()[iIndex]), sizeof(void *), dwPrevProtection, (PDWORD)NULL);
#else //linux
				//we modified .rodata section, it was read only (ro), so restore old protection
				mprotect(pAddr, sysconf(_SC_PAGESIZE), PROT_READ);
#endif
			}
		}

#define VMTGetOriginalFunctionAuto(pTable, TReturn, TClass, TFuncName, TAttribs, ...) [](NSUtils::CVirtualMemberTableMan * pTbl) -> void * \
		{ \
			GetVFuncInfo(GetOriginalFunc(TReturn, TClass, TFuncName, TAttribs, __VA_ARGS__)) \
			if (sInfo.m_bIsValid) \
			{ \
				if (sInfo.m_iVTableOffset) \
				{ \
					void * pNewThis = (void *)((char *)pTbl->GetThis() + sInfo.m_iVTableOffset); \
					NSUtils::CVirtualMemberTableMan * pNewTbl = nullptr; \
					if (!(pNewTbl = pTbl->FindChild(pNewThis))) \
					{ \
						pNewTbl = new NSUtils::CVirtualMemberTableMan(pNewThis); \
						pTbl->AddChild(pNewTbl); \
						return (void *)pNewTbl->GetOriginalFunction(sInfo.m_iFuncOffset); \
					} \
					else \
						return (void *)pNewTbl->GetOriginalFunction(sInfo.m_iFuncOffset); \
				} \
				else \
					return (void *)pTbl->GetOriginalFunction(sInfo.m_iFuncOffset); \
			} \
			return nullptr; \
		}(pTable)

#define VMTHookFunctionAuto(pTable, pCallback, TReturn, TClass, TFuncName, TAttribs, ...) [](NSUtils::CVirtualMemberTableMan * pTbl) -> void \
		{ \
			GetVFuncInfo(GetOriginalFunc(TReturn, TClass, TFuncName, TAttribs, __VA_ARGS__)) \
			if (sInfo.m_bIsValid) \
			{ \
				if (sInfo.m_iVTableOffset) \
				{ \
					void * pNewThis = (void *)((char *)pTbl->GetThis() + sInfo.m_iVTableOffset); \
					NSUtils::CVirtualMemberTableMan * pNewTbl = nullptr; \
					if (!(pNewTbl = pTbl->FindChild(pNewThis))) \
					{ \
						pNewTbl = new NSUtils::CVirtualMemberTableMan(pNewThis); \
						pTbl->AddChild(pNewTbl); \
						return pNewTbl->HookFunction(pCallback, sInfo.m_iFuncOffset); \
					} \
					else \
						return pNewTbl->HookFunction(pCallback, sInfo.m_iFuncOffset); \
				} \
				else \
					return pTbl->HookFunction(pCallback, sInfo.m_iFuncOffset); \
			} \
		}(pTable)

#define VMTUnhookFunctionAuto(pTable, TReturn, TClass, TFuncName, TAttribs, ...) [](NSUtils::CVirtualMemberTableMan * pTbl) -> void \
		{ \
			GetVFuncInfo(GetOriginalFunc(TReturn, TClass, TFuncName, TAttribs, __VA_ARGS__)) \
			if (sInfo.m_bIsValid) \
			{ \
				if (sInfo.m_iVTableOffset) \
				{ \
					void * pNewThis = (void *)((char *)pTbl->GetThis() + sInfo.m_iVTableOffset); \
					NSUtils::CVirtualMemberTableMan * pNewTbl = nullptr; \
					if (!(pNewTbl = pTbl->FindChild(pNewThis))) \
					{ \
						pNewTbl = new NSUtils::CVirtualMemberTableMan(pNewThis); \
						pTbl->AddChild(pNewTbl); \
						return pNewTbl->UnhookFunction(sInfo.m_iFuncOffset); \
					} \
					else \
						return pNewTbl->UnhookFunction(sInfo.m_iFuncOffset); \
				} \
				else \
					return pTbl->UnhookFunction(sInfo.m_iFuncOffset); \
			} \
		}(pTable)

		//when we don't know count of arguments / varang func. Might be unsafe!

#define VMTGetOriginalFunctionAutoUnknown(pTable, TReturn, TClass, TFuncName, TAttribs) [](NSUtils::CVirtualMemberTableMan * pTbl) -> void * \
		{ \
			GetVFuncInfo(GetOriginalFuncUnknown(TReturn, TClass, TFuncName, TAttribs)) \
			if (sInfo.m_bIsValid) \
			{ \
				if (sInfo.m_iVTableOffset) \
				{ \
					void * pNewThis = (void *)((char *)pTbl->GetThis() + sInfo.m_iVTableOffset); \
					NSUtils::CVirtualMemberTableMan * pNewTbl = nullptr; \
					if (!(pNewTbl = pTbl->FindChild(pNewThis))) \
					{ \
						pNewTbl = new NSUtils::CVirtualMemberTableMan(pNewThis); \
						pTbl->AddChild(pNewTbl); \
						return (void *)pNewTbl->GetOriginalFunction(sInfo.m_iFuncOffset); \
					} \
					else \
						return (void *)pNewTbl->GetOriginalFunction(sInfo.m_iFuncOffset); \
				} \
				else \
					return (void *)pTbl->GetOriginalFunction(sInfo.m_iFuncOffset); \
			} \
			return nullptr; \
		}(pTable)

#define VMTHookFunctionAutoUnknown(pTable, pCallback, TReturn, TClass, TFuncName, TAttribs) [](NSUtils::CVirtualMemberTableMan * pTbl) -> void \
		{ \
			GetVFuncInfo(GetOriginalFuncUnknown(TReturn, TClass, TFuncName, TAttribs)) \
			if (sInfo.m_bIsValid) \
			{ \
				if (sInfo.m_iVTableOffset) \
				{ \
					void * pNewThis = (void *)((char *)pTbl->GetThis() + sInfo.m_iVTableOffset); \
					NSUtils::CVirtualMemberTableMan * pNewTbl = nullptr; \
					if (!(pNewTbl = pTbl->FindChild(pNewThis))) \
					{ \
						pNewTbl = new NSUtils::CVirtualMemberTableMan(pNewThis); \
						pTbl->AddChild(pNewTbl); \
						return pNewTbl->HookFunction(pCallback, sInfo.m_iFuncOffset); \
					} \
					else \
						return pNewTbl->HookFunction(pCallback, sInfo.m_iFuncOffset); \
				} \
				else \
					return pTbl->HookFunction(pCallback, sInfo.m_iFuncOffset); \
			} \
		}(pTable)

#define VMTUnhookFunctionAutoUnknown(pTable, TReturn, TClass, TFuncName, TAttribs) [](NSUtils::CVirtualMemberTableMan * pTbl) -> void \
		{ \
			GetVFuncInfo(GetOriginalFuncUnknown(TReturn, TClass, TFuncName, TAttribs)) \
			if (sInfo.m_bIsValid) \
			{ \
				if (sInfo.m_iVTableOffset) \
				{ \
					void * pNewThis = (void *)((char *)pTbl->GetThis() + sInfo.m_iVTableOffset); \
					NSUtils::CVirtualMemberTableMan * pNewTbl = nullptr; \
					if (!(pNewTbl = pTbl->FindChild(pNewThis))) \
					{ \
						pNewTbl = new NSUtils::CVirtualMemberTableMan(pNewThis); \
						pTbl->AddChild(pNewTbl); \
						return pNewTbl->UnhookFunction(sInfo.m_iFuncOffset); \
					} \
					else \
						return pNewTbl->UnhookFunction(sInfo.m_iFuncOffset); \
				} \
				else \
					return pTbl->UnhookFunction(sInfo.m_iFuncOffset); \
			} \
		}(pTable)

		static inline const char * UndecorateClassName(const char * pDecorated)
		{
#ifdef _WIN32
			if (strncmp(pDecorated, xorstr_(".?AV"), 4) && strncmp(pDecorated, xorstr_(".?AU"), 4))
				return pDecorated;
			static char s_cOut[256];
			const char * pSym = &pDecorated[4];
			strcpy(s_cOut, pSym);
			char * pAt = strrchr(s_cOut, '@');
			if (pAt)
			{
				while (*pAt == '@') { *pAt = '\0'; pAt--; }
				if (strchr(s_cOut, '@'))
				{
					std::vector<std::string> sStrs; std::string sOut;
					boost::split(sStrs, s_cOut, boost::is_any_of(xorstr_("@")));
					for (int i = sStrs.size() - 1; i >= 0; i--)
					{
						sOut += sStrs[i];
						if (i) sOut += xorstr_("::");
					}
					strcpy(s_cOut, sOut.c_str());
				}
			}
			return s_cOut;
#else //linux
			//https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling
			//Not sure about this
			static char cFullDecorated[256];
			cFullDecorated[0] = 0;
			if (strncmp(pDecorated, xorstr_("_ZTS"), 4))
				strcpy(cFullDecorated, xorstr_("_ZTS"));
			strcat(cFullDecorated, pDecorated);
			int iStatus = 0;
			const char * pUndecoratedName = abi::__cxa_demangle(cFullDecorated, nullptr, 0, &iStatus); //FIXME: Potential memory leak! Should we free allocated memory? We need to check it.
			if (iStatus) //must be zero. -1 == fail to allocate memory, -2 == invalid decorated name
				return pDecorated;
			return pUndecoratedName;
#endif
		}

#ifdef _WIN32
		static inline NSRTTI::_RTTICompleteObjectLocator * GetRTTILocator(void ** pVTable)
		{
			NSRTTI::_RTTICompleteObjectLocator * pRTTILocator = nullptr;
			__try { pRTTILocator = (NSRTTI::_RTTICompleteObjectLocator *)pVTable[-1]; } //Pointer to _RTTICompleteObjectLocator
			__except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
			return pRTTILocator;
		}
#else //linux
		static inline NSRTTI::_RTTIWholeInfo * GetRTTIWholeInfo(void ** pVTable)
		{
			//check vtable for being valid
			InstallSegvCatcher();
			try { volatile void * pTemp = (volatile void *)pVTable[-1]; /*not a mistake*/ }
			catch (std::exception & e) { DeinstallSegvCatcher(); return nullptr; }
			DeinstallSegvCatcher();
			NSRTTI::_RTTIWholeInfo * pRTTIWholeInfo = (NSRTTI::_RTTIWholeInfo *)&pVTable[-2];
			return pRTTIWholeInfo;
		}
#endif
		
		static inline bool DoRTTICheck(void *** pThis, const char * pPotentialClassName)
		{
			if (!pThis) return false;
			void ** pVTable = *pThis;
			if (!pVTable) return false;
			const char * pClassName = nullptr;
#ifdef _WIN32
			NSRTTI::_RTTICompleteObjectLocator * pRTTILocator = GetRTTILocator(pVTable);
			if (!pRTTILocator) return false;
			__try
			{
				NSRTTI::_RTTIClassHierarchyDescriptor * pRTTIHierDesc = pRTTILocator->m_pClassDescriptor;
				NSRTTI::_RTTIBaseClassArray * pClassArr = pRTTIHierDesc->m_pBaseClassArray;
				NSRTTI::_RTTIBaseClassDescriptor * pBaseDescriptor = pClassArr->m_pArrayOfBaseClassDescriptors[0];
				NSRTTI::_RTTITypeDescriptor * pDescriptor = pBaseDescriptor->m_pTypeDescriptor;
				pClassName = &pDescriptor->_M_d_name[0];
			}
			__except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#else //linux
			NSRTTI::_RTTIWholeInfo * pRTTIWholeInfo = GetRTTIWholeInfo(pVTable);
			if (!pRTTIWholeInfo) return false;
			InstallSegvCatcher();
			try
			{
				NSRTTI::_RTTISharedBaseClass * pSharedClass = pRTTIWholeInfo->m_pSharedClass;
				pClassName = pSharedClass->m_pMangledClassName;
				volatile char ch = (volatile char)*pClassName; //check name for being valid
			}
			catch (std::exception & e) { DeinstallSegvCatcher(); return false; }
			DeinstallSegvCatcher();
#endif
			if (pClassName && *pClassName)
				return !strcmp(UndecorateClassName(pClassName), pPotentialClassName);
			return false;
		}

		//didn't tested yet
		static inline bool IsChildOf(void *** pThis, const char * pPotentialParent)
		{
			if (!pThis) return false;
			void ** pVTable = *pThis;
			if (!pVTable) return false;
#ifdef _WIN32
			NSRTTI::_RTTICompleteObjectLocator * pRTTILocator = GetRTTILocator(pVTable);
			if (!pRTTILocator) return false;
			NSRTTI::_RTTIBaseClassArray * pClassArr = nullptr;
			uint32_t iNumParents = 0;
			__try
			{
				NSRTTI::_RTTIClassHierarchyDescriptor * pRTTIHierDesc = pRTTILocator->m_pClassDescriptor;
				iNumParents = pRTTIHierDesc->m_uNumBaseClasses;
				if (iNumParents <= 1) return false; //there are no base classes of this class
				pClassArr = pRTTIHierDesc->m_pBaseClassArray;
			}
			__except (EXCEPTION_EXECUTE_HANDLER) { return false; }
			for (uint32_t iClass = 1 /*not a mistake!*/; iClass < iNumParents; iClass++)
			{
				__try
				{
					NSRTTI::_RTTIBaseClassDescriptor * pBaseDescriptor = pClassArr->m_pArrayOfBaseClassDescriptors[iClass];
					NSRTTI::_RTTITypeDescriptor * pDescriptor = pBaseDescriptor->m_pTypeDescriptor;
					const char * pClassName = &pDescriptor->_M_d_name[0];
					if (pClassName && *pClassName && !strcmp(UndecorateClassName(pClassName), pPotentialParent))
						return true;
				}
				__except (EXCEPTION_EXECUTE_HANDLER) { return false; }
			}
#else //linux
			NSRTTI::_RTTIWholeInfo * pRTTIWholeInfo = GetRTTIWholeInfo(pVTable);
			if (!pRTTIWholeInfo) return false;
			InstallSegvCatcher();
			try
			{
				if (RTTI_IS_MULTICLASS(pRTTIWholeInfo->m_pSharedClass->m_MultiClass.m_uFlags))
				{
					for (uint32_t iClass = 0; iClass < pRTTIWholeInfo->m_pSharedClass->m_MultiClass.m_uNumBases; iClass++)
					{
						NSRTTI::_RTTIMultiBaseClassesTypeInfo * pBaseClass = (((char *)&pRTTIWholeInfo->m_pSharedClass->m_MultiClass.m_SBases[0]) + sizeof(NSRTTI::_RTTIMultiBaseClassesTypeInfo) * iClass);
						const char * pClassName = pBaseClass->m_pBaseClass->m_pMangledClassName;
						if (pClassName && *pClassName && !strcmp(UndecorateClassName(pClassName), pPotentialParent))
						{
							DeinstallSegvCatcher();
							return true;
						}
					}
				}
			}
			catch (std::exception & e) { DeinstallSegvCatcher(); return false; }
			DeinstallSegvCatcher();
#endif
			return false;
		}
		
		//Note: This returns rtti class string of complete class both in linux and windows
		static inline const char * GetRTTIClass(void *** pThis)
		{
			if (!pThis) return "";
			void ** pVTable = *pThis;
			if (!pVTable) return "";
#ifdef _WIN32
			NSRTTI::_RTTICompleteObjectLocator * pRTTILocator = GetRTTILocator(pVTable);
			if (!pRTTILocator) return "";
			const char * pClassName = nullptr;
			__try
			{
				NSRTTI::_RTTIClassHierarchyDescriptor * pRTTIHierDesc = pRTTILocator->m_pClassDescriptor;
				NSRTTI::_RTTIBaseClassArray * pClassArr = pRTTIHierDesc->m_pBaseClassArray;
				NSRTTI::_RTTIBaseClassDescriptor * pBaseDescriptor = pClassArr->m_pArrayOfBaseClassDescriptors[0];
				NSRTTI::_RTTITypeDescriptor * pDescriptor = pBaseDescriptor->m_pTypeDescriptor;
				pClassName = &pDescriptor->_M_d_name[0];
			}
			__except (EXCEPTION_EXECUTE_HANDLER) { return ""; }
			if (pClassName && *pClassName)
				return UndecorateClassName(pClassName);
#else //linux
			NSRTTI::_RTTIWholeInfo * pRTTIWholeInfo = GetRTTIWholeInfo(pVTable);
			if (!pRTTIWholeInfo) return "";
			InstallSegvCatcher();
			try
			{
				NSRTTI::_RTTISharedBaseClass * pSharedClass = pRTTIWholeInfo->m_pSharedClass;
				const char * pClassName = pSharedClass->m_pMangledClassName;
				if (pClassName && *pClassName)
				{
					DeinstallSegvCatcher();
					return UndecorateClassName(pClassName);
				}
			}
			catch (std::exception & e) { DeinstallSegvCatcher(); return ""; }
			DeinstallSegvCatcher();
#endif
			return "";
		}

		//gets this pointer of complete class from one of its interfaces
		//Why it's might be needed?
		//Answer is simple, you can dyn cast to void * but can't from
		//These functions allow to cast from void * and provide proper pointers
		static inline void *** FindCompleteThisPointer(void *** pInterfaceThis)
		{
			if (!pInterfaceThis) return nullptr;
			void ** pVTable = *pInterfaceThis;
			if (!pVTable) return nullptr;
#ifdef _WIN32
			NSRTTI::_RTTICompleteObjectLocator * pRTTILocator = GetRTTILocator(pVTable);
			if (!pRTTILocator) return nullptr;
			if (pRTTILocator->m_uOffset == 0) return pInterfaceThis; //interface this is actually complete this
			void *** pCastedThis = nullptr;
			__try
			{
				pCastedThis = (void ***)((char *)pInterfaceThis - pRTTILocator->m_uOffset);
				if (pRTTILocator->m_cdOffset != 0)
				{
					int iAdditionalSub = *reinterpret_cast<int *>((char *)pInterfaceThis - pRTTILocator->m_cdOffset);
					pCastedThis = (void ***)((char *)pCastedThis - iAdditionalSub);
				}
			}
			__except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
			return pCastedThis;
#else //linux
			NSRTTI::_RTTIWholeInfo * pRTTIWholeInfo = GetRTTIWholeInfo(pVTable);
			if (!pRTTIWholeInfo || pRTTIWholeInfo->m_iCompleteOffset > 0) return nullptr; //invalid rtti data
			if (pRTTIWholeInfo->m_iCompleteOffset == 0) return pInterfaceThis; //interface this is actually complete this
			void *** pCastedThis = (void ***)((char *)pInterfaceThis + pRTTIWholeInfo->m_iCompleteOffset);
			return pCastedThis;
#endif
		}

		static inline void *** FindInterfaceThisPointer(void *** pCompleteThis, const char * pInterfaceName)
		{
			if (!pCompleteThis) return nullptr;
			if (!pInterfaceName || !*pInterfaceName) return nullptr;
			void ** pVTable = *pCompleteThis;
			if (!pVTable) return nullptr;
#ifdef _WIN32
			NSRTTI::_RTTICompleteObjectLocator * pRTTILocator = GetRTTILocator(pVTable);
			if (pRTTILocator->m_uOffset) //must be 0
			{
				pCompleteThis = FindCompleteThisPointer(pCompleteThis);
				if (!pCompleteThis) return nullptr;
				//do it again
				pVTable = *pCompleteThis;
				if (!pVTable) return nullptr;
				pRTTILocator = GetRTTILocator(pVTable);
			}
			NSRTTI::_RTTIBaseClassArray * pClassArr = nullptr;
			unsigned int iNumParents = 0;
			__try
			{
				NSRTTI::_RTTIClassHierarchyDescriptor * pRTTIHierDesc = pRTTILocator->m_pClassDescriptor;
				iNumParents = pRTTIHierDesc->m_uNumBaseClasses;
				if (iNumParents <= 1) return nullptr; //there are no base classes of this class
				pClassArr = pRTTIHierDesc->m_pBaseClassArray;
			}
			__except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
			for (uint32_t iClass = 1 /*not a mistake!*/; iClass < iNumParents; iClass++)
			{
				__try
				{
					NSRTTI::_RTTIBaseClassDescriptor * pBaseDescriptor = pClassArr->m_pArrayOfBaseClassDescriptors[iClass];
					NSRTTI::_RTTITypeDescriptor * pDescriptor = pBaseDescriptor->m_pTypeDescriptor;
					const char * pClassName = &pDescriptor->_M_d_name[0];
					if (pClassName && *pClassName && !strcmp(UndecorateClassName(pClassName), pInterfaceName))
					{
						void *** pCastedThis = (void ***)((char *)pCompleteThis + pBaseDescriptor->m_pmd.m_mdisp);
						if (pBaseDescriptor->m_pmd.m_pdisp != -1)
						{
							//Complete base is virtual. We need to calculate interface pointer correctly
							uint32_t uVtableAdditinalOffset = pBaseDescriptor->m_pmd.m_pdisp;
							pCastedThis = (void ***)((char *)pCastedThis + uVtableAdditinalOffset + pBaseDescriptor->m_pmd.m_vdisp);
						}
						return pCastedThis;
					}
				}
				__except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
			}
#else //linux
			NSRTTI::_RTTIWholeInfo * pRTTIWholeInfo = GetRTTIWholeInfo(pVTable);
			if (!pRTTIWholeInfo || pRTTIWholeInfo->m_iCompleteOffset > 0) return nullptr; //invalid rtti data
			if (pRTTIWholeInfo->m_iCompleteOffset) //must be 0
			{
				pCompleteThis = FindCompleteThisPointer(pCompleteThis);
				if (!pCompleteThis) return nullptr;
				//do it again
				pVTable = *pCompleteThis;
				if (!pVTable) return nullptr;
				pRTTIWholeInfo = GetRTTIWholeInfo(pVTable);
			}

			InstallSegvCatcher();
			try
			{
				if (RTTI_IS_MULTICLASS(pRTTIWholeInfo->m_pSharedClass->m_MultiClass.m_uFlags))
				{
					for (uint32_t iClass = 0; iClass < pRTTIWholeInfo->m_pSharedClass->m_MultiClass.m_uNumBases; iClass++)
					{
						NSRTTI::_RTTIMultiBaseClassesTypeInfo * pBaseClass = (((char *)&pRTTIWholeInfo->m_pSharedClass->m_MultiClass.m_SBases[0]) + sizeof(NSRTTI::_RTTIMultiBaseClassesTypeInfo) * iClass);
						const char * pClassName = pBaseClass->m_pBaseClass->m_pMangledClassName;
						if (pClassName && *pClassName && !strcmp(UndecorateClassName(pClassName), pPotentialParent))
						{
							int iOffset = RTTI_GET_OFFSET(pBaseClass->m_pBaseClass.m_iAttributes);
							void *** pCastedThis = (void ***)((char *)pCompleteThis + iOffset);
							DeinstallSegvCatcher();
							return pCastedThis;
						}
					}
				}
			}
			catch (std::exception & e) { DeinstallSegvCatcher(); return nullptr; }
			DeinstallSegvCatcher();
#endif
			return nullptr;
		}

		//if pCastTo == nullptr then casted this will be this of complete class
		static inline void *** DoDynamicCast(void *** pThis, const char * pCastTo = nullptr)
		{
			void ** pVTable = *pThis;
			if (!pVTable)
				return nullptr;
#ifdef _WIN32
			NSRTTI::_RTTICompleteObjectLocator * pRTTILocator = GetRTTILocator(pVTable);
			if (!pRTTILocator) return nullptr;
			if (pRTTILocator->m_uOffset == 0)
#else //linux
			NSRTTI::_RTTIWholeInfo * pRTTIWholeInfo = GetRTTIWholeInfo(pVTable);
			if (!pRTTIWholeInfo) return nullptr;
			if (pRTTIWholeInfo->m_iCompleteOffset == 0)
#endif
			{
				if (!pCastTo || !*pCastTo)
					return pThis;
				return FindInterfaceThisPointer(pThis, pCastTo);
			}
			else
			{
				if (pCastTo && *pCastTo)
				{
					void *** pCompleteThis = FindCompleteThisPointer(pThis);
					if (pCompleteThis)
					{
						if (DoRTTICheck(pCompleteThis, pCastTo))
							return pCompleteThis;
						else
							return FindInterfaceThisPointer(pCompleteThis, pCastTo);
					}
					else
						return nullptr; //must never happen!
				}
				else
					return FindCompleteThisPointer(pThis);
			}
		}

		inline bool DoRTTICheck(const char * pClassName) { if (!m_bValidHook) return false; return DoRTTICheck(m_pThis, pClassName); }
		inline bool IsChildOf(const char * pPotentialParent) { if (!m_bValidHook) return false; return IsChildOf(m_pThis, pPotentialParent); }
		inline const char * GetClassFromRTTI() { if (!m_bValidHook) return ""; return GetRTTIClass(m_pThis); }
		inline void *** FindCompleteThisPointer() { if (!m_bValidHook) return nullptr; return FindCompleteThisPointer(m_pThis); }
		inline void *** FindInterfaceThisPointer(const char * pInterfaceName) { if (!m_bValidHook) return nullptr; return FindInterfaceThisPointer(m_pThis, pInterfaceName); }
		inline void *** DoDynamicCast(const char * pCastTo = nullptr) { if (!m_bValidHook) return nullptr; return DoDynamicCast(m_pThis, pCastTo); }

		template<class TFunc> inline static void GetFuncVInfo(TFunc pFunc, SVFuncInfo * pInfo)
		{
			pInfo->m_bIsValid = false;

			if (!pFunc)
				return;

#ifdef _WIN32
			int iSize = sizeof(pFunc) / sizeof(void *);

			switch (iSize)
			{
				case 1:
				{
					pInfo->m_iVTableOffset = 0;
					pInfo->m_bIsValid = true;
					break;
				}
				case 2:
				{
					struct SMSVCFuncPointer2
					{
						void * m_pFunc;
						int m_iDelta;
					};

					pInfo->m_iVTableOffset = reinterpret_cast<SMSVCFuncPointer2 *>(&pFunc)->m_iDelta;
					pInfo->m_bIsValid = true;
					break;
				}
				case 3:
				{
					//must never happen
					pInfo->m_bIsValid = false;
					break;
				}
				case 4:
				{
					union UMSVCMemberFuncInfo
					{
						TFunc m_pFunction;

						struct
						{
							void * m_pFunc;
							int m_iThisDelta;
							int m_iVTableOffset;
							int m_iVTableIndex; //this value will be 0 if there no virtual inheritance
						}
						m_SInfo;
					}
					UInfo;

					UInfo.m_pFunction = pFunc;

					int iActualIndex = 0;

					if (UInfo.m_SInfo.m_iVTableIndex)
					{
						//fuck this, todo
						pInfo->m_bIsValid = false;
						pInfo->m_iVTableOffset = UInfo.m_SInfo.m_iVTableOffset;
					}
					else
					{
						pInfo->m_bIsValid = true;
						pInfo->m_iVTableOffset = UInfo.m_SInfo.m_iThisDelta;
					}
					break;
				}
			}

			if (pInfo->m_bIsValid)
			{
				//now find real vtable offset
				unsigned char * pCallAddr = (unsigned char *)*(void **)&pFunc;

				if (*pCallAddr == 0xE9) //there is a jump for non-virtual function thunk. Example: https://i.imgur.com/7626ks7.png
					pCallAddr += *(unsigned int *)(pCallAddr + 1) + 5; //get real address then

				//To understand this you must know assembly:
				//Let's say we have some function we want to call
				//if this is normal virtual function, then it uses __thiscall call convention
				//and it looks like this:
				//1008A3BB 8B 01              mov         eax, dword ptr [ecx] 
				//1008A3BD FF 60 0C           call        dword ptr [eax+0Ch]
				//or like this:
				//1013FDC4 8B 01              mov         eax, dword ptr [ecx] 
				//1013FDC6 FF A0 34 02 00 00  call        dword ptr [eax+234h]
				//so we need to look up for these cases and get vfunction offsets from them at runtime
				//UPD:
				//also varang function looks kinda different:
				//100020C2 8B 44 24 04        mov         eax, dword ptr [esp+4]
				//100020C6 8B 00              mov         eax, dword ptr [eax]
				//100020C8 FF 60 08           call        dword ptr [eax+8]

				bool bFound = false;

				if (pCallAddr[0] == 0x8B && pCallAddr[1] == 1)
				{
					bFound = true;
					pCallAddr += 2;
				}
				else if (pCallAddr[0] == 0x8B && pCallAddr[1] == 0x44 && pCallAddr[2] == 0x24 && pCallAddr[3] == 4 && pCallAddr[4] == 0x8B && pCallAddr[5] == 0)
				{
					bFound = true;
					pCallAddr += 6;
				}

				if (!bFound || pCallAddr[0] != 0xFF) //call opcode
				{
					pInfo->m_bIsValid = false;
					return;
				}

				pCallAddr++;

				if (pCallAddr[0] == 0xA0)
					pInfo->m_iFuncOffset = *((unsigned int *)++pCallAddr) / 4;
				else if (pCallAddr[0] == 0x60)
					pInfo->m_iFuncOffset = *((unsigned char *)++pCallAddr) / 4;
				else if (pCallAddr[0] == 0x20) //destructor
					pInfo->m_iFuncOffset = 0;
				else
					pInfo->m_bIsValid = false;
			}
#else //linux
			struct SABIFuncInfo
			{
				union
				{
					void * m_pFunc;
					int m_iVTableOffsetAdjusted;
				};

				int m_iDelta;
			};

			SABIFuncInfo * pFuncInfo = (SABIFuncInfo *)&pFunc;
			
			if (pFuncInfo->m_iVTableOffsetAdjusted & 1)
			{
				pInfo->m_bIsValid = true;
				pInfo->m_iFuncOffset = (pFuncInfo->m_iVTableOffsetAdjusted - 1) / sizeof(void *);
				pInfo->m_iVTableOffset = pFuncInfo->m_iDelta;
			}
#endif
		}

	private:
		static inline bool IsValidFunction(void * pFunc)
		{
#ifdef _WIN32
			MEMORY_BASIC_INFORMATION sInfo;
			LI_FN(VirtualQuery)(pFunc, &sInfo, sizeof(sInfo));
			return sInfo.Type && !(sInfo.Protect & (PAGE_GUARD | PAGE_NOACCESS)) && sInfo.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
#else //linux
			return true;
#endif
		}

		static inline bool IsObjectPointerValid(void * pObject) //only for polymorphic objects (objects with atleast 1 virtual function)
		{
#ifdef _WIN32
			//here we check if pointer to object is valid
			//ofc we can use MmCopyVirtualMemory, RtlCopyMemory && region size check, rtti veryfication, etc., but method below a lot simplier and faster
			//this is windows only!
			volatile void * pVtableFunc = nullptr;
			__try
			{
				pVtableFunc = **(volatile void ***)pObject;
				if (!pVtableFunc) return false;
			}
			__except (EXCEPTION_EXECUTE_HANDLER) { return false; } //Catch sigsegv: https://en.wikipedia.org/wiki/SIGSEGV
#else //linux
			InstallSegvCatcher();
			volatile void * pVtableFunc = nullptr;
			try
			{
				pVtableFunc = **(volatile void ***)pObject;
				if (!pVtableFunc) { DeinstallSegvCatcher(); return false; }
			}
			catch (std::exception & e) { DeinstallSegvCatcher(); return false; }
			DeinstallSegvCatcher();
#endif
			return !!*GetRTTIClass((void ***)pObject); //check rtti info
		}

#ifdef __linux__
		static inline void SigSegvCatcher()
		{
			if (ms_iInstallsCounter > 0)
				throw std::exception(xorstr_("Hello I'm sigsegv catch me tf"));
			else
				exit(SIGSEGV);
		}

		static inline void InstallSegvCatcher()
		{
			if (!ms_bSegvCatcherInstalled)
			{
				//install it once
				segvcatch::init_segv(&CVirtualMemberTableMan::SigSegvCatcher);
				ms_bSegvCatcherInstalled = true;
			}
			ms_iInstallsCounter++;
		}

		static inline void DeinstallSegvCatcher() { ms_iInstallsCounter--; if (ms_iInstallsCounter < 0) ms_iInstallsCounter = 0; }

	private:
		static bool ms_bSegvCatcherInstalled; //declared in global.cpp
		static int ms_iInstallsCounter;
#endif

	public:
		void *** m_pThis = nullptr;
		std::map<unsigned int, void *> m_mHookedFunctions;
		std::vector<CVirtualMemberTableMan *> m_vChildTables;
		size_t m_iSize = 0;
		bool m_bValidHook;
	};
};

#endif