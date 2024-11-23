//=============================================================================//
//
// Purpose: Launcher user interface implementation.
//
//=============================================================================//
#include "advanced_surface.h"
#include "sdklauncher.h"
#include "mathlib/bits.h"
#include "vpklib/packedstore.h"
#include "vstdlib/keyvaluessystem.h"
#include "filesystem/filesystem_std.h"
#include "tier2/fileutils.h"

//-----------------------------------------------------------------------------
// Purpose: creates a font by name
//-----------------------------------------------------------------------------
HFONT CreateFontByName(const char* const name, const int size)
{
	return CreateFont(
		size,                   // Height of font
		0,                      // Width of font
		0,                      // Angle of escapement
		0,                      // Orientation angle
		FW_NORMAL,              // Font weight
		FALSE,                  // Italic
		FALSE,                  // Underline
		FALSE,                  // Strikeout
		DEFAULT_CHARSET,        // Character set identifier
		OUT_DEFAULT_PRECIS,     // Output precision
		CLIP_DEFAULT_PRECIS,    // Clipping precision
		DEFAULT_QUALITY,        // Output quality
		DEFAULT_PITCH | FF_DONTCARE, // Pitch and family
		TEXT(name)              // Font name
	);
}

//-----------------------------------------------------------------------------
// Purpose: creates the surface layout
//-----------------------------------------------------------------------------
void CAdvancedSurface::Init()
{
	// START DESIGNER CODE
	const INT WindowX = 800;
	const INT WindowY = 353;

	const HFONT font = (HFONT)CreateFontByName("Microsoft Sans Serif", -11);
	this->SetFont(new Drawing::Font(this->_Handle, font, true));

	this->SuspendLayout();
	this->SetAutoScaleDimensions({ 6, 13 });
	this->SetAutoScaleMode(Forms::AutoScaleMode::Font);
	this->SetText("Dashboard");
	this->SetClientSize({ WindowX, WindowY });
	this->SetFormBorderStyle(Forms::FormBorderStyle::FixedSingle);
	this->SetStartPosition(Forms::FormStartPosition::CenterScreen);
	this->SetMinimizeBox(true);
	this->SetMaximizeBox(false);
	this->SetBackColor(Drawing::Color(47, 54, 61));

	this->Load += &OnLoad;
	this->Closing += &OnClose;

	// ########################################################################
	//	GAME
	// ########################################################################
	this->m_GameGroup = new UIX::UIXGroupBox();
	this->m_GameGroup->SetSize({ 458, 84 });
	this->m_GameGroup->SetLocation({ 12, 12 });
	this->m_GameGroup->SetTabIndex(0);
	this->m_GameGroup->SetText("Game");
	this->m_GameGroup->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->m_GameGroup);

	this->m_GameGroupExt = new UIX::UIXGroupBox();
	this->m_GameGroupExt->SetSize({ 458, 55 });
	this->m_GameGroupExt->SetLocation({ 12, 95 });
	this->m_GameGroupExt->SetTabIndex(0);
	this->m_GameGroupExt->SetText("");
	this->m_GameGroupExt->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->m_GameGroupExt);

	this->m_MapLabel = new UIX::UIXLabel();
	this->m_MapLabel->SetSize({ 50, 25 });
	this->m_MapLabel->SetLocation({ 365, 28 });
	this->m_MapLabel->SetTabIndex(0);
	this->m_MapLabel->SetText("Map");
	this->m_MapLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_MapLabel->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->m_GameGroup->AddControl(this->m_MapLabel);

	this->m_MapCombo = new UIX::UIXComboBox();
	this->m_MapCombo->SetSize({ 347, 25 });
	this->m_MapCombo->SetLocation({ 15, 25 });
	this->m_MapCombo->SetTabIndex(0);
	this->m_MapCombo->SetSelectedIndex(0);
	this->m_MapCombo->DropDownOpened += ReloadMaplists;
	this->m_MapCombo->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_MapCombo->SetDropDownStyle(Forms::ComboBoxStyle::DropDownList);
	this->m_GameGroup->AddControl(this->m_MapCombo);

	this->m_PlaylistLabel = new UIX::UIXLabel();
	this->m_PlaylistLabel->SetSize({ 50, 25 });
	this->m_PlaylistLabel->SetLocation({ 365, 53 });
	this->m_PlaylistLabel->SetTabIndex(0);
	this->m_PlaylistLabel->SetText("Playlist");
	this->m_PlaylistLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_PlaylistLabel->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->m_GameGroup->AddControl(this->m_PlaylistLabel);

	this->m_PlaylistCombo = new UIX::UIXComboBox();
	this->m_PlaylistCombo->SetSize({ 347, 25 });
	this->m_PlaylistCombo->SetLocation({ 15, 50 });
	this->m_PlaylistCombo->SetTabIndex(0);
	this->m_PlaylistCombo->SetSelectedIndex(0);
	this->m_PlaylistCombo->DropDownOpened += ReloadPlaylists;
	this->m_PlaylistCombo->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_PlaylistCombo->SetDropDownStyle(Forms::ComboBoxStyle::DropDownList);
	this->m_GameGroup->AddControl(this->m_PlaylistCombo);

	this->m_CheatsToggle = new UIX::UIXCheckBox();
	this->m_CheatsToggle->SetSize({ 110, 18 });
	this->m_CheatsToggle->SetLocation({ 15, 7 });
	this->m_CheatsToggle->SetTabIndex(0);
	this->m_CheatsToggle->SetText("Enable cheats");
	this->m_CheatsToggle->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_GameGroupExt->AddControl(this->m_CheatsToggle);

	this->m_DeveloperToggle = new UIX::UIXCheckBox();
	this->m_DeveloperToggle->SetSize({ 110, 18 });
	this->m_DeveloperToggle->SetLocation({ 130, 7 });
	this->m_DeveloperToggle->SetTabIndex(0);
	this->m_DeveloperToggle->SetText("Enable developer");
	this->m_DeveloperToggle->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_GameGroupExt->AddControl(this->m_DeveloperToggle);

	this->m_ConsoleToggle = new UIX::UIXCheckBox();
	this->m_ConsoleToggle->SetSize({ 110, 18 });
	this->m_ConsoleToggle->SetLocation({ 290, 7 });
	this->m_ConsoleToggle->SetTabIndex(0);
#ifdef DEDI_LAUNCHER
	this->m_ConsoleToggle->SetChecked(true);
#else // For client builds, don't show the console by default
	this->m_ConsoleToggle->SetChecked(false);
