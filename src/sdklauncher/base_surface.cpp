//=============================================================================//
//
// Purpose: Launcher user interface implementation.
//
//=============================================================================//
#include "base_surface.h"
#include "sdklauncher.h"
#include "advanced_surface.h"
#include "download_surface.h"
#include "sdklauncher_utils.h"
#include "tier1/xorstr.h"
#include "tier1/utlmap.h"
#include "tier2/curlutils.h"
#include "zip/src/ZipFile.h"
#include "tier2/fileutils.h"

#define WINDOW_SIZE_X 400
#define WINDOW_SIZE_Y 224

CBaseSurface::CBaseSurface()
{
	this->SuspendLayout();
	this->SetAutoScaleDimensions({ 6, 13 });
	this->SetAutoScaleMode(Forms::AutoScaleMode::Font);
	this->SetText(XorStr("R5Reloaded"));
	this->SetClientSize({ WINDOW_SIZE_X, WINDOW_SIZE_Y });
	this->SetFormBorderStyle(Forms::FormBorderStyle::FixedSingle);
	this->SetStartPosition(Forms::FormStartPosition::CenterScreen);
	this->SetMinimizeBox(true);
	this->SetMaximizeBox(false);
	this->SetBackColor(Drawing::Color(47, 54, 61));

	this->Load += &OnLoad;
	this->FormClosing += &OnClose;

	this->m_BaseGroup = new UIX::UIXGroupBox();
	this->m_ManageButton = new UIX::UIXButton();
	this->m_RepairButton = new UIX::UIXButton();
	this->m_DonateButton = new UIX::UIXButton();
	this->m_JoinButton = new UIX::UIXButton();
	this->m_AdvancedButton = new UIX::UIXButton();

	const INT BASE_GROUP_OFFSET = 12;

	this->m_BaseGroup = new UIX::UIXGroupBox();
	this->m_BaseGroup->SetSize({ WINDOW_SIZE_X - (BASE_GROUP_OFFSET * 2), WINDOW_SIZE_Y - (BASE_GROUP_OFFSET * 2) });
	this->m_BaseGroup->SetLocation({ BASE_GROUP_OFFSET, BASE_GROUP_OFFSET });
	this->m_BaseGroup->SetTabIndex(0);
	this->m_BaseGroup->SetText("");
	this->m_BaseGroup->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->m_BaseGroup);

	m_bIsInstalled = fs::exists("r5apex.exe");

	this->m_ManageButton = new UIX::UIXButton();
	this->m_ManageButton->SetSize({ 168, 70 });
	this->m_ManageButton->SetLocation({ 10, 10 });
	this->m_ManageButton->SetTabIndex(9);
	this->m_ManageButton->SetText(m_bIsInstalled ? XorStr("Launch Apex") : XorStr("Install Apex"));
	this->m_ManageButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->m_ManageButton->Click += m_bIsInstalled ? &OnLaunchClick : &OnInstallClick;
	m_BaseGroup->AddControl(this->m_ManageButton);

	this->m_RepairButton = new UIX::UIXButton();
	this->m_RepairButton->SetSize({ 168, 70 });
	this->m_RepairButton->SetLocation({ 10, 90 });
	this->m_RepairButton->SetTabIndex(9);
	this->m_RepairButton->SetEnabled(m_bIsInstalled);
	this->m_RepairButton->SetText(XorStr("Advanced Options"));
	this->m_RepairButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	// TODO: should hash every file against a downloaded manifest instead and
	// start repairing what mismatches.
	this->m_RepairButton->Click += &OnAdvancedClick;
	m_BaseGroup->AddControl(this->m_RepairButton);

	this->m_DonateButton = new UIX::UIXButton();
	this->m_DonateButton->SetSize({ 178, 43 });
	this->m_DonateButton->SetLocation({ 188, 10 });
	this->m_DonateButton->SetTabIndex(9);
	this->m_DonateButton->SetText(XorStr("Support Amos (The Creator)"));
	this->m_DonateButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->m_DonateButton->Click += &OnSupportClick;
	m_BaseGroup->AddControl(this->m_DonateButton);

	this->m_JoinButton = new UIX::UIXButton();
	this->m_JoinButton->SetSize({ 178, 43 });
	this->m_JoinButton->SetLocation({ 188, 63 });
	this->m_JoinButton->SetTabIndex(9);
	this->m_JoinButton->SetText(XorStr("Join our Discord"));
	this->m_JoinButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->m_JoinButton->Click += &OnJoinClick;
	m_BaseGroup->AddControl(this->m_JoinButton);

	this->m_AdvancedButton = new UIX::UIXButton();
	this->m_AdvancedButton->SetSize({ 178, 43 });
	this->m_AdvancedButton->SetLocation({ 188, 116 });
	this->m_AdvancedButton->SetTabIndex(9);
	this->m_AdvancedButton->SetText(XorStr("Follow on YouTube"));
	this->m_AdvancedButton->SetAnchor(Forms::AnchorStyles::Bottom | Forms::AnchorStyles::Left);
	this->m_AdvancedButton->Click += &OnYouTubeClick;
	m_BaseGroup->AddControl(this->m_AdvancedButton);

	m_ExperimentalBuildsCheckbox = new UIX::UIXCheckBox();
	this->m_ExperimentalBuildsCheckbox->SetSize({ 250, 18 });
	this->m_ExperimentalBuildsCheckbox->SetLocation({ 10, 170 });
	this->m_ExperimentalBuildsCheckbox->SetTabIndex(0);
	this->m_ExperimentalBuildsCheckbox->SetText(XorStr("Check for playtest/event updates"));
	this->m_ExperimentalBuildsCheckbox->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_ExperimentalBuildsCheckbox->Click += OnExperimentalBuildsClick;

	m_BaseGroup->AddControl(this->m_ExperimentalBuildsCheckbox);

	// TODO: Use a toggle item instead; remove this field.
	m_bPartialInstall = true;
	m_bUpdateViewToggled = false;
}

