#pragma once

struct LogList_t
{
	LogList_t(const LogType_t nLevel, const string& svText)
	{
		m_nLevel = nLevel;
		m_svText = svText;
	}

	LogType_t m_nLevel;
	string m_svText;
};

class CAdvancedSurface : public Forms::Form
{
public:
	CAdvancedSurface();
	virtual ~CAdvancedSurface()
	{
	};
	UIX::UIXListView* ConsoleListView() const { return m_ConsoleListView; };
	std::vector<LogList_t> m_LogList;

	void Init();
	void AddLog(const LogType_t type, const char* const pszText);

	eLaunchMode BuildParameter(string& svParameter);

private:
	void Setup();
	void LoadSettings();
	void SaveSettings();
	void ParseMaps();
	void ParsePlaylists();

	static void OnLoad(Forms::Control* pSender);
	static void OnClose(const std::unique_ptr<FormClosingEventArgs>& pEventArgs, Forms::Control* pSender);

	static void LaunchGame(Forms::Control* pSender);
	static void CleanSDK(Forms::Control* pSender);
	static void UpdateSDK(Forms::Control* pSender);

	static void ReloadMaplists(Forms::Control* pSender);
	static void ReloadPlaylists(Forms::Control* pSender);

	static void VirtualItemToClipboard(const std::unique_ptr<MouseEventArgs>& pEventArgs, Forms::Control* pSender);
	static void GetVirtualItem(const std::unique_ptr<Forms::RetrieveVirtualItemEventArgs>& pEventArgs, Forms::Control* pSender);
	static void ForwardCommandToGame(Forms::Control* pSender);

	uint64_t GetProcessorAffinity(string& szParameter);

	void AppendParameterInternal(string& svParameterList, const char* const szParameter, const char* const szArgument = nullptr);
	void AppendProcessorParameters(string& svParameter);
	void AppendConsoleParameters(string& svParameter);
	void AppendVideoParameters(string& svParameter);
	void AppendHostParameters(string& svParameter);
	void AppendNetParameters(string& svParameter);

	enum class eMode
	{
		NONE = -1,
		HOST,
		SERVER,
		CLIENT,
	};
	enum class eVisibility
	{
		PUBLIC,
		HIDDEN,
	};

	// Game.
	UIX::UIXGroupBox* m_GameGroup;
	UIX::UIXGroupBox* m_GameGroupExt;
	UIX::UIXLabel* m_MapLabel;
	UIX::UIXComboBox* m_MapCombo;
	UIX::UIXLabel* m_PlaylistLabel;
	UIX::UIXComboBox* m_PlaylistCombo;
	UIX::UIXCheckBox* m_CheatsToggle;
	UIX::UIXCheckBox* m_DeveloperToggle;
	UIX::UIXCheckBox* m_ConsoleToggle;
	UIX::UIXCheckBox* m_ColorConsoleToggle;
	UIX::UIXTextBox* m_PlaylistFileTextBox;
	UIX::UIXLabel* m_PlaylistFileLabel;

	// Main.
	UIX::UIXGroupBox* m_MainGroup;
	UIX::UIXGroupBox* m_MainGroupExt;
	UIX::UIXComboBox* m_ModeCombo;
	UIX::UIXLabel* m_ModeLabel;
	UIX::UIXTextBox* m_HostNameTextBox;
	UIX::UIXLabel* m_HostNameLabel;
	UIX::UIXComboBox* m_VisibilityCombo;
	UIX::UIXLabel* m_VisibilityLabel;
	UIX::UIXTextBox* m_LaunchArgsTextBox;
	UIX::UIXLabel* m_LaunchArgsLabel;
	UIX::UIXButton* m_CleanSDK;
	UIX::UIXButton* m_UpdateSDK;
	UIX::UIXButton* m_LaunchSDK;

	// Engine.
	UIX::UIXGroupBox* m_EngineBaseGroup;
	UIX::UIXGroupBox* m_EngineNetworkGroup;
	UIX::UIXGroupBox* m_EngineVideoGroup;
	UIX::UIXTextBox* m_ReservedCoresTextBox;
	UIX::UIXLabel* m_ReservedCoresLabel;
	UIX::UIXTextBox* m_WorkerThreadsTextBox;
	UIX::UIXLabel* m_WorkerThreadsLabel;
	UIX::UIXTextBox* m_ProcessorAffinityTextBox;
	UIX::UIXLabel* m_ProcessorAffinityLabel;
	UIX::UIXCheckBox* m_NoAsyncJobsToggle;
	UIX::UIXCheckBox* m_NetEncryptionToggle;
	UIX::UIXCheckBox* m_NetRandomKeyToggle;
	UIX::UIXCheckBox* m_QueuedPacketThread;
	UIX::UIXCheckBox* m_NoTimeOutToggle;
	UIX::UIXCheckBox* m_WindowedToggle;
	UIX::UIXCheckBox* m_NoBorderToggle;
	UIX::UIXTextBox* m_FpsTextBox;
	UIX::UIXLabel* m_FpsLabel;
	UIX::UIXTextBox* m_WidthTextBox;
	UIX::UIXTextBox* m_HeightTextBox;
	UIX::UIXLabel* m_ResolutionLabel;

	// Console.
	UIX::UIXGroupBox* m_ConsoleGroup;
	UIX::UIXGroupBox* m_ConsoleGroupExt;
	UIX::UIXListView* m_ConsoleListView;
	UIX::UIXTextBox* m_ConsoleCommandTextBox;
	UIX::UIXButton* m_ConsoleSendCommand;
};