#endif // DEDI_LAUNCHER
	this->m_ConsoleToggle->SetText("Show console");
	this->m_ConsoleToggle->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_GameGroupExt->AddControl(this->m_ConsoleToggle);

	this->m_ColorConsoleToggle = new UIX::UIXCheckBox();
	this->m_ColorConsoleToggle->SetSize({ 110, 18 });
	this->m_ColorConsoleToggle->SetLocation({ 15, 30 });
	this->m_ColorConsoleToggle->SetTabIndex(0);
	this->m_ColorConsoleToggle->SetChecked(true);
	this->m_ColorConsoleToggle->SetText("Color console");
	this->m_ColorConsoleToggle->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_GameGroupExt->AddControl(this->m_ColorConsoleToggle);

	this->m_PlaylistFileTextBox = new UIX::UIXTextBox();
	this->m_PlaylistFileTextBox->SetSize({ 178, 18 });
	this->m_PlaylistFileTextBox->SetLocation({ 130, 30 });
	this->m_PlaylistFileTextBox->SetTabIndex(0);
	this->m_PlaylistFileTextBox->SetText("playlists_r5_patch.txt");
	this->m_PlaylistFileTextBox->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_GameGroupExt->AddControl(this->m_PlaylistFileTextBox);

	this->m_PlaylistFileLabel = new UIX::UIXLabel();
	this->m_PlaylistFileLabel->SetSize({ 60, 18 });
	this->m_PlaylistFileLabel->SetLocation({ 311, 32 });
	this->m_PlaylistFileLabel->SetTabIndex(0);
	this->m_PlaylistFileLabel->SetText("Playlists file");
	this->m_PlaylistFileLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_GameGroupExt->AddControl(this->m_PlaylistFileLabel);

	// ########################################################################
	//	MAIN
	// ########################################################################
	this->m_MainGroup = new UIX::UIXGroupBox();
	this->m_MainGroup->SetSize({ 308, 84 });
	this->m_MainGroup->SetLocation({ 480, 12 });
	this->m_MainGroup->SetTabIndex(0);
	this->m_MainGroup->SetText("Main");
	this->m_MainGroup->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->m_MainGroup);

	this->m_MainGroupExt = new UIX::UIXGroupBox();
	this->m_MainGroupExt->SetSize({ 308, 55 });
	this->m_MainGroupExt->SetLocation({ 480, 95 });
	this->m_MainGroupExt->SetTabIndex(0);
	this->m_MainGroupExt->SetText("");
	this->m_MainGroupExt->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->m_MainGroupExt);

	this->m_ModeCombo = new UIX::UIXComboBox();
	this->m_ModeCombo->SetSize({ 82, 25 });
	this->m_ModeCombo->SetLocation({ 15, 25 });
	this->m_ModeCombo->SetTabIndex(0);
	this->m_ModeCombo->SetSelectedIndex(0);
	this->m_ModeCombo->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_ModeCombo->SetDropDownStyle(Forms::ComboBoxStyle::DropDownList);
	this->m_MainGroup->AddControl(this->m_ModeCombo);

	this->m_ModeLabel = new UIX::UIXLabel();
	this->m_ModeLabel->SetSize({ 50, 25 });
	this->m_ModeLabel->SetLocation({ 100, 28 });
	this->m_ModeLabel->SetTabIndex(0);
	this->m_ModeLabel->SetText("Mode");
	this->m_ModeLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_ModeLabel->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->m_MainGroup->AddControl(this->m_ModeLabel);

	this->m_HostNameTextBox = new UIX::UIXTextBox();
	this->m_HostNameTextBox->SetSize({ 80, 21 });
	this->m_HostNameTextBox->SetLocation({ 150, 25 });
	this->m_HostNameTextBox->SetTabIndex(0);
	this->m_HostNameTextBox->SetText("");
	this->m_HostNameTextBox->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_MainGroup->AddControl(this->m_HostNameTextBox);

	this->m_HostNameLabel = new UIX::UIXLabel();
	this->m_HostNameLabel->SetSize({ 70, 21 });
	this->m_HostNameLabel->SetLocation({ 233, 28 });
	this->m_HostNameLabel->SetTabIndex(0);
	this->m_HostNameLabel->SetText("Host name");
	this->m_HostNameLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_MainGroup->AddControl(this->m_HostNameLabel);

	this->m_VisibilityCombo = new UIX::UIXComboBox();
	this->m_VisibilityCombo->SetSize({ 82, 25 });
	this->m_VisibilityCombo->SetLocation({ 15, 50 });
	this->m_VisibilityCombo->SetTabIndex(0);
	this->m_VisibilityCombo->SetSelectedIndex(0);
	this->m_VisibilityCombo->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_VisibilityCombo->SetDropDownStyle(Forms::ComboBoxStyle::DropDownList);
	this->m_MainGroup->AddControl(this->m_VisibilityCombo);

	this->m_VisibilityLabel = new UIX::UIXLabel();
	this->m_VisibilityLabel->SetSize({ 70, 21 });
	this->m_VisibilityLabel->SetLocation({ 100, 53 });
	this->m_VisibilityLabel->SetTabIndex(0);
	this->m_VisibilityLabel->SetText("Visibility");
	this->m_VisibilityLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_MainGroup->AddControl(this->m_VisibilityLabel);

	this->m_LaunchArgsTextBox = new UIX::UIXTextBox();
	this->m_LaunchArgsTextBox->SetSize({ 80, 21 });
	this->m_LaunchArgsTextBox->SetLocation({ 150, 50 });
	this->m_LaunchArgsTextBox->SetTabIndex(0);
	this->m_LaunchArgsTextBox->SetText("");
	this->m_LaunchArgsTextBox->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_MainGroup->AddControl(this->m_LaunchArgsTextBox);

	this->m_LaunchArgsLabel = new UIX::UIXLabel();
	this->m_LaunchArgsLabel->SetSize({ 70, 21 });
	this->m_LaunchArgsLabel->SetLocation({ 233, 53 });
	this->m_LaunchArgsLabel->SetTabIndex(0);
	this->m_LaunchArgsLabel->SetText("Command line");
	this->m_LaunchArgsLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_MainGroup->AddControl(this->m_LaunchArgsLabel);

	this->m_CleanSDK = new UIX::UIXButton();
	this->m_CleanSDK->SetSize({ 111, 18 });
	this->m_CleanSDK->SetLocation({ 15, 7 });
	this->m_CleanSDK->SetTabIndex(0);
	this->m_CleanSDK->SetText("Clean SDK");
	this->m_CleanSDK->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_CleanSDK->Click += &CleanSDK;
	this->m_MainGroupExt->AddControl(this->m_CleanSDK);

	this->m_UpdateSDK = new UIX::UIXButton();
	this->m_UpdateSDK->SetSize({ 111, 18 });
	this->m_UpdateSDK->SetLocation({ 15, 30 });
	this->m_UpdateSDK->SetTabIndex(0);
	this->m_UpdateSDK->SetEnabled(true); // !TODO: Implement updater
	this->m_UpdateSDK->SetText("Update SDK");
	this->m_UpdateSDK->Click += &UpdateSDK;
	this->m_UpdateSDK->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_MainGroupExt->AddControl(this->m_UpdateSDK);

	this->m_LaunchSDK = new UIX::UIXButton();
	this->m_LaunchSDK->SetSize({ 170, 41 });
	this->m_LaunchSDK->SetLocation({ 131, 7 });
	this->m_LaunchSDK->SetTabIndex(0);
	this->m_LaunchSDK->SetText("Launch game");
	this->m_LaunchSDK->SetBackColor(Drawing::Color(3, 102, 214));
	this->m_LaunchSDK->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_LaunchSDK->Click += &LaunchGame;
	this->m_MainGroupExt->AddControl(this->m_LaunchSDK);

	// ########################################################################
	//	ENGINE
	// ########################################################################
	this->m_EngineBaseGroup = new UIX::UIXGroupBox();
	this->m_EngineBaseGroup->SetSize({ 337, 73 });
	this->m_EngineBaseGroup->SetLocation({ 12, 160 });
	this->m_EngineBaseGroup->SetTabIndex(0);
	this->m_EngineBaseGroup->SetText("Engine");
	this->m_EngineBaseGroup->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->m_EngineBaseGroup);

	this->m_EngineNetworkGroup = new UIX::UIXGroupBox();
	this->m_EngineNetworkGroup->SetSize({ 337, 55 });
	this->m_EngineNetworkGroup->SetLocation({ 12, 232 });
	this->m_EngineNetworkGroup->SetTabIndex(0);
	this->m_EngineNetworkGroup->SetText("");
	this->m_EngineNetworkGroup->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->m_EngineNetworkGroup);

	this->m_EngineVideoGroup = new UIX::UIXGroupBox();
	this->m_EngineVideoGroup->SetSize({ 337, 55 });
	this->m_EngineVideoGroup->SetLocation({ 12, 286 });
	this->m_EngineVideoGroup->SetTabIndex(0);
	this->m_EngineVideoGroup->SetText("");
	this->m_EngineVideoGroup->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->m_EngineVideoGroup);

	this->m_ReservedCoresTextBox = new UIX::UIXTextBox();
	this->m_ReservedCoresTextBox->SetSize({ 18, 18 });
	this->m_ReservedCoresTextBox->SetLocation({ 15, 25 });
	this->m_ReservedCoresTextBox->SetTabIndex(0);
	this->m_ReservedCoresTextBox->SetReadOnly(false);
	this->m_ReservedCoresTextBox->SetText("-1");
	this->m_ReservedCoresTextBox->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineBaseGroup->AddControl(this->m_ReservedCoresTextBox);

	this->m_ReservedCoresLabel = new UIX::UIXLabel();
	this->m_ReservedCoresLabel->SetSize({ 105, 18 });
	this->m_ReservedCoresLabel->SetLocation({ 36, 27 });
	this->m_ReservedCoresLabel->SetTabIndex(0);
	this->m_ReservedCoresLabel->SetText("Reserved cores");
	this->m_ReservedCoresLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_ReservedCoresLabel->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->m_EngineBaseGroup->AddControl(this->m_ReservedCoresLabel);

	this->m_WorkerThreadsTextBox = new UIX::UIXTextBox();
	this->m_WorkerThreadsTextBox->SetSize({ 18, 18 });
	this->m_WorkerThreadsTextBox->SetLocation({ 155, 25 });
	this->m_WorkerThreadsTextBox->SetTabIndex(0);
	this->m_WorkerThreadsTextBox->SetReadOnly(false);
	this->m_WorkerThreadsTextBox->SetText("-1");
	this->m_WorkerThreadsTextBox->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineBaseGroup->AddControl(this->m_WorkerThreadsTextBox);

	this->m_WorkerThreadsLabel = new UIX::UIXLabel();
	this->m_WorkerThreadsLabel->SetSize({ 105, 18 });
	this->m_WorkerThreadsLabel->SetLocation({ 176, 27 });
	this->m_WorkerThreadsLabel->SetTabIndex(0);
	this->m_WorkerThreadsLabel->SetText("Worker threads");
	this->m_WorkerThreadsLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_WorkerThreadsLabel->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->m_EngineBaseGroup->AddControl(this->m_WorkerThreadsLabel);

	this->m_ProcessorAffinityTextBox = new UIX::UIXTextBox();
	this->m_ProcessorAffinityTextBox->SetSize({ 18, 18 });
	this->m_ProcessorAffinityTextBox->SetLocation({ 15, 48 });
	this->m_ProcessorAffinityTextBox->SetTabIndex(0);
	this->m_ProcessorAffinityTextBox->SetReadOnly(false);
	this->m_ProcessorAffinityTextBox->SetText("0");
	this->m_ProcessorAffinityTextBox->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineBaseGroup->AddControl(this->m_ProcessorAffinityTextBox);

	this->m_ProcessorAffinityLabel = new UIX::UIXLabel();
	this->m_ProcessorAffinityLabel->SetSize({ 105, 18 });
	this->m_ProcessorAffinityLabel->SetLocation({ 36, 50 });
	this->m_ProcessorAffinityLabel->SetTabIndex(0);
	this->m_ProcessorAffinityLabel->SetText("Processor affinity");
	this->m_ProcessorAffinityLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_ProcessorAffinityLabel->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->m_EngineBaseGroup->AddControl(this->m_ProcessorAffinityLabel);

	this->m_NoAsyncJobsToggle = new UIX::UIXCheckBox();
	this->m_NoAsyncJobsToggle->SetSize({ 105, 18 });
	this->m_NoAsyncJobsToggle->SetLocation({ 155, 48 });
	this->m_NoAsyncJobsToggle->SetTabIndex(2);
	this->m_NoAsyncJobsToggle->SetText("No async");
	this->m_NoAsyncJobsToggle->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineBaseGroup->AddControl(this->m_NoAsyncJobsToggle);

	this->m_NetEncryptionToggle = new UIX::UIXCheckBox();
	this->m_NetEncryptionToggle->SetSize({ 105, 18 });
	this->m_NetEncryptionToggle->SetLocation({ 15, 7 });
	this->m_NetEncryptionToggle->SetTabIndex(0);
	this->m_NetEncryptionToggle->SetChecked(true);
	this->m_NetEncryptionToggle->SetText("Encrypt packets");
	this->m_NetEncryptionToggle->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineNetworkGroup->AddControl(this->m_NetEncryptionToggle);

	this->m_NetRandomKeyToggle = new UIX::UIXCheckBox();
	this->m_NetRandomKeyToggle->SetSize({ 105, 18 });
	this->m_NetRandomKeyToggle->SetLocation({ 155, 7 });
	this->m_NetRandomKeyToggle->SetTabIndex(0);
	this->m_NetRandomKeyToggle->SetChecked(true);
	this->m_NetRandomKeyToggle->SetText("Random netkey");
	this->m_NetRandomKeyToggle->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineNetworkGroup->AddControl(this->m_NetRandomKeyToggle);

	this->m_QueuedPacketThread = new UIX::UIXCheckBox();
	this->m_QueuedPacketThread->SetSize({ 105, 18 });
	this->m_QueuedPacketThread->SetLocation({ 15, 30 });
	this->m_QueuedPacketThread->SetTabIndex(2);
	this->m_QueuedPacketThread->SetChecked(true);
	this->m_QueuedPacketThread->SetText("Queued packets");
	this->m_QueuedPacketThread->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineNetworkGroup->AddControl(this->m_QueuedPacketThread);

	this->m_NoTimeOutToggle = new UIX::UIXCheckBox();
	this->m_NoTimeOutToggle->SetSize({ 105, 18 });
	this->m_NoTimeOutToggle->SetLocation({ 155, 30 });
	this->m_NoTimeOutToggle->SetTabIndex(0);
	this->m_NoTimeOutToggle->SetText("No timeout");
	this->m_NoTimeOutToggle->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineNetworkGroup->AddControl(this->m_NoTimeOutToggle);

	this->m_WindowedToggle = new UIX::UIXCheckBox();
	this->m_WindowedToggle->SetSize({ 105, 18 });
	this->m_WindowedToggle->SetLocation({ 15, 7 });
	this->m_WindowedToggle->SetTabIndex(0);
	this->m_WindowedToggle->SetText("Windowed");
	this->m_WindowedToggle->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineVideoGroup->AddControl(this->m_WindowedToggle);

	this->m_NoBorderToggle = new UIX::UIXCheckBox();
	this->m_NoBorderToggle->SetSize({ 105, 18 });
	this->m_NoBorderToggle->SetLocation({ 155, 7 });
	this->m_NoBorderToggle->SetTabIndex(0);
	this->m_NoBorderToggle->SetText("Borderless");
	this->m_NoBorderToggle->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineVideoGroup->AddControl(this->m_NoBorderToggle);

	this->m_FpsTextBox = new UIX::UIXTextBox();
	this->m_FpsTextBox->SetSize({ 25, 18 });
	this->m_FpsTextBox->SetLocation({ 15, 30 });
	this->m_FpsTextBox->SetTabIndex(0);
	this->m_FpsTextBox->SetReadOnly(false);
	this->m_FpsTextBox->SetText("-1");
	this->m_FpsTextBox->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineVideoGroup->AddControl(this->m_FpsTextBox);

	this->m_FpsLabel = new UIX::UIXLabel();
	this->m_FpsLabel->SetSize({ 105, 18 });
	this->m_FpsLabel->SetLocation({ 43, 32 });
	this->m_FpsLabel->SetTabIndex(0);
	this->m_FpsLabel->SetText("Max FPS");
	this->m_FpsLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_FpsLabel->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->m_EngineVideoGroup->AddControl(this->m_FpsLabel);

	this->m_WidthTextBox = new UIX::UIXTextBox();
	this->m_WidthTextBox->SetSize({ 50, 18 });
	this->m_WidthTextBox->SetLocation({ 106, 30 });
	this->m_WidthTextBox->SetTabIndex(0);
	this->m_WidthTextBox->SetReadOnly(false);
	this->m_WidthTextBox->SetText("");
	this->m_WidthTextBox->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineVideoGroup->AddControl(this->m_WidthTextBox);

	this->m_HeightTextBox = new UIX::UIXTextBox();
	this->m_HeightTextBox->SetSize({ 50, 18 });
	this->m_HeightTextBox->SetLocation({ 155, 30 });
	this->m_HeightTextBox->SetTabIndex(0);
	this->m_HeightTextBox->SetReadOnly(false);
	this->m_HeightTextBox->SetText("");
	this->m_HeightTextBox->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_EngineVideoGroup->AddControl(this->m_HeightTextBox);

	this->m_ResolutionLabel = new UIX::UIXLabel();
	this->m_ResolutionLabel->SetSize({ 125, 18 });
	this->m_ResolutionLabel->SetLocation({ 208, 32 });
	this->m_ResolutionLabel->SetTabIndex(0);
	this->m_ResolutionLabel->SetText("Resolution (width | height)");
	this->m_ResolutionLabel->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_ResolutionLabel->SetTextAlign(Drawing::ContentAlignment::TopLeft);
	this->m_EngineVideoGroup->AddControl(this->m_ResolutionLabel);

	// ########################################################################
	//	CONSOLE
	// ########################################################################
	this->m_ConsoleGroup = new UIX::UIXGroupBox();
	this->m_ConsoleGroup->SetSize({ 429, 15 });
	this->m_ConsoleGroup->SetLocation({ 359, 160 });
	this->m_ConsoleGroup->SetTabIndex(0);
	this->m_ConsoleGroup->SetText("Console");
	this->m_ConsoleGroup->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->m_ConsoleGroup);

	this->m_ConsoleGroupExt = new UIX::UIXGroupBox();
	this->m_ConsoleGroupExt->SetSize({ 429, 167 });
	this->m_ConsoleGroupExt->SetLocation({ 359, 174 });
	this->m_ConsoleGroupExt->SetTabIndex(0);
	this->m_ConsoleGroupExt->SetText("");
	this->m_ConsoleGroupExt->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->AddControl(this->m_ConsoleGroupExt);

	this->m_ConsoleListView = new UIX::UIXListView();
	this->m_ConsoleListView->SetSize({ 427, 172 });
	this->m_ConsoleListView->SetLocation({ 1, -23 }); // HACK: hide columns
	this->m_ConsoleListView->SetTabIndex(0);
	this->m_ConsoleListView->SetBackColor(Drawing::Color(29, 33, 37));
	this->m_ConsoleListView->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_ConsoleListView->SetView(Forms::View::Details);
	this->m_ConsoleListView->SetVirtualMode(true);
	this->m_ConsoleListView->SetFullRowSelect(true);
	this->m_ConsoleListView->Columns.Add({ "index", 40 });
	this->m_ConsoleListView->Columns.Add({ "buffer", 387 });
	this->m_ConsoleListView->MouseClick += &VirtualItemToClipboard;
	this->m_ConsoleListView->RetrieveVirtualItem += &GetVirtualItem;
	this->m_ConsoleGroupExt->AddControl(this->m_ConsoleListView);

	this->m_ConsoleCommandTextBox = new UIX::UIXTextBox();
	this->m_ConsoleCommandTextBox->SetSize({ 351, 18 });
	this->m_ConsoleCommandTextBox->SetLocation({ 0, 149 });
	this->m_ConsoleCommandTextBox->SetTabIndex(0);
	this->m_ConsoleCommandTextBox->SetReadOnly(false);
	this->m_ConsoleCommandTextBox->SetText("");
	this->m_ConsoleCommandTextBox->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_ConsoleGroupExt->AddControl(this->m_ConsoleCommandTextBox);

	this->m_ConsoleSendCommand = new UIX::UIXButton();
	this->m_ConsoleSendCommand->SetSize({ 79, 18 });
	this->m_ConsoleSendCommand->SetLocation({ 350, 149 });
	this->m_ConsoleSendCommand->SetTabIndex(0);
	this->m_ConsoleSendCommand->SetText("Send");
	this->m_ConsoleSendCommand->SetBackColor(Drawing::Color(3, 102, 214));
	this->m_ConsoleSendCommand->SetAnchor(Forms::AnchorStyles::Top | Forms::AnchorStyles::Left);
	this->m_ConsoleSendCommand->Click += &ForwardCommandToGame;
	this->m_ConsoleGroupExt->AddControl(this->m_ConsoleSendCommand);

	this->ResumeLayout(false);
	this->PerformLayout();

	// END DESIGNER CODE

	this->Setup();
}