void CBaseSurface::ToggleUpdateView(bool bValue)
{
	// Game must be installed before this can be called!!
	Assert(m_bIsInstalled);

	// Don't switch it to the same view, else we install the callbacks
	// twice and thus call them twice.
	if (bValue == m_bUpdateViewToggled)
		return;

	m_bUpdateViewToggled = bValue;
	this->m_ManageButton->SetText(bValue ? XorStr("Update && Launch Apex") : XorStr("Launch Apex"));

	if (bValue)
	{
		this->m_ManageButton->Click -= &OnLaunchClick;
		this->m_ManageButton->Click += &OnUpdateClick;
	}
	else
	{
		this->m_ManageButton->Click -= &OnUpdateClick;
		this->m_ManageButton->Click += &OnLaunchClick;
	}
}

bool CBaseSurface::CheckForUpdate()
{
	Msg(eDLL_T::COMMON, "%s: runframe; interval=%f\n", __FUNCTION__, g_flUpdateCheckRate);
	const bool updateAvailable = SDKLauncher_CheckForUpdate(m_ExperimentalBuildsCheckbox->Checked());

	if (m_bIsInstalled)
	{
		if (m_bIsInstalled && updateAvailable)
		{
			Msg(eDLL_T::COMMON, "%s: found update; interval=%f\n", __FUNCTION__, g_flUpdateCheckRate);
			ToggleUpdateView(true);

			return true;
		}

		ToggleUpdateView(false);
	}

	return false;
}


void CBaseSurface::Frame()
{
	for (;;)
	{
		Msg(eDLL_T::COMMON, "%s: runframe; interval=%f\n", __FUNCTION__, g_flUpdateCheckRate);

		if (CheckForUpdate())
		{
			Msg(eDLL_T::COMMON, "%s: found update; interval=%f\n", __FUNCTION__, g_flUpdateCheckRate);
		}

		std::this_thread::sleep_for(IntervalToDuration(g_flUpdateCheckRate));
	}
}

//-----------------------------------------------------------------------------
// Purpose: load callback
// Input  : *pSender - 
//-----------------------------------------------------------------------------
void CBaseSurface::OnLoad(Forms::Control* pSender)
{
	CBaseSurface* pBaseSurface = (CBaseSurface*)pSender->FindForm();
	pBaseSurface->LoadSettings();

	Threading::Thread([pBaseSurface] {
		pBaseSurface->Frame();
	}).Start();
}

//-----------------------------------------------------------------------------
// Purpose: close callback
// Input  : *pSender - 
//-----------------------------------------------------------------------------
void CBaseSurface::OnClose(const std::unique_ptr<FormClosingEventArgs>& /*pEventArgs*/, Forms::Control* pSender)
{
	((CBaseSurface*)pSender->FindForm())->SaveSettings();
}

