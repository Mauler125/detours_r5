#pragma once

struct MODULERESOURCE
{
	MODULERESOURCE(void)
	{
		m_pData = nullptr;
		m_nSize = NULL;
		m_idIcon = nullptr;
		m_nWidth = NULL;
		m_nHeight = NULL;
	}
	MODULERESOURCE(LPVOID pData, DWORD nSize, ID3D11ShaderResourceView* pIcon = nullptr, 
		int nWidth = NULL, int nHeight = NULL)
	{
		m_pData = pData;
		m_nSize = nSize;
		m_idIcon = pIcon;
		m_nWidth = nWidth;
		m_nHeight = nHeight;
	}
	LPVOID m_pData;
	DWORD  m_nSize;
	ID3D11ShaderResourceView* m_idIcon;
	int m_nWidth;
	int m_nHeight;
};

MODULERESOURCE GetModuleResource(HMODULE hModule, const int iResource);