//-----------------------------------------------------------------------------
// Purpose: post-init surface setup
//-----------------------------------------------------------------------------
void CAdvancedSurface::Setup()
{
	// Already parse these out since you can scroll select in a combo box
	// without uncollapsing it.
	this->ParseMaps();
	this->ParsePlaylists();

	this->m_ModeCombo->Items.Add("Host");
	this->m_ModeCombo->Items.Add("Server");
	this->m_ModeCombo->Items.Add("Client");

#ifdef DEDI_LAUNCHER
	this->m_ModeCombo->SetSelectedIndex(1);
#endif // DEDI_LAUNCHER

	this->m_VisibilityCombo->Items.Add("Public");
	this->m_VisibilityCombo->Items.Add("Hidden");
	this->m_VisibilityCombo->Items.Add("Offline");
}

//-----------------------------------------------------------------------------
// Purpose: load and apply settings
//-----------------------------------------------------------------------------
void CAdvancedSurface::LoadSettings()
{
	CUtlString settingsPath;
	settingsPath.Format("platform/" SDK_USER_CFG_PATH "%s", LAUNCHER_SETTING_FILE);

	const char* pSettingsPath = settingsPath.String();

	if (!FileSystem()->FileExists(pSettingsPath))
		return;

	KeyValues kv("LauncherSettings");

	if (!kv.LoadFromFile(FileSystem(), pSettingsPath, nullptr))
	{
		printf("%s: Failed to parse VDF file: '%s'\n", __FUNCTION__, pSettingsPath);
		return;
	}

	const int settingsVersion = kv.GetInt("version", -1);

	if (settingsVersion != SDK_LAUNCHER_VERSION)
		return;

	KeyValues* sv = kv.FindKey("vars");

	if (!sv)
	{
		printf("%s: VDF file '%s' lacks subkey: '%s'\n", __FUNCTION__, pSettingsPath, "vars");
		return; // No settings to apply
	}

	// Game.
	this->m_PlaylistFileTextBox->SetText(sv->GetString("playlistsFile", "playlists_r5_patch.txt"));
	this->m_CheatsToggle->SetChecked(sv->GetBool("enableCheats"));
	this->m_DeveloperToggle->SetChecked(sv->GetBool("enableDeveloper"));
	this->m_ConsoleToggle->SetChecked(sv->GetBool("enableConsole"));
	this->m_ColorConsoleToggle->SetChecked(sv->GetBool("colorConsole", true));

	// Engine.
	this->m_ReservedCoresTextBox->SetText(sv->GetString("reservedCoreCount", "-1"));
	this->m_WorkerThreadsTextBox->SetText(sv->GetString("workerThreadCount", "-1"));
	this->m_ProcessorAffinityTextBox->SetText(sv->GetString("processorAffinity", "0"));
	this->m_NoAsyncJobsToggle->SetChecked(sv->GetBool("noAsync"));

	// Network.
	this->m_NetEncryptionToggle->SetChecked(sv->GetBool("encryptPackets", true));
	this->m_NetRandomKeyToggle->SetChecked(sv->GetBool("randomNetKey", true));
	this->m_QueuedPacketThread->SetChecked(sv->GetBool("queuedPackets", true));
	this->m_NoTimeOutToggle->SetChecked(sv->GetBool("noTimeOut"));

	// Video.
	this->m_WindowedToggle->SetChecked(sv->GetBool("windowed"));
	this->m_NoBorderToggle->SetChecked(sv->GetBool("borderless"));
	this->m_FpsTextBox->SetText(sv->GetString("fpsMax", "-1"));
	this->m_WidthTextBox->SetText(sv->GetString("width"));
	this->m_HeightTextBox->SetText(sv->GetString("height"));
}