void CBaseSurface::LoadSettings()
{
	CUtlString settingsPath;
	settingsPath.Format("platform/" SDK_USER_CFG_PATH "%s", LAUNCHER_SETTING_FILE);

	const char* pSettingsPath = settingsPath.String();

	if (!FileSystem()->FileExists(pSettingsPath))
		return;

	KeyValues kv("LauncherSettings");

	if (!kv.LoadFromFile(FileSystem(), pSettingsPath, nullptr))
	{
		Warning(eDLL_T::COMMON,"%s: Failed to parse VDF file: '%s'\n", __FUNCTION__, pSettingsPath);
		return;
	}

	const int settingsVersion = kv.GetInt("version", -1);

	if (settingsVersion != SDK_LAUNCHER_VERSION)
		return;

	KeyValues* sv = kv.FindKey("vars");

	if (!sv)
	{
		Warning(eDLL_T::COMMON, "%s: VDF file '%s' lacks subkey: '%s'\n", __FUNCTION__, pSettingsPath, "vars");
		return; // No settings to apply
	}

	this->m_ExperimentalBuildsCheckbox->SetChecked(sv->GetBool("experimentalBuilds"));
}

//-----------------------------------------------------------------------------
// Purpose: gets the control item value
// Input  : *pControl - 
//-----------------------------------------------------------------------------
const char* GetControlValue(Forms::Control* pControl)
{
	switch (pControl->GetType())
	{
	case Forms::ControlTypes::CheckBox:
	case Forms::ControlTypes::RadioButton:
	{
		return reinterpret_cast<UIX::UIXCheckBox*>(pControl)->Checked() ? "1" : "0";
	}
	default:
	{
		return pControl->Text().ToCString();
	}
	}
}


void CBaseSurface::SaveSettings()
{
	CUtlString settingsPath;
	settingsPath.Format("platform/" SDK_USER_CFG_PATH "%s", LAUNCHER_SETTING_FILE);

	CUtlString settingsDir = settingsPath.DirName();

	const char* pSettingsPath = settingsPath.String();
	const char* pSettingsDir = settingsDir.String();

	FileSystem()->CreateDirHierarchy(pSettingsDir);

	if (!FileSystem()->IsDirectory(pSettingsDir))
	{
		Warning(eDLL_T::COMMON, "%s: Failed to create directory: '%s'\n", __FUNCTION__, pSettingsPath);
		return;
	}

	KeyValues kv("LauncherSettings");
	kv.SetInt("version", SDK_LAUNCHER_VERSION);

	KeyValues* sv = new KeyValues("vars");

	if (!sv)
	{
		Warning(eDLL_T::COMMON, "%s: Failed to allocate subkey: '%s'\n", __FUNCTION__, "vars");
		return; // No settings to apply
	}

	kv.AddSubKey(sv);

	sv->SetBool("experimentalBuilds", this->m_ExperimentalBuildsCheckbox->Checked());

	CUtlBuffer outBuf(ssize_t(0), 0, CUtlBuffer::TEXT_BUFFER);
	kv.RecursiveSaveToFile(outBuf, 0);

	if (!FileSystem()->WriteFile(pSettingsPath, "PLATFORM", outBuf))
	{
		Warning(eDLL_T::COMMON, "%s: Failed to create VDF file: '%s'\n", __FUNCTION__, pSettingsPath);
		return;
	}

}

void CBaseSurface::OnUpdateClick(Forms::Control* Sender)
{
	CBaseSurface* pSurf = (CBaseSurface*)Sender->FindForm();

	vector<HWND> vecHandles;
	EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&vecHandles));

	if (!vecHandles.empty())
	{
		Forms::MessageBox::Show("Close all game instances before updating the game!\n",
			"Warning", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Warning);

		return;
	}

	auto downloadSurface = std::make_unique<CProgressPanel>();
	CProgressPanel* pProgress = downloadSurface.get();

	pProgress->SetAutoClose(true);

	bool bInstallComplete = false;

	Threading::Thread([pProgress, &bInstallComplete] {

		if (!SDKLauncher_CreateDepotDirectories())
		{
			Warning(eDLL_T::COMMON, "Failed to create depot directories: Error code = %08x\n: %s\n", GetLastError());

			Forms::MessageBox::Show(Format("Failed to create depot directories: Error code = %08x\n", GetLastError()).c_str(),
				"Error", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Error);

			pProgress->Close();
			return;
		}

		CUtlVector<CUtlString> fileList;
		CUtlString errorMessage;

		if (!SDKLauncher_BeginInstall(false, false, false, fileList, &errorMessage, pProgress))
		{
			Warning(eDLL_T::COMMON, "Failed to install game: %s\n", errorMessage.String());

			Forms::MessageBox::Show(Format("Failed to install game: %s\n", errorMessage.String()).c_str(),
				"Error", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Error);

			pProgress->Close();
			return;
		}

		// Close on finish.
		bInstallComplete = true;
		pProgress->Close();
		}).Start();

	pProgress->ShowDialog();

	// Save settings and restart the launcher process from here through updater.exe!
	pSurf->SaveSettings();
	
	//Only restart and launch the game if the install completed
	if(bInstallComplete)
		SDKLauncher_Restart();
}

