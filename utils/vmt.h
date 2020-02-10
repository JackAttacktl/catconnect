/*
	Virtual Member Table Functions Manager

	Author: bluecutty
	This is x86 windows only! (cuz tf2 binaries has x86 bit capacity)

	Todo:
	Virtual Member Table manager (easy) - Done
	Implement RTTI information wrapper (easy): - Done
		DoRTTICheck - Done
		IsChildOf - Done
		GetRTTIClass - Done
	Implement custom dynamic cast (easy): - Done
		Implement FindCompleteThisPointer function (we can get parents / childs and their pointers from RTTI) (easy) - Done
		Implement FindInterfaceThisPointer function (and custom dyncast) (easy) - Done
	Add support to hook member function by its name (ala HookFunction(ThisPointer, Class, FuncName, Callback)) (easy) - WIP, 50% done but can be used already
	Find function offset from its signature (easy)
	Add inline hooks (ala detours) support (easy)
	Add vectored EH hooks support (easy)
	Add VW hooks support (easy)
	Add import table hooks support (is this even needed?) (easy)
	Port it to linux (HARD I guess because I don't know how GCC / Clang linux compilers implement some things :C)
*/

#ifndef __VMT_INC
#define __VMT_INC

#ifdef _WIN32
#include <Windows.h>
#include "lazy.h"
#else //linux, todo
#include <cxxabi.h>
#endif
#include <stdint.h>
#include <map>
#include <vector>
#include <string>
#include "xorstr.h"
#include <boost/algorithm/string.hpp>

//disable stupid warnings
#pragma warning (disable : 4996)
#pragma warning (disable : 4200)
#pragma warning (disable : 4700)

#define CALL_VFUNC_OFFS(funcdef, vthis, offset) ((funcdef)((*(void ***)vthis)[offset]))
#define CALL_VFUNC_NAME(instance, name) (instance->*name)
#define VMT_NOATTRIBS
//#define GET_IFACE_PTR(var) __asm mov var, ecx

namespace NSUtils
{
	//RTTI functions and structs (x86 and Windows only cuz source engine libraries in tf2 are x86)
	//Here we can work with rtti information: check class parents, check class name, etc
	//If you need x64 one you can do it by yourself
	//In linux it works similar, but there it will be __class_type_info blah blah blah
	//If you are is LightCat, here Linux example of working with rtti information: https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/libsupc%2B%2B/dyncast.cc#L45-L95
	//More info about vtables and rtti: https://yurichev.com/mirrors/RE/Recon-2012-Skochinsky-Compiler-Internals.pdf (english version, if you need russian one, just google it)

	namespace NSRTTI
	{
#pragma pack(push, 1)
		struct STypeInfo
		{
			void * m_pVTable;											// STypeInfo class vftable
			void * _M_data;												// nullptr until loaded at runtime
			char _M_d_name[1];											// Mangled name (prefix: .?AV=classes, .?AU=structs)
		};

		constexpr uint32_t MIN_TYPE_INFO_SIZE = (offsetof(STypeInfo, _M_d_name) + sizeof(".?AVx"));
		typedef STypeInfo _TypeDescriptor;
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
			PMD  m_pmd;													// 08 Pointer-to-member displacement info
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
		NSUtils::CVirtualMemberTableMan::GetFuncVInfo(pOriginalFunc, &sInfo); \
		if (sInfo.m_bIsValid) \
		{ \
			if (sInfo.m_iVTableOffset) \
			{ \
				/*Not completed yet! So until I complete this m_iVTableOffset must be 0! \
				TODO: Resolve function offset in complete vtable! We can do it using rtti information */ \
				sInfo.m_bIsValid = false; \
			} \
		}

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
			delete this;
		}

		inline bool IsThisPointerValid() { return IsObjectPointerValid((void *)m_pThis); }
		inline void *** GetThis() { return m_pThis; }
		inline void ** GetVTable() { return *m_pThis; }
		inline bool IsVTableValid() { return m_bValidHook; }
		inline size_t GetVTableSize() { return m_iSize; }

		template<typename TFunc = void *> inline TFunc GetOriginalFunction(const unsigned int iIndex)
		{
			if (m_mHookedFunctions.find(iIndex) != m_mHookedFunctions.end())
				return reinterpret_cast<TFunc>(m_mHookedFunctions[iIndex]);
			return reinterpret_cast<TFunc>(GetVTable()[iIndex]);
		}

		inline void HookFunction(void * pFunc, const unsigned int iIndex)
		{
			if (iIndex <= m_iSize && iIndex >= 0)
			{
				if (m_mHookedFunctions.find(iIndex) != m_mHookedFunctions.end())
					UnhookFunction(iIndex);
				DWORD dwPrevProtection = 0;
				LI_FN(VirtualProtect)((LPVOID)&(GetVTable()[iIndex]), sizeof(void *), PAGE_EXECUTE_READWRITE, &dwPrevProtection);
				m_mHookedFunctions[iIndex] = GetVTable()[iIndex];
				GetVTable()[iIndex] = pFunc;
				LI_FN(VirtualProtect)((LPVOID)&(GetVTable()[iIndex]), sizeof(void *), dwPrevProtection, (PDWORD)NULL);
			}
		}

