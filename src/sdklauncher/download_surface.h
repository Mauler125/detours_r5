#pragma once

class CProgressPanel : public Forms::Form
{
public:
	CProgressPanel();
	virtual ~CProgressPanel() = default;

	// Updates the progress
	void UpdateProgress(uint32_t Progress, bool Finished);
	const bool IsCanceled() const;
	
	void SetCanCancel(bool bEnable);

	void SetAutoClose(bool Value);
	void SetExportLabel(const char* pNewLabel);

private:
	// Internal routine to setup the component
	void InitializeComponent();

	// Internal event on finish click
	static void OnFinishClick(Forms::Control* Sender);
	static void OnCancelClick(Forms::Control* Sender);

	void CancelProgress(); // Cancels the progress

	// Internal controls reference
	UIX::UIXLabel* m_DownloadLabel;
	UIX::UIXButton* m_CancelButton;
	UIX::UIXProgressBar* m_ProgressBar;

	bool m_bCanClose;  // Whether or not we can close.
	bool m_bCanceled;  // Whether or not we canceled it.
	bool m_bAutoClose; // Whether or not to automatically close.
};