void CBaseSurface::OnInstallClick(Forms::Control* Sender)
{
	vector<HWND> vecHandles;
	EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&vecHandles));

	if (!vecHandles.empty())
	{
		Forms::MessageBox::Show("Close all game instances before installing the game!\n",
			"Warning", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Warning);

		return;
	}


	CBaseSurface* pSurf = (CBaseSurface*)Sender;
	const bool bPartial = pSurf->m_bPartialInstall;

	const int minRequiredSpace = bPartial ? MIN_REQUIRED_DISK_SPACE : MIN_REQUIRED_DISK_SPACE_OPT;
	int currentDiskSpace;

	if (!SDKLauncher_CheckDiskSpace(minRequiredSpace, &currentDiskSpace))
	{
		Forms::MessageBox::Show(Format("There is not enough space available on the disk to install R5Reloaded;"
			" you need at least %iGB, you currently have %iGB\n", minRequiredSpace, currentDiskSpace).c_str(),
			"Error", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Error);

		return;
	}

	auto downloadSurface = std::make_unique<CProgressPanel>();
	CProgressPanel* pProgress = downloadSurface.get();

	pProgress->SetAutoClose(true);

	bool bInstallCompleted = false;

	Threading::Thread([pProgress, &bInstallCompleted] {
		
		if (!SDKLauncher_CreateDepotDirectories())
		{
			Warning(eDLL_T::COMMON, "Failed to create intermediate directory: Error code = %08x\n: %s\n", GetLastError());

			Forms::MessageBox::Show(Format("Failed to create intermediate directory: Error code = %08x\n", GetLastError()).c_str(),
				"Error", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Error);

			pProgress->Close();
			return;
		}

		CUtlVector<CUtlString> fileList;
		CUtlString errorMessage;

		if (!SDKLauncher_BeginInstall(false, false, true, fileList, &errorMessage, pProgress))
		{
			Warning(eDLL_T::COMMON, "Failed to install game: %s\n", errorMessage.String());

			Forms::MessageBox::Show(Format("Failed to install game: %s\n", errorMessage.String()).c_str(),
				"Error", Forms::MessageBoxButtons::OK, Forms::MessageBoxIcon::Error);
			
			pProgress->Close();
			return;
		}

		bInstallCompleted = true;
		// Close on finish.
		pProgress->Close();
	}).Start();

	pProgress->ShowDialog();

	// Restart the launcher process from here through updater.exe!
	if(bInstallCompleted)
		SDKLauncher_Restart();
}


void CBaseSurface::OnLaunchClick(Forms::Control* Sender)
{
	// !TODO: parameter building and settings loading should be its own class!!
	if (SDKLauncher()->CreateLaunchContext(eLaunchMode::LM_CLIENT, 0, "", "startup_launcher.cfg"))
	{
		if(SDKLauncher()->LaunchProcess())
			SDKLauncher()->Shutdown();
	}
}

void CBaseSurface::OnAdvancedClick(Forms::Control* Sender)
{
	auto pAdvancedSurface = std::make_unique<CAdvancedSurface>();
	pAdvancedSurface->Init();
	pAdvancedSurface->ShowDialog((Forms::Form*)Sender->FindForm());
}

void CBaseSurface::OnExperimentalBuildsClick(Forms::Control* Sender)
{
	CBaseSurface* pBaseSurface = reinterpret_cast<CBaseSurface*>(Sender->FindForm());
	pBaseSurface->CheckForUpdate();
}

void CBaseSurface::OnSupportClick(Forms::Control* /*Sender*/)
{
	ShellExecute(0, 0, XorStr("https://ko-fi.com/amos0"), 0, 0, SW_SHOW);
}

void CBaseSurface::OnJoinClick(Forms::Control* /*Sender*/)
{
	ShellExecute(0, 0, XorStr("https://discord.com/invite/jqMkUdXrBr"), 0, 0, SW_SHOW);
}

void CBaseSurface::OnYouTubeClick(Forms::Control* /*Sender*/)
{
	ShellExecute(0, 0, XorStr("https://www.youtube.com/@AmosMods"), 0, 0, SW_SHOW);
}
