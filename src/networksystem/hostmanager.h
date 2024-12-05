#ifndef HOSTMANAGER_H
#define HOSTMANAGER_H
#include <networksystem/serverlisting.h>

enum HostStatus_e
{
	NOT_HOSTING,
	HOSTING
};

enum ServerVisibility_e
{
	OFFLINE,
	HIDDEN,
	PUBLIC
};

class CServerHostManager
{
public:
	CServerHostManager();

	void LaunchServer(const char* const map, const char* const mode) const;
	void ChangeLevel(const char* const map, const char* const mode) const;

	inline HostStatus_e GetHostStatus(void) const { return m_HostingStatus; }
	inline void SetHostStatus(const HostStatus_e hostStatus) { m_HostingStatus = hostStatus; }

	inline ServerVisibility_e GetVisibility(void) const { return m_ServerVisibility; }
	inline void SetVisibility(const ServerVisibility_e visibility) { m_ServerVisibility = visibility; }

	inline void SetCurrentToken(const string& token) { m_Token = token; }
	inline const string& GetCurrentToken() const { return m_Token; }

	inline void SetCurrentError(const string& error) { m_ErrorMsg = error; }
	inline const string& GetCurrentError() const { return m_ErrorMsg; }

	inline void SetHostIP(const string& ip) { m_HostIP = ip; };
	inline const string& GetHostIP() const { return m_HostIP; };

private:
	HostStatus_e m_HostingStatus;
	ServerVisibility_e m_ServerVisibility;

	string m_Token;
	string m_ErrorMsg;
	string m_HostIP;
};

extern CServerHostManager g_ServerHostManager;

#endif // HOSTMANAGER_H