		inline void UnhookFunction(const unsigned int iIndex)
		{
			if (iIndex <= m_iSize && iIndex >= 0)
			{
				if (m_mHookedFunctions.find(iIndex) == m_mHookedFunctions.end())
					return;
				DWORD dwPrevProtection = 0;
				LI_FN(VirtualProtect)((LPVOID)&(GetVTable()[iIndex]), sizeof(void *), PAGE_EXECUTE_READWRITE, &dwPrevProtection);
				GetVTable()[iIndex] = m_mHookedFunctions[iIndex];
				m_mHookedFunctions.erase(m_mHookedFunctions.find(iIndex));
				LI_FN(VirtualProtect)((LPVOID)&(GetVTable()[iIndex]), sizeof(void *), dwPrevProtection, (PDWORD)NULL);
			}
		}

#define VMTGetOriginalFunctionAuto(pTable, TReturn, TClass, TFuncName, TAttribs, ...) [](NSUtils::CVirtualMemberTableMan * pTbl) -> void * \
		{ \
			GetVFuncInfo(GetOriginalFunc(TReturn, TClass, TFuncName, TAttribs, __VA_ARGS__)) \
			if (sInfo.m_bIsValid) \
				return (void *)pTbl->GetOriginalFunction(sInfo.m_iFuncOffset); \
			return nullptr; \
		}(pTable)

#define VMTHookFunctionAuto(pTable, pCallback, TReturn, TClass, TFuncName, TAttribs, ...) [](NSUtils::CVirtualMemberTableMan * pTbl) -> void \
		{ \
			GetVFuncInfo(GetOriginalFunc(TReturn, TClass, TFuncName, TAttribs, __VA_ARGS__)) \
			if (sInfo.m_bIsValid) \
				return pTbl->HookFunction(pCallback, sInfo.m_iFuncOffset); \
		}(pTable)

#define VMTUnhookFunctionAuto(pTable, TReturn, TClass, TFuncName, TAttribs, ...) [](NSUtils::CVirtualMemberTableMan * pTbl) -> void \
		{ \
			GetVFuncInfo(GetOriginalFunc(TReturn, TClass, TFuncName, TAttribs, __VA_ARGS__)) \
			if (sInfo.m_bIsValid) \
				return pTbl->UnhookFunction(sInfo.m_iFuncOffset); \
		}(pTable)

		//when we don't know count of arguments / varang func. Might be unsafe!

#define VMTGetOriginalFunctionAutoUnknown(pTable, TReturn, TClass, TFuncName, TAttribs) [](NSUtils::CVirtualMemberTableMan * pTbl) -> void * \
		{ \
			GetVFuncInfo(GetOriginalFuncUnknown(TReturn, TClass, TFuncName, TAttribs)) \
			if (sInfo.m_bIsValid) \
				return (void *)pTbl->GetOriginalFunction(sInfo.m_iFuncOffset); \
			return nullptr; \
		}(pTable)

#define VMTHookFunctionAutoUnknown(pTable, pCallback, TReturn, TClass, TFuncName, TAttribs) [](NSUtils::CVirtualMemberTableMan * pTbl) -> void \
		{ \
			GetVFuncInfo(GetOriginalFuncUnknown(TReturn, TClass, TFuncName, TAttribs)) \
			if (sInfo.m_bIsValid) \
				return pTbl->HookFunction(pCallback, sInfo.m_iFuncOffset); \
		}(pTable)

#define VMTUnhookFunctionAutoUnknown(pTable, TReturn, TClass, TFuncName, TAttribs) [](NSUtils::CVirtualMemberTableMan * pTbl) -> void \
		{ \
			GetVFuncInfo(GetOriginalFuncUnknown(TReturn, TClass, TFuncName, TAttribs)) \
			if (sInfo.m_bIsValid) \
				return pTbl->UnhookFunction(sInfo.m_iFuncOffset); \
		}(pTable)

		static inline const char * UndecorateClassName(const char * pDecorated)
		{
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
		}

		static inline NSRTTI::_RTTICompleteObjectLocator * GetRTTILocator(void ** pVTable)
		{
			NSRTTI::_RTTICompleteObjectLocator * pRTTILocator = nullptr;
			__try { pRTTILocator = (NSRTTI::_RTTICompleteObjectLocator *)pVTable[-1]; } //Pointer to _RTTICompleteObjectLocator
			__except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
			return pRTTILocator;
		}

