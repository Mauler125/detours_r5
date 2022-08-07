//=====================================================================================//
//
// Purpose: Implementation of the CBanSystem class.
//
// $NoKeywords: $
//=====================================================================================//

#include "core/stdafx.h"
#include "engine/net.h"
#include "engine/client/client.h"
#include "public/include/bansystem.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBanSystem::CBanSystem(void)
{
	Load();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pair - 
//-----------------------------------------------------------------------------
void CBanSystem::operator[](std::pair<string, uint64_t> pair)
{
	AddEntry(pair.first, pair.second);
}

//-----------------------------------------------------------------------------
// Purpose: loads and parses the banlist
//-----------------------------------------------------------------------------
void CBanSystem::Load(void)
{
	fs::path path = std::filesystem::current_path() /= "platform\\banlist.json";

	nlohmann::json jsIn;
	ifstream banFile(path, std::ios::in);

	int nTotalBans = 0;

	if (banFile.good() && banFile)
	{
		banFile >> jsIn; // Into json.
		banFile.close();

		if (!jsIn.is_null())
		{
			if (!jsIn["totalBans"].is_null())
			{
				nTotalBans = jsIn["totalBans"].get<int>();
			}
		}

		for (int i = 0; i < nTotalBans; i++)
		{
			nlohmann::json jsEntry = jsIn[std::to_string(i).c_str()];
			if (jsEntry.is_null())
			{
				continue;
			}

			uint64_t nOriginID = jsEntry["originID"].get<uint64_t>();
			string svIpAddress = jsEntry["ipAddress"].get<string>();

			m_vBanList.push_back(std::make_pair(svIpAddress, nOriginID));
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: saves the banlist
//-----------------------------------------------------------------------------
void CBanSystem::Save(void) const
{
	nlohmann::json jsOut;

	for (int i = 0; i < m_vBanList.size(); i++)
	{
		jsOut["totalBans"] = m_vBanList.size();
		jsOut[std::to_string(i).c_str()]["ipAddress"] = m_vBanList[i].first;
		jsOut[std::to_string(i).c_str()]["originID"] = m_vBanList[i].second;
	}

	fs::path path = std::filesystem::current_path() /= "platform\\banlist.json";
	ofstream outFile(path, std::ios::out | std::ios::trunc); // Write config file..

	outFile << jsOut.dump(4);
}

//-----------------------------------------------------------------------------
// Purpose: adds a banned player entry to the banlist
// Input  : svIpAddress - 
//			nOriginID - 
//-----------------------------------------------------------------------------
void CBanSystem::AddEntry(string svIpAddress, uint64_t nOriginID)
{
	if (!svIpAddress.empty())
	{
		auto it = std::find(m_vBanList.begin(), m_vBanList.end(), std::make_pair(svIpAddress, nOriginID));
		if (it == m_vBanList.end())
		{
			m_vBanList.push_back(std::make_pair(svIpAddress, nOriginID));
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: deletes an entry in the banlist
// Input  : svIpAddress - 
//			nOriginID - 
//-----------------------------------------------------------------------------
void CBanSystem::DeleteEntry(string svIpAddress, uint64_t nOriginID)
{
	for (size_t i = 0; i < m_vBanList.size(); i++)
	{
		if (svIpAddress.compare(m_vBanList[i].first) == NULL || nOriginID == m_vBanList[i].second)
		{
			m_vBanList.erase(m_vBanList.begin() + i);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: adds a connect refuse entry to the refuselist
// Input  : svError - 
//			nOriginID - 
//-----------------------------------------------------------------------------
void CBanSystem::AddConnectionRefuse(string svError, uint64_t nOriginID)
{
	if (m_vRefuseList.empty())
	{
		m_vRefuseList.push_back(std::make_pair(svError, nOriginID));
	}
	else
	{
		for (size_t i = 0; i < m_vRefuseList.size(); i++)
		{
			if (m_vRefuseList[i].second != nOriginID)
			{
				m_vRefuseList.push_back(std::make_pair(svError, nOriginID));
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: deletes an entry in the refuselist
// Input  : nOriginID - 
//-----------------------------------------------------------------------------
void CBanSystem::DeleteConnectionRefuse(uint64_t nOriginID)
{
	for (size_t i = 0; i < m_vRefuseList.size(); i++)
	{
		if (m_vRefuseList[i].second == nOriginID)
		{
			m_vRefuseList.erase(m_vRefuseList.begin() + i);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check refuse list and kill netchan connection.
//-----------------------------------------------------------------------------
void CBanSystem::BanListCheck(void)
{
	if (IsRefuseListValid())
	{
		for (size_t i = 0; i < m_vRefuseList.size(); i++)
		{
			for (int c = 0; c < MAX_PLAYERS; c++) // Loop through all possible client instances.
			{
				CClient* pClient = g_pClient->GetClient(i);
				if (!pClient)
					continue;

				CNetChan* pNetChan = pClient->GetNetChan();
				if (!pNetChan)
					continue;

				if (pClient->GetOriginID() != m_vRefuseList[i].second)
					continue;

				string svIpAddress = pNetChan->GetAddress();

				Warning(eDLL_T::SERVER, "Connection rejected for '%s' ('%llu' is banned from this server!)\n", svIpAddress.c_str(), pClient->GetOriginID());
				AddEntry(svIpAddress, pClient->GetOriginID());
				Save(); // Save banlist to file.
				NET_DisconnectClient(pClient, c, m_vRefuseList[i].first.c_str(), false, true);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: checks if specified ip address or necleus id is banned
// Input  : svIpAddress - 
//			nOriginID - 
// Output : true if banned, false if not banned
//-----------------------------------------------------------------------------
bool CBanSystem::IsBanned(string svIpAddress, uint64_t nOriginID) const
{
	for (size_t i = 0; i < m_vBanList.size(); i++)
	{
		string ipAddress = m_vBanList[i].first;
		uint64_t originID = m_vBanList[i].second;

		if (ipAddress.empty() ||
			!originID) // Cannot be null.
		{
			continue;
		}

		if (ipAddress.compare(svIpAddress) == NULL ||
			nOriginID == originID)
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: checks if refuselist is valid
//-----------------------------------------------------------------------------
bool CBanSystem::IsRefuseListValid(void) const
{
	return !m_vRefuseList.empty();
}

//-----------------------------------------------------------------------------
// Purpose: checks if banlist is valid
//-----------------------------------------------------------------------------
bool CBanSystem::IsBanListValid(void) const
{
	return !m_vBanList.empty();
}
///////////////////////////////////////////////////////////////////////////////
CBanSystem* g_pBanSystem = new CBanSystem();
