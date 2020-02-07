#ifndef __NETVARS_INC_
#define __NETVARS_INC_

#include <unordered_map>
#include <string.h>
#include <memory>
#include "interfaces.h"
#include "vmt.h"
#include "cdll_int.h"
#include "client_class.h"
#include "dt_common.h"
#include "dt_recv.h"
#include "dt_send.h"

//all of this creds to "Altimor"

namespace NSUtils
{
	class CEqualChar
	{
	public:
		inline bool operator()(const char * const& v1, const char * const& v2) const { return !strcmp(v1, v2); }
	};

	struct CHashChar
	{
	public:
		inline size_t operator()(const char * pObj) const
		{
			size_t szRes = 0;
			const size_t szPrime = 31;
			for (size_t i = 0; i < strlen(pObj); ++i)
				szRes = pObj[i] + (szRes * szPrime);
			return szRes;
		}
	};

	class CNetVarTree
	{
		struct SNode;
		using TMapType = std::unordered_map<const char *, std::shared_ptr<SNode>, CHashChar, CEqualChar>;

		struct SNode
		{
			inline SNode(int iOffset, RecvProp * pProp) : m_iOffset(iOffset), m_pRecvProp(pProp) {}

			TMapType m_mNodes;
			int m_iOffset;
			RecvProp * m_pRecvProp;
		};

		TMapType m_mNodes;

	public:

		inline void Init()
		{
			//const auto * pClientClass = NSInterfaces::g_pClient->GetAllClasses();
			const auto * pClientClass = CALL_VFUNC_OFFS(ClientClass * (__thiscall *)(IBaseClientDLL * pThis), NSInterfaces::g_pClient, 8)(NSInterfaces::g_pClient); //old offset 6, new 8
			while (pClientClass)
			{
				const auto pClassInfo = std::make_shared<SNode>(0, nullptr);
				auto * pRecvTable = pClientClass->m_pRecvTable;
				PopulateNodes(pRecvTable, &pClassInfo->m_mNodes);
				m_mNodes.emplace(pRecvTable->GetName(), pClassInfo);
				pClientClass = pClientClass->m_pNext;
			}
		}

	private:
		inline void PopulateNodes(RecvTable * pRecvTable, TMapType * pMap)
		{
			pMap->clear();
			for (auto i = 0; i < pRecvTable->GetNumProps(); i++)
			{
				const auto * pProp = pRecvTable->GetProp(i);
				const auto pPropInfo = std::make_shared<SNode>(pProp->GetOffset(), const_cast<RecvProp *>(pProp));
				if (pProp->GetType() == DPT_DataTable)
					PopulateNodes(pProp->GetDataTable(), &pPropInfo->m_mNodes);
				pMap->emplace(pProp->GetName(), pPropInfo);
			}
		}

		inline int GetOffsetRecursive(TMapType & rMap, int iAccumulated, const char * pName) { if (!rMap.count(pName)) return 0; return iAccumulated + rMap[pName]->m_iOffset; }
		template <typename... vaArgs> inline int GetOffsetRecursive(TMapType & rMap, int iAccumulated, const char * pName, vaArgs... args)
		{
			if (!rMap.count(pName))
				return 0;
			const auto & rNode = rMap[pName];
			return GetOffsetRecursive(rNode->m_mNodes, iAccumulated + rNode->m_iOffset, args...);
		}

		inline RecvProp * GetPropRecursive(TMapType & rMap, const char * pName) { return rMap[pName]->m_pRecvProp; }
		template <typename... vaArgs> inline RecvProp * GetPropRecursive(TMapType & rMap, const char * pName, vaArgs... args)
		{
			const auto & rNode = rMap[pName];
			return GetPropRecursive(rNode->m_mNodes, args...);
		}

	public:

		template <typename... vaArgs> inline int GetOffset(const char * pName, vaArgs... args)
		{
			const auto & rNode = m_mNodes[pName];
			if (rNode == 0)
				return 0;
			int iOffset = GetOffsetRecursive(rNode->m_mNodes, rNode->m_iOffset, args...);
			return iOffset;
		}

		template <typename... vaArgs> inline RecvProp * GetNetProp(const char * pName, vaArgs... args)
		{
			const auto & rNode = m_mNodes[pName];
			return GetPropRecursive(rNode->m_mNodes, args...);
		}
	};

	//defined in globals.cpp
	extern CNetVarTree g_CNetvars;
};

#define NETVARS_GET_OFFSET(name, ...) NSUtils::g_CNetvars.GetOffset(name, __VA_ARGS__)
#define NETVARS_GET_RECVPROP(name, ...) NSUtils::g_CNetvars.GetNetProp(name, __VA_ARGS__)

#endif