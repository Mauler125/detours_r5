#pragma once

/////////////////////////////////////////////////////////////////////////////
// Internals
BOOL IsBadReadPtrV2(void* ptr);
BOOL FileExists(LPCTSTR szPath);
int CreateDirHierarchy(const char* filePath);
bool IsDirectory(const char* path);
bool FileEmpty(ifstream& pFile);
MODULEINFO GetModuleInfo(const char* szModule);

/////////////////////////////////////////////////////////////////////////////
// Debug
void DbgPrint(LPCSTR sFormat, ...);
void PrintLastError(void);
void HexDump(const char* szHeader, const char* szLogger, const void* pData, size_t nSize);

/////////////////////////////////////////////////////////////////////////////
// Char
char* StripTabsAndReturns(const char* pInBuffer, char* pOutBuffer, int nOutBufferSize);
char* StripQuotes(const char* pInBuffer, char* pOutBuffer, int nOutBufferSize);

/////////////////////////////////////////////////////////////////////////////
// String
bool HasPartial(const string& svInput, const string& svPartial);
bool HasExtension(const string& svInput, const string& svExtension);
string GetExtension(const string& svInput, bool bReturnOriginal = false, bool bKeepDelimiter = false);
string RemoveExtension(const string& svInput);

bool HasFileName(const string& svInput, const string& svFileName);
string GetFileName(const string& svInput, bool bRemoveExtension = false, bool bWindows = false);
string RemoveFileName(const string& svInput, bool bWindows = false);

string CreateTimedFileName();
string CreateUUID();
void CreateDirectories(string svInput, string* pszOutput = nullptr, bool bWindows = false);

void AppendSlash(string& svInput, const char separator = '\\');

string ConvertToWinPath(const string& svInput);
string ConvertToUnixPath(const string& svInput);

bool IsEqualNoCase(const string& svInput, const string& svSecond);
bool IsValidBase64(const string& svInput, string* psvOutput = nullptr);

string Base64Encode(const string& svInput);
string Base64Decode(const string& svInput);

string UTF8Encode(const wstring& wsvInput);
//string UTF8Decode(const string& svInput);

bool StringIsDigit(const string& svInput);
bool CompareStringAlphabetically(const string& svA, const string& svB);
bool CompareStringLexicographically(const string& svA, const string& svB);

bool StringReplace(string& svInput, const string& svFrom, const string& svTo);
string StringReplaceC(const string& svInput, const string& svFrom, const string& svTo);
string StringEscape(const string& svInput);
string StringUnescape(const string& svInput);
size_t StringCount(const string& svInput, char cDelim);
vector<string> StringSplit(string svInput, char cDelim, size_t nMax = SIZE_MAX);

string& StringLTrim(string& svInput, const char* pszToTrim, bool bTrimBefore = false);
string& StringRTrim(string& svInput, const char* pszToTrim, bool bTrimAfter = false);
string& StringTrim(string& svInput, const char* pszToTrim, bool bTrimAll = false);

typedef char FourCCString_t[5];
void FourCCToString(FourCCString_t& buf, const int n);

/////////////////////////////////////////////////////////////////////////////
// Bytes
vector<int> StringToBytes(const char* szInput, bool bNullTerminator);
pair<vector<uint8_t>, string> StringToMaskedBytes(const char* szInput, bool bNullTerminator);
vector<int> PatternToBytes(const char* szInput);
pair<vector<uint8_t>, string> PatternToMaskedBytes(const char* szInput);
vector<int> IntToDigits(int iValue);

/////////////////////////////////////////////////////////////////////////////
// Print
void PrintM128i8(__m128i in);
void PrintM128i16(__m128i in);
void PrintM128i32(__m128i in);
void PrintM128i64(__m128i in);

void AppendPrintf(char* pBuffer, size_t nBufSize, char const* pFormat, ...);
string PrintPercentageEscape(const string& svInput);

string FormatBytes(size_t nBytes);
string FormatV(const char* szFormat, va_list args);
string Format(const char* szFormat, ...);

/////////////////////////////////////////////////////////////////////////////
// Array
template <typename Iter, typename Compare>
inline Iter ExtremeElement(Iter first, Iter last, Compare compare)
{
    return std::min_element(first, last, compare);
}

template <typename Iter> // Return lowest element in array.
inline Iter MinElement(Iter first, Iter last)
{
    return ExtremeElement(first, last, std::less<>());
}

template <typename Iter> // Return highest element in array.
inline Iter MaxElement(Iter first, Iter last)
{
    return ExtremeElement(first, last, std::greater<>());
}

/////////////////////////////////////////////////////////////////////////////
// Net
int CompareIPv6(const IN6_ADDR& ipA, const IN6_ADDR& ipB);

/////////////////////////////////////////////////////////////////////////////
// Time
uint64_t GetUnixTimeStamp();
std::chrono::nanoseconds IntervalToDuration(const float flInterval);

/////////////////////////////////////////////////////////////////////////////