//-----------------------------------------------------------------------------
// Purpose: save current launcher state
//-----------------------------------------------------------------------------
void CAdvancedSurface::SaveSettings()
{
	CUtlString settingsPath;
	settingsPath.Format("platform/" SDK_USER_CFG_PATH "%s", LAUNCHER_SETTING_FILE);

	CUtlString settingsDir = settingsPath.DirName();

	const char* pSettingsPath = settingsPath.String();
	const char* pSettingsDir = settingsDir.String();

	FileSystem()->CreateDirHierarchy(pSettingsDir);

	if (!FileSystem()->IsDirectory(pSettingsDir))
	{
		printf("%s: Failed to create directory: '%s'\n", __FUNCTION__, pSettingsPath);
		return;
	}

	KeyValues kv("LauncherSettings");
	kv.SetInt("version", SDK_LAUNCHER_VERSION);

	KeyValues* sv = new KeyValues("vars");

	if (!sv)
	{
		printf("%s: Failed to allocate subkey: '%s'\n", __FUNCTION__, "vars");
		return; // No settings to apply
	}

	kv.AddSubKey(sv);

	// Game.
	sv->SetString("playlistsFile", this->m_PlaylistFileTextBox->Text().ToCString());
	sv->SetBool("enableCheats", this->m_CheatsToggle->Checked());
	sv->SetBool("enableDeveloper", this->m_DeveloperToggle->Checked());
	sv->SetBool("enableConsole", this->m_ConsoleToggle->Checked());
	sv->SetBool("colorConsole", this->m_ColorConsoleToggle->Checked());

	// Engine.
	sv->SetString("reservedCoreCount", this->m_ReservedCoresTextBox->Text().ToCString());
	sv->SetString("workerThreadCount", this->m_WorkerThreadsTextBox->Text().ToCString());
	sv->SetString("processorAffinity", this->m_ProcessorAffinityTextBox->Text().ToCString());
	sv->SetBool("noAsync", this->m_NoAsyncJobsToggle->Checked());

	// Network.
	sv->SetBool("encryptPackets", this->m_NetEncryptionToggle->Checked());
	sv->SetBool("randomNetKey", this->m_NetRandomKeyToggle->Checked());
	sv->SetBool("queuedPackets", this->m_QueuedPacketThread->Checked());
	sv->SetBool("noTimeOut", this->m_NoTimeOutToggle->Checked());

	// Video.
	sv->SetBool("windowed", this->m_WindowedToggle->Checked());
	sv->SetBool("borderless", this->m_NoBorderToggle->Checked());
	sv->SetString("fpsMax", this->m_FpsTextBox->Text().ToCString());
	sv->SetString("width", this->m_WidthTextBox->Text().ToCString());
	sv->SetString("height", this->m_HeightTextBox->Text().ToCString());

	CUtlBuffer outBuf(ssize_t(0), 0, CUtlBuffer::TEXT_BUFFER);
	kv.RecursiveSaveToFile(outBuf, 0);

	if (!FileSystem()->WriteFile(pSettingsPath, "PLATFORM", outBuf))
	{
		printf("%s: Failed to create VDF file: '%s'\n", __FUNCTION__, pSettingsPath);
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: load callback
// Input  : *pSender - 
//-----------------------------------------------------------------------------
void CAdvancedSurface::OnLoad(Forms::Control* pSender)
{
	((CAdvancedSurface*)pSender->FindForm())->LoadSettings();
}

//-----------------------------------------------------------------------------
// Purpose: close callback
// Input  : *pSender - 
//-----------------------------------------------------------------------------
void CAdvancedSurface::OnClose(const std::unique_ptr<FormClosingEventArgs>& /*pEventArgs*/, Forms::Control* pSender)
{
	((CAdvancedSurface*)pSender->FindForm())->SaveSettings();
}

//-----------------------------------------------------------------------------
// Purpose: removes redundant files from the game install
// Input  : *pSender - 
//-----------------------------------------------------------------------------
void CAdvancedSurface::CleanSDK(Forms::Control* pSender)
{
	Msg(eDLL_T::COMMON, "Running cleaner for SDK installation\n");
	std::system("bin\\clean_sdk.bat");
}

//-----------------------------------------------------------------------------
// Purpose: updates the SDK
// Input  : *pSender - 
//-----------------------------------------------------------------------------
void CAdvancedSurface::UpdateSDK(Forms::Control* pSender)
{
	Msg(eDLL_T::COMMON, "Running updater for SDK installation\n");
	std::system("bin\\update_sdk.bat");
}

//-----------------------------------------------------------------------------
// Purpose: launches the game with the SDK
// Input  : *pSender - 
//-----------------------------------------------------------------------------
void CAdvancedSurface::LaunchGame(Forms::Control* pSender)
{
	CAdvancedSurface* pSurface = reinterpret_cast<CAdvancedSurface*>(pSender->FindForm());
	string svParameter;

	pSurface->AppendParameterInternal(svParameter, "-launcher");

	eLaunchMode launchMode = SDKLauncher()->BuildParameter(svParameter);
	uint64_t nProcessorAffinity = pSurface->GetProcessorAffinity(svParameter);

	if (SDKLauncher()->CreateLaunchContext(launchMode, nProcessorAffinity, svParameter.c_str(), "startup_launcher.cfg"))
		SDKLauncher()->LaunchProcess();
}

//-----------------------------------------------------------------------------
// Purpose: parses all available maps from the main vpk directory
//-----------------------------------------------------------------------------
void CAdvancedSurface::ParseMaps()
{
	if (!m_MapCombo->Items.Contains(""))
		m_MapCombo->Items.Add("");

	const fs::path vpkPath("vpk");

	if (!fs::exists(vpkPath))
	{
		return;
	}

	fs::directory_iterator directoryIterator(vpkPath);
	std::cmatch regexMatches;

	for (const fs::directory_entry& directoryEntry : directoryIterator)
	{
		std::string fileName = directoryEntry.path().u8string();
		std::regex_search(fileName.c_str(), regexMatches, g_VpkDirFileRegex);

		if (!regexMatches.empty())
		{
			const std::sub_match<const char*>& match = regexMatches[2];

			if (match.compare("frontend") == 0)
			{
				continue;
			}
			else if (match.compare("mp_common") == 0)
			{
				if (!this->m_MapCombo->Items.Contains("mp_lobby"))
				{
					this->m_MapCombo->Items.Add("mp_lobby");
				}
				continue;
			}
			else
			{
				const string mapName = match.str();

				if (!this->m_MapCombo->Items.Contains(match.str().c_str()))
				{
					this->m_MapCombo->Items.Add(match.str().c_str());
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: parses all playlists from user selected playlist file
//-----------------------------------------------------------------------------
void CAdvancedSurface::ParsePlaylists()
{
	if (!m_PlaylistCombo->Items.Contains(""))
		m_PlaylistCombo->Items.Add("");

	CUtlString playlistPath;
	playlistPath.Format("platform\\%s", this->m_PlaylistFileTextBox->Text().ToCString());

	const char* pPlaylistPath = playlistPath.String();

	if (!FileSystem()->FileExists(pPlaylistPath))
		return;

	KeyValues kv("playlists");

	if (!kv.LoadFromFile(FileSystem(), pPlaylistPath, nullptr))
	{
		printf("%s: Failed to parse playlists file: '%s'\n", __FUNCTION__, pPlaylistPath);
		return;
	}

	KeyValues* playlists = kv.FindKey("Playlists");

	if (!playlists)
		return; // Empty playlists

	for (KeyValues* pSubKey = playlists->GetFirstTrueSubKey(); pSubKey != nullptr; pSubKey = pSubKey->GetNextTrueSubKey())
	{
		const char* keyName = pSubKey->GetName();

		if (!this->m_PlaylistCombo->Items.Contains(keyName))
		{
			this->m_PlaylistCombo->Items.Add(keyName);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: clears the form and reloads the map list
// Input  : *pSender - 
//-----------------------------------------------------------------------------
void CAdvancedSurface::ReloadMaplists(Forms::Control* pSender)
{
	CAdvancedSurface* pSurface = reinterpret_cast<CAdvancedSurface*>(pSender->FindForm());

	pSurface->m_MapCombo->Items.Clear();
	pSurface->m_MapCombo->OnSizeChanged();

	pSurface->ParseMaps();
}

//-----------------------------------------------------------------------------
// Purpose: clears the form and reloads the playlist
// Input  : *pSender - 
//-----------------------------------------------------------------------------
void CAdvancedSurface::ReloadPlaylists(Forms::Control* pSender)
{
	CAdvancedSurface* pSurface = reinterpret_cast<CAdvancedSurface*>(pSender->FindForm());

	pSurface->m_PlaylistCombo->Items.Clear();
	pSurface->m_PlaylistCombo->OnSizeChanged();

	pSurface->ParsePlaylists();
}

//-----------------------------------------------------------------------------
// Purpose: adds a log to the surface console
// Input  : type - 
// Input  : *pszText - 
//-----------------------------------------------------------------------------
void CAdvancedSurface::AddLog(const LogType_t type, const char* const pszText)
{
	m_LogList.push_back(LogList_t(type, pszText));

	// Clamp the log list size, as we cannot fit more elements than
	// 8 in the console window.
	while (m_LogList.size() > 8)
	{
		m_LogList.erase(m_LogList.begin());
	}

	m_ConsoleListView->SetVirtualListSize(static_cast<int32_t>(m_LogList.size()));
	m_ConsoleListView->Refresh();
}

//-----------------------------------------------------------------------------
// Purpose: copies selected virtual items to clipboard
// Input  : &pEventArgs - 
// Input  : *pSender - 
//-----------------------------------------------------------------------------
void CAdvancedSurface::VirtualItemToClipboard(const std::unique_ptr<MouseEventArgs>& pEventArgs, Forms::Control* pSender)
{
	if (pEventArgs->Button != Forms::MouseButtons::Right)
		return;

	CAdvancedSurface* pSurface = reinterpret_cast<CAdvancedSurface*>(pSender->FindForm());
	List<uint32_t> lSelected = pSurface->m_ConsoleListView->SelectedIndices();

	if (!lSelected.Count())
		return;

	string svClipBoard;
	for (uint32_t i = 0; i < lSelected.Count(); i++)
		svClipBoard.append(pSurface->m_LogList[lSelected[i]].m_svText);

	clip::set_text(svClipBoard);
}

//-----------------------------------------------------------------------------
// Purpose: gets and handles the virtual item
// Input  : &pEventArgs - 
//			*pSender - 
//-----------------------------------------------------------------------------
void CAdvancedSurface::GetVirtualItem(const std::unique_ptr<Forms::RetrieveVirtualItemEventArgs>& pEventArgs, Forms::Control* pSender)
{
	CAdvancedSurface* pSurface = reinterpret_cast<CAdvancedSurface*>(pSender->FindForm());
	if (static_cast<int>(pSurface->m_LogList.size()) <= 0)
		return;

	pEventArgs->Style.ForeColor = (Gdiplus::ARGB)Drawing::Color::White;
	pEventArgs->Style.BackColor = pSender->BackColor();
	pSurface->m_ConsoleListView->SetVirtualListSize(static_cast<int32_t>(pSurface->m_LogList.size()));

	static const Drawing::Color cColor[] =
	{
		Drawing::Color(92, 236, 89),   // Info

		Drawing::Color(255, 255, 255),   // Unused

		Drawing::Color(236, 203, 0),   // Warn
		Drawing::Color(236, 28, 0),    // Error

		Drawing::Color(255, 255, 255),   // Unused
		Drawing::Color(255, 255, 255),   // Unused
	};
	static const String svLevel[] =
	{
		"info",

		"other",

		"warning",
		"error",

		"other",
		"other",
	};

	switch (pEventArgs->SubItemIndex)
	{
	case 0:
		pEventArgs->Style.ForeColor = cColor[(int)pSurface->m_LogList[pEventArgs->ItemIndex].m_nLevel];
		pEventArgs->Text = svLevel[(int)pSurface->m_LogList[pEventArgs->ItemIndex].m_nLevel];
		break;
	case 1:
		pEventArgs->Text = pSurface->m_LogList[pEventArgs->ItemIndex].m_svText;
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: forward input command to all game window instances
// Input  : *pSender - 
//-----------------------------------------------------------------------------
void CAdvancedSurface::ForwardCommandToGame(Forms::Control* pSender)
{
	CAdvancedSurface* pSurface = reinterpret_cast<CAdvancedSurface*>(pSender->FindForm());
	vector<HWND> vecHandles;

	if (!EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&vecHandles)))
		return;

	if (vecHandles.empty())
		return;

	const String kzCommand = pSurface->m_ConsoleCommandTextBox->Text();

	if (String::IsNullOrEmpty(kzCommand))
		return;

	bool bSuccess = false;

	for (const HWND hWindow : vecHandles)
	{
		char szWindowName[256];
		GetWindowTextA(hWindow, szWindowName, 256);

		COPYDATASTRUCT cData = { 0, (DWORD)(std::min)(kzCommand.Length(), (uint32_t)259) + 1, (void*)kzCommand.ToCString() };
		bool bProcessingMessage = SendMessageA(hWindow, WM_COPYDATA, NULL, (LPARAM)&cData); // WM_COPYDATA will only return 0 or 1, that's why we use a boolean.
		if (bProcessingMessage && !bSuccess)
		{
			bSuccess = true;
		}
	}

	if (bSuccess) // At least one game instance received the command.
	{
		pSurface->AddLog(LogType_t::LOG_INFO, Format("Sent command: %s\n", kzCommand.ToCString()).c_str());
		pSurface->m_ConsoleCommandTextBox->SetText("");
	}
}

//-----------------------------------------------------------------------------
// Purpose: formats and appends parameters, arguments to the parameter list
// Input  : &svParameterList - 
//			*szParameter - 
//			*szArgument - 
//-----------------------------------------------------------------------------
void CAdvancedSurface::AppendParameterInternal(string& svParameterList, const char* const szParameter, const char* const szArgument /*= nullptr*/)
{
	if (szArgument)
		svParameterList.append(Format("%s \"%s\"\n", szParameter, szArgument));
	else
		svParameterList.append(Format("%s\n", szParameter));
}

//-----------------------------------------------------------------------------
// Purpose: appends the reversed core count value to the command line buffer
// Input  : &svParameters - 
//-----------------------------------------------------------------------------
void CAdvancedSurface::AppendProcessorParameters(string& svParameters)
{
	const int nReservedCores = atoi(this->m_ReservedCoresTextBox->Text().ToCString());
	if (nReservedCores > -1) // A reserved core count of 0 seems to crash the game on some systems.
	{
		AppendParameterInternal(svParameters, "-numreservedcores", 
			this->m_ReservedCoresTextBox->Text().ToCString());
	}

	const int nWorkerThreads = atoi(this->m_WorkerThreadsTextBox->Text().ToCString());
	if (nWorkerThreads > -1)
	{
		AppendParameterInternal(svParameters, "-numworkerthreads",
			this->m_WorkerThreadsTextBox->Text().ToCString());
	}
}

//-----------------------------------------------------------------------------
// Purpose: appends the console parameters
// Input  : &svParameters - 
//-----------------------------------------------------------------------------
void CAdvancedSurface::AppendConsoleParameters(string& svParameters)
{
	if (this->m_ConsoleToggle->Checked())
		AppendParameterInternal(svParameters, "-wconsole");
	else
		AppendParameterInternal(svParameters, "-noconsole");

	if (this->m_ColorConsoleToggle->Checked())
		AppendParameterInternal(svParameters, "-ansicolor");

	if (!String::IsNullOrEmpty(this->m_PlaylistFileTextBox->Text()))
		AppendParameterInternal(svParameters, "-playlistfile", 
			this->m_PlaylistFileTextBox->Text().ToCString());
}

//-----------------------------------------------------------------------------
// Purpose: appends the video parameters
// Input  : &svParameters - 
//-----------------------------------------------------------------------------
void CAdvancedSurface::AppendVideoParameters(string& svParameters)
{
	if (this->m_WindowedToggle->Checked())
		AppendParameterInternal(svParameters, "-windowed");
	else
		AppendParameterInternal(svParameters, "-fullscreen");

	if (this->m_NoBorderToggle->Checked())
		AppendParameterInternal(svParameters, "-noborder");
	else
		AppendParameterInternal(svParameters, "-forceborder");

	if (StringIsDigit(this->m_FpsTextBox->Text().ToCString()))
		AppendParameterInternal(svParameters, "+fps_max", 
			this->m_FpsTextBox->Text().ToCString());

	if (!String::IsNullOrEmpty(this->m_WidthTextBox->Text()))
		AppendParameterInternal(svParameters, "-w", 
			this->m_WidthTextBox->Text().ToCString());

	if (!String::IsNullOrEmpty(this->m_HeightTextBox->Text()))
		AppendParameterInternal(svParameters, "-h", 
			this->m_HeightTextBox->Text().ToCString());
}

//-----------------------------------------------------------------------------
// Purpose: appends the host parameters
// Input  : &svParameters - 
//-----------------------------------------------------------------------------
void CAdvancedSurface::AppendHostParameters(string& svParameters)
{
	if (!String::IsNullOrEmpty(this->m_HostNameTextBox->Text()))
	{
		AppendParameterInternal(svParameters, "+hostname", this->m_HostNameTextBox->Text().ToCString());
		const char* szMode = "0"; // '0' = Offline (default).

		switch (static_cast<eVisibility>(this->m_VisibilityCombo->SelectedIndex()))
		{
		case eVisibility::PUBLIC:
		{
			szMode = "2";
			break;
		}
		case eVisibility::HIDDEN:
		{
			szMode = "1";
			break;
		}
		}

		AppendParameterInternal(svParameters, "+sv_pylonVisibility", szMode);
	}
}

//-----------------------------------------------------------------------------
// Purpose: appends the net parameters
// Input  : &svParameters - 
//-----------------------------------------------------------------------------
void CAdvancedSurface::AppendNetParameters(string& svParameters)
{
	AppendParameterInternal(svParameters, "+net_encryptionEnable", this->m_NetEncryptionToggle->Checked() ? "1" : "0");
	AppendParameterInternal(svParameters, "+net_useRandomKey", this->m_NetRandomKeyToggle->Checked() ? "1" : "0");
	AppendParameterInternal(svParameters, "+net_queued_packet_thread", this->m_QueuedPacketThread->Checked() ? "1" : "0");

	if (this->m_NoTimeOutToggle->Checked())
		AppendParameterInternal(svParameters, "-notimeout");
}

//-----------------------------------------------------------------------------
// Purpose: clears the form and reloads the playlist
// Input  : &svParameters - 
// Output : eLaunchMode [HOST - SERVER - CLIENT - NONE]
//-----------------------------------------------------------------------------
eLaunchMode CAdvancedSurface::BuildParameter(string& svParameters)
{
	eLaunchMode results = eLaunchMode::LM_NONE;

	this->AppendProcessorParameters(svParameters);
	this->AppendConsoleParameters(svParameters);
	this->AppendNetParameters(svParameters);

	switch (static_cast<eMode>(this->m_ModeCombo->SelectedIndex()))
	{
	case eMode::HOST:
	{
		// GAME ###############################################################
		if (!String::IsNullOrEmpty(this->m_MapCombo->Text()))
		{
			AppendParameterInternal(svParameters, "+map", this->m_MapCombo->Text().ToCString());
		}
		if (!String::IsNullOrEmpty(this->m_PlaylistCombo->Text()))
		{
			AppendParameterInternal(svParameters, "+launchplaylist", this->m_PlaylistCombo->Text().ToCString());
		}
		if (this->m_DeveloperToggle->Checked())
		{
			AppendParameterInternal(svParameters, "-dev");
			AppendParameterInternal(svParameters, "-devsdk");
			results = eLaunchMode::LM_GAME_DEV;
		}
		else
			results = eLaunchMode::LM_GAME;

		if (this->m_CheatsToggle->Checked())
		{
			AppendParameterInternal(svParameters, "-dev");
			AppendParameterInternal(svParameters, "-showdevmenu");
		}

		// ENGINE ###############################################################
		if (this->m_NoAsyncJobsToggle->Checked())
		{
			AppendParameterInternal(svParameters, "-noasync");
			AppendParameterInternal(svParameters, "+async_serialize", "0");
			AppendParameterInternal(svParameters, "+buildcubemaps_async", "0");
			AppendParameterInternal(svParameters, "+sv_asyncAIInit", "0");
			AppendParameterInternal(svParameters, "+sv_asyncSendSnapshot", "0");
			AppendParameterInternal(svParameters, "+sv_scriptCompileAsync", "0");
			AppendParameterInternal(svParameters, "+cl_scriptCompileAsync", "0");
			AppendParameterInternal(svParameters, "+cl_async_bone_setup", "0");
			AppendParameterInternal(svParameters, "+cl_updatedirty_async", "0");
			AppendParameterInternal(svParameters, "+mat_syncGPU", "1");
			AppendParameterInternal(svParameters, "+mat_sync_rt", "1");
			AppendParameterInternal(svParameters, "+mat_sync_rt_flushes_gpu", "1");
			AppendParameterInternal(svParameters, "+net_async_sendto", "0");
			AppendParameterInternal(svParameters, "+physics_async_sv", "0");
			AppendParameterInternal(svParameters, "+physics_async_cl", "0");
		}

		this->AppendHostParameters(svParameters);
		this->AppendVideoParameters(svParameters);

		if (!String::IsNullOrEmpty(this->m_LaunchArgsTextBox->Text()))
			AppendParameterInternal(svParameters, this->m_LaunchArgsTextBox->Text().ToCString());

		return results;
	}
	case eMode::SERVER:
	{
		// GAME ###############################################################
		if (!String::IsNullOrEmpty(this->m_MapCombo->Text()))
		{
			AppendParameterInternal(svParameters, "+map", this->m_MapCombo->Text().ToCString());
		}
		if (!String::IsNullOrEmpty(this->m_PlaylistCombo->Text()))
		{
			AppendParameterInternal(svParameters, "+launchplaylist", this->m_PlaylistCombo->Text().ToCString());
		}
		if (this->m_DeveloperToggle->Checked())
		{
			AppendParameterInternal(svParameters, "-dev");
			AppendParameterInternal(svParameters, "-devsdk");
			results = eLaunchMode::LM_SERVER_DEV;
		}
		else
			results = eLaunchMode::LM_SERVER;

		if (this->m_CheatsToggle->Checked())
			AppendParameterInternal(svParameters, "+sv_cheats", "1");

		// ENGINE ###############################################################
		if (this->m_NoAsyncJobsToggle->Checked())
		{
			AppendParameterInternal(svParameters, "-noasync");
			AppendParameterInternal(svParameters, "+async_serialize", "0");
			AppendParameterInternal(svParameters, "+sv_asyncAIInit", "0");
			AppendParameterInternal(svParameters, "+sv_asyncSendSnapshot", "0");
			AppendParameterInternal(svParameters, "+sv_scriptCompileAsync", "0");
			AppendParameterInternal(svParameters, "+physics_async_sv", "0");
		}

		this->AppendHostParameters(svParameters);

		if (!String::IsNullOrEmpty(this->m_LaunchArgsTextBox->Text()))
			AppendParameterInternal(svParameters, this->m_LaunchArgsTextBox->Text().ToCString());

		return results;
	}
	case eMode::CLIENT:
	{
		// Tells the loader module to only load the client dll.
		AppendParameterInternal(svParameters, "-noserverdll");

		// GAME ###############################################################
		if (this->m_DeveloperToggle->Checked())
		{
			AppendParameterInternal(svParameters, "-dev");
			AppendParameterInternal(svParameters, "-devsdk");
			results = eLaunchMode::LM_CLIENT_DEV;
		}
		else
			results = eLaunchMode::LM_CLIENT;

		if (this->m_CheatsToggle->Checked())
		{
			AppendParameterInternal(svParameters, "-dev");
			AppendParameterInternal(svParameters, "-showdevmenu");
		}

		// ENGINE ###############################################################
		if (this->m_NoAsyncJobsToggle->Checked())
		{
			AppendParameterInternal(svParameters, "-noasync");
			AppendParameterInternal(svParameters, "+async_serialize", "0");
			AppendParameterInternal(svParameters, "+buildcubemaps_async", "0");
			AppendParameterInternal(svParameters, "+cl_scriptCompileAsync", "0");
			AppendParameterInternal(svParameters, "+cl_async_bone_setup", "0");
			AppendParameterInternal(svParameters, "+cl_updatedirty_async", "0");
			AppendParameterInternal(svParameters, "+mat_syncGPU", "1");
			AppendParameterInternal(svParameters, "+mat_sync_rt", "1");
			AppendParameterInternal(svParameters, "+mat_sync_rt_flushes_gpu", "1");
			AppendParameterInternal(svParameters, "+net_async_sendto", "0");
			AppendParameterInternal(svParameters, "+physics_async_cl", "0");
		}

		this->AppendVideoParameters(svParameters);

		// MAIN ###############################################################
		if (!String::IsNullOrEmpty(this->m_LaunchArgsTextBox->Text()))
			AppendParameterInternal(svParameters, this->m_LaunchArgsTextBox->Text().ToCString());

		return results;
	}
	default:
		Assert(0);
		return results;
	}
}

//-----------------------------------------------------------------------------
// Purpose: gets processor affinity, and automatically appends the single-core
//          dedi cvar if core count = 1
// Input  : &svParameters - 
// 
// CPU3 CPU2 CPU1 CPU0  Bin  Hex
// ---- ---- ---- ----  ---  ---
// OFF  OFF  OFF  ON  = 0001 = 1
// OFF  OFF  ON   OFF = 0010 = 2
// OFF  OFF  ON   ON  = 0011 = 3
// OFF  ON   OFF  OFF = 0100 = 4
// OFF  ON   OFF  ON  = 0101 = 5
// OFF  ON   ON   OFF = 0110 = 6
// OFF  ON   ON   ON  = 0111 = 7
// ON   OFF  OFF  OFF = 1000 = 8
// ON   OFF  OFF  ON  = 1001 = 9
// ON   OFF  ON   OFF = 1010 = A
// ON   OFF  ON   ON  = 1011 = B
// ON   ON   OFF  OFF = 1100 = C
// ON   ON   OFF  ON  = 1101 = D
// ON   ON   ON   OFF = 1110 = E
// ON   ON   ON   ON  = 1111 = F
//-----------------------------------------------------------------------------
uint64_t CAdvancedSurface::GetProcessorAffinity(string& svParameters)
{
	char* pEnd;
	const uint64_t nProcessorAffinity = strtoull(this->m_ProcessorAffinityTextBox->Text().ToCString(), &pEnd, 16);

	if (nProcessorAffinity)
	{
		const uint32_t nProcessorCount = PopCount(nProcessorAffinity);

		if (nProcessorCount == 1)
			AppendParameterInternal(svParameters, "+sv_single_core_dedi", "1");
	}

	return nProcessorAffinity;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAdvancedSurface::CAdvancedSurface() : Forms::Form()
{
	// Game.
	m_GameGroup = nullptr;
	m_GameGroupExt = nullptr;
	m_MapLabel = nullptr;
	m_MapCombo = nullptr;
	m_PlaylistLabel = nullptr;
	m_PlaylistCombo = nullptr;
	m_CheatsToggle = nullptr;
	m_DeveloperToggle = nullptr;
	m_ConsoleToggle = nullptr;
	m_ColorConsoleToggle = nullptr;
	m_PlaylistFileTextBox = nullptr;
	m_PlaylistFileLabel = nullptr;

	// Main.
	m_MainGroup = nullptr;
	m_MainGroupExt = nullptr;
	m_ModeCombo = nullptr;
	m_ModeLabel = nullptr;
	m_HostNameTextBox = nullptr;
	m_HostNameLabel = nullptr;
	m_VisibilityCombo = nullptr;
	m_VisibilityLabel = nullptr;
	m_LaunchArgsTextBox = nullptr;
	m_LaunchArgsLabel = nullptr;
	m_CleanSDK = nullptr;
	m_UpdateSDK = nullptr;
	m_LaunchSDK = nullptr;

	// Engine.
	m_EngineBaseGroup = nullptr;
	m_EngineNetworkGroup = nullptr;
	m_EngineVideoGroup = nullptr;
	m_ReservedCoresTextBox = nullptr;
	m_ReservedCoresLabel = nullptr;
	m_WorkerThreadsTextBox = nullptr;
	m_WorkerThreadsLabel = nullptr;
	m_ProcessorAffinityTextBox = nullptr;
	m_ProcessorAffinityLabel = nullptr;
	m_NoAsyncJobsToggle = nullptr;
	m_NetEncryptionToggle = nullptr;
	m_NetRandomKeyToggle = nullptr;
	m_QueuedPacketThread = nullptr;
	m_NoTimeOutToggle = nullptr;
	m_WindowedToggle = nullptr;
	m_NoBorderToggle = nullptr;
	m_FpsTextBox = nullptr;
	m_FpsLabel = nullptr;
	m_WidthTextBox = nullptr;
	m_HeightTextBox = nullptr;
	m_ResolutionLabel = nullptr;

	// Console.
	m_ConsoleGroup = nullptr;
	m_ConsoleGroupExt = nullptr;
	m_ConsoleListView = nullptr;
	m_ConsoleCommandTextBox = nullptr;
	m_ConsoleSendCommand = nullptr;
}