		static inline bool DoRTTICheck(void *** pThis, const char * pPotentialClassName)
		{
			void ** pVTable = *pThis;
			if (!pVTable) return false;
			NSRTTI::_RTTICompleteObjectLocator * pRTTILocator = GetRTTILocator(pVTable);
			if (!pRTTILocator) return false;
			const char * pClassName = nullptr;
			__try
			{
				NSRTTI::_RTTIClassHierarchyDescriptor * pRTTIHierDesc = pRTTILocator->m_pClassDescriptor;
				NSRTTI::_RTTIBaseClassArray * pClassArr = pRTTIHierDesc->m_pBaseClassArray;
				NSRTTI::_RTTIBaseClassDescriptor * pBaseDescriptor = pClassArr->m_pArrayOfBaseClassDescriptors[0];
				NSRTTI::_RTTITypeDescriptor * pDescriptor = pBaseDescriptor->m_pTypeDescriptor;
				pClassName = &pDescriptor->_M_d_name[0];
			}
			__except (EXCEPTION_EXECUTE_HANDLER) { return false; }
			if (pClassName && *pClassName)
				return !strcmp(UndecorateClassName(pClassName), pPotentialClassName);
			return false;
		}

		//didn't tested yet
		static inline bool IsChildOf(void *** pThis, const char * pPotentialParent)
		{
			void ** pVTable = *pThis;
			if (!pVTable) return false;
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
			return false;
		}

		static inline const char * GetRTTIClass(void *** pThis)
		{
			void ** pVTable = *pThis;
			if (!pVTable) return "";
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
			return "";
		}

		//gets this pointer of complete class from one of its interfaces
		//Why it's might be needed?
		//Answer is simple, you can dyn cast to void * but can't from
		//These functions allow to cast from void * and provide proper pointers
		static inline void *** FindCompleteThisPointer(void *** pInterfaceThis)
		{
			void ** pVTable = *pInterfaceThis;
			if (!pVTable) return nullptr;
			NSRTTI::_RTTICompleteObjectLocator * pRTTILocator = GetRTTILocator(pVTable);
			if (!pRTTILocator) return nullptr;
			if (pRTTILocator->m_uOffset == 0) return pInterfaceThis; //interface this is actually complete this
			void *** pCastedThis = nullptr;
			__try
			{
				NSRTTI::_RTTIClassHierarchyDescriptor * pRTTIHierDesc = pRTTILocator->m_pClassDescriptor;
				for (uint32_t iClass = 1 /*not a mistake!*/; iClass < pRTTIHierDesc->m_uNumBaseClasses; iClass++)
				{
					NSRTTI::_RTTIBaseClassArray * pClassArr = pRTTIHierDesc->m_pBaseClassArray;
					NSRTTI::_RTTIBaseClassDescriptor * pBaseDescriptor = pClassArr->m_pArrayOfBaseClassDescriptors[iClass];
					if (pBaseDescriptor->m_pmd.m_mdisp == pRTTILocator->m_uOffset) //not sure about that in case where base is virtual (check for CHD_AMBIGUOUS|CHD_VIRTINH flags), tests needed
					{
						pCastedThis = (void ***)((char *)pInterfaceThis - pBaseDescriptor->m_pmd.m_mdisp);
						if (pBaseDescriptor->m_pmd.m_pdisp != -1)
						{
							uint32_t uVtableAdditinalOffset = pBaseDescriptor->m_pmd.m_pdisp;
							pCastedThis = (void ***)((char *)pCastedThis - (uVtableAdditinalOffset + pBaseDescriptor->m_pmd.m_vdisp));
						}
					}
				}
			}
			__except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
			return pCastedThis;
		}

		static inline void *** FindInterfaceThisPointer(void *** pCompleteThis, const char * pInterfaceName)
		{
			if (!pInterfaceName || !*pInterfaceName) return nullptr;
			void ** pVTable = *pCompleteThis;
			if (!pVTable) return nullptr;
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
			return nullptr;
		}

		//if pCastTo == nullptr then casted this will be this of complete class
		static inline void *** DoDynamicCast(void *** pThis, const char * pCastTo = nullptr)
		{
			void ** pVTable = *pThis;
			if (!pVTable)
				return nullptr;
			NSRTTI::_RTTICompleteObjectLocator * pRTTILocator = GetRTTILocator(pVTable);
			if (!pRTTILocator) return nullptr;
			if (pRTTILocator->m_uOffset == 0)
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
		}

	private:
		static inline bool IsValidFunction(void * pFunc)
		{
			MEMORY_BASIC_INFORMATION sInfo;
			LI_FN(VirtualQuery)(pFunc, &sInfo, sizeof(sInfo));
			return sInfo.Type && !(sInfo.Protect & (PAGE_GUARD | PAGE_NOACCESS)) && sInfo.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
		}

		static inline bool IsObjectPointerValid(void * pObject) //only for polymorphic objects (objects with atleast 1 virtual function)
		{
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
			//also if you want to catch sigsegv in linux, you need to use posix signals handlers.
			//you also can use this library: https://code.google.com/archive/p/segvcatch/ to catch it with c++ try {} catch(...) {} construction
			return !!*GetRTTIClass((void ***)pObject); //check rtti info
		}

	public:
		void *** m_pThis = nullptr;
		std::map<unsigned int, void *> m_mHookedFunctions;
		size_t m_iSize = 0;
		bool m_bValidHook;
	};
};

#endif