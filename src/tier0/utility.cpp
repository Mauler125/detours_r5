/*-----------------------------------------------------------------------------
 * _utility
 *-----------------------------------------------------------------------------*/

#include "core/logdef.h"
#include "tier0/utility.h"

// These are used for the 'stat()' and 'access()' in ::IsDirectory().
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h> 

///////////////////////////////////////////////////////////////////////////////
// For checking if a specific file exists.
BOOL FileExists(LPCTSTR szPath)
{
    DWORD dwAttrib = GetFileAttributes(szPath);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
        !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

///////////////////////////////////////////////////////////////////////////////
// For creating a directory hierarchy
int CreateDirHierarchy(const char* const filePath)
{
    char fullPath[1024];
    int results;

    snprintf(fullPath, sizeof(fullPath), "%s", filePath);
    V_FixSlashes(fullPath);

    const size_t pathLen = strlen(fullPath);
    char* const pathEnd = &fullPath[pathLen - 1];

    // Strip the trailing slash if there's one
    if (*pathEnd == CORRECT_PATH_SEPARATOR)
        *pathEnd = '\0';

    // Get the pointer to the last dir separator, the last dir is what we want
    // to create and return the value of which is why we run that outside the
    // loop
    const char* const lastDir = strrchr(fullPath, CORRECT_PATH_SEPARATOR);

    char* pFullPath = fullPath;
    while ((pFullPath = strchr(pFullPath, CORRECT_PATH_SEPARATOR)) != NULL)
    {
        // Temporarily turn the slash into a null
        // to get the current directory.
        *pFullPath = '\0';

        results = _mkdir(fullPath);

        if (results && errno != EEXIST)
            return results;

        *pFullPath = CORRECT_PATH_SEPARATOR;

        // Last dir should be created separately, and its return value should
        // be returned
        if (pFullPath == lastDir)
            break;

        pFullPath++;
    }

    // Try to create the final directory in the path.
    return _mkdir(fullPath);
}

///////////////////////////////////////////////////////////////////////////////
// For checking if a directory exists
bool IsDirectory(const char* const path)
{
    if (_access(path, 0) == 0)
    {
        struct stat status;
        stat(path, &status);

        return (status.st_mode & S_IFDIR) != 0;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
// For checking if a specific file is empty.
bool FileEmpty(ifstream& pFile)
{
    return pFile.peek() == ifstream::traits_type::eof();
}

///////////////////////////////////////////////////////////////////////////////
// For checking if pointer is valid or bad.
BOOL IsBadReadPtrV2(const void* const ptr)
{
    MEMORY_BASIC_INFORMATION mbi = { 0 };
    if (::VirtualQuery(ptr, &mbi, sizeof(mbi)))
    {
        DWORD mask = (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
        bool b = !(mbi.Protect & mask);
        // check the page is not a guard page
        if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) b = true;
        return b;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// For getting information about the executing module.
MODULEINFO GetModuleInfo(const char* const szModule)
{
    MODULEINFO modinfo = { 0 };
    HMODULE hModule = GetModuleHandleA(szModule);

    if (hModule && hModule != INVALID_HANDLE_VALUE)
    {
        GetModuleInformation(GetCurrentProcess(), hModule, &modinfo, sizeof(MODULEINFO));
    }

    return modinfo;
}

///////////////////////////////////////////////////////////////////////////////
// For printing output to the debugger.
void DbgPrint(const char* const sFormat, ...)
{
    va_list sArgs;
    va_start(sArgs, sFormat);

    const string result = FormatV(sFormat, sArgs);
    va_end(sArgs);

    // Output the string to the debugger.
    OutputDebugStringA(result.c_str());
}

///////////////////////////////////////////////////////////////////////////////
// For printing the last error to the console if any.
void PrintLastError(void)
{
    const DWORD errorMessageID = ::GetLastError();

    if (errorMessageID != NULL)
    {
        LPSTR messageBuffer;
        const DWORD size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

        if (size > 0)
        {
            spdlog::error("{:s}\n", messageBuffer);
            LocalFree(messageBuffer);
        }
        else // FormatMessageA failed.
        {
            spdlog::error("{:s}: Failed: {:s}\n", __FUNCTION__, 
                std::system_category().message(static_cast<int>(::GetLastError())));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// For dumping data from a buffer to a file on the disk.
void HexDump(const char* const szHeader, const char* const szLogger, const void* const pData, const size_t nSize)
{
    char szAscii[17];
    static std::mutex m;

    std::shared_ptr<spdlog::logger> logger;

    m.lock();
    szAscii[16] = '\0';

    if (szLogger)
    {
        logger = spdlog::get(szLogger);
        if (!logger)
        {
            logger = spdlog::default_logger();
            m.unlock();
            assert(0);
            return;
        }
    }
    else
    {
        logger = spdlog::default_logger();
    }

    // Add time stamp.
    logger->set_level(spdlog::level::trace);
    logger->set_pattern("%v [%H:%M:%S.%f]\n");
    logger->trace("--------------------------------------------------------");

    // Disable EOL and create block header.
    logger->set_pattern("%v");
    logger->trace("{:-<32s} LEN BYTES: {:<20d} {:<8s}:\n:{:<72s}:\n", szHeader, nSize, " ", " ");
    logger->trace("-------  0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F  0123456789ABCDEF\n");

    // Output the buffer to the file.
    for (size_t i = 0; i < nSize; i++)
    {
        if (i % nSize == 0)
        {
            logger->trace("0x{:04X}  ", i);
        }

        logger->trace("{:02x} ", (reinterpret_cast<const uint8_t*>(pData))[i]);

        if ((reinterpret_cast<const uint8_t*>(pData))[i] >= ' ' && 
            (reinterpret_cast<const uint8_t*>(pData))[i] <= '~')
        {
            szAscii[i % 16] = (reinterpret_cast<const uint8_t*>(pData))[i];
        }
        else
        {
            szAscii[i % 16] = '.';
        }

        if ((i + 1) % 8 == 0 || i + 1 == nSize)
        {
            logger->trace(' ');

            if ((i + 1) % 16 == 0)
            {
                if (i + 1 == nSize)
                {
                    logger->trace("{:s}\n--------------------------------------------------------------------------\n\n", szAscii);
                }
                else
                {
                    i++;
                    logger->trace("{:s}\n0x{:04X}  ", szAscii, i--);
                }
            }
            else if (i + 1 == nSize)
            {
                szAscii[(i + 1) % 16] = '\0';
                if ((i + 1) % 16 <= 8)
                {
                    logger->trace(' ');
                }
                for (size_t j = (i + 1) % 16; j < 16; j++)
                {
                    logger->trace("   ");
                }
                logger->trace("{:s}\n--------------------------------------------------------------------------\n\n", szAscii);
            }
        }
    }
    m.unlock();
    ///////////////////////////////////////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////
// For stripping tabs and return characters from input buffer.
char* StripTabsAndReturns(const char* const pInBuffer, char* const pOutBuffer, ssize_t nOutBufferSize)
{
    char* out = pOutBuffer;
    const char* i = pInBuffer;
    char* o = out;

    out[0] = 0;

    while (*i && o - out < nOutBufferSize - 1)
    {
        if (*i == '\n' ||
            *i == '\r' ||
            *i == '\t')
        {
            *o++ = ' ';
            i++;
            continue;
        }
        if (*i == '\"')
        {
            *o++ = '\'';
            i++;
            continue;
        }

        *o++ = *i++;
    }

    *o = '\0';

    return out;
}

///////////////////////////////////////////////////////////////////////////////
// For stripping quote characters from input buffer.
char* StripQuotes(const char* const pInBuffer, char* const pOutBuffer, const ssize_t nOutBufferSize)
{
    const char* i = pInBuffer;
    char* o = pOutBuffer;

    pOutBuffer[0] = 0;

    while (*i && o - pOutBuffer < nOutBufferSize - 1)
    {
        if (*i == '\"')
        {
            *o++ = '\'';
            i++;
            continue;
        }

        *o++ = *i++;
    }

    *o = '\0';

    return pOutBuffer;
}

///////////////////////////////////////////////////////////////////////////////
// For finding a partial string within input (case insensitive).
bool HasPartial(const string& svInput, const string& svPartial)
{
    const auto it = std::search(svInput.begin(), svInput.end(),
        svPartial.begin(), svPartial.end(), [](unsigned char ci, unsigned char cp)
        {
            return std::toupper(ci) == std::toupper(cp);
        }
    );
    return (it != svInput.end());
}

///////////////////////////////////////////////////////////////////////////////
// For checking if file name has a specific extension.
bool HasExtension(const string& svInput, const string& svExtension)
{
    if (svInput.substr(svInput.find_last_of('.') + 1) == svExtension)
    {
        return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
// For removing file names from the extension.
string GetExtension(const string& svInput, const bool bReturnOriginal, const bool bKeepDelimiter)
{
    string::size_type nPos = svInput.rfind('.');

    if (nPos != string::npos)
    {
        if (!bKeepDelimiter)
        {
            nPos += 1;
        }

       return svInput.substr(nPos);
    }

    if (bReturnOriginal)
    {
        return svInput;
    }

    return "";
}

///////////////////////////////////////////////////////////////////////////////
// For removing extensions from file names.
string RemoveExtension(const string& svInput)
{
    const string::size_type nPos = svInput.find_last_of('.');

    if (nPos == string::npos)
    {
        return svInput;
    }
    return svInput.substr(0, nPos);
}

///////////////////////////////////////////////////////////////////////////////
// For checking file names equality without extension.
bool HasFileName(const string& svInput, const string& svFileName)
{
    if (RemoveExtension(svInput) == RemoveExtension(svFileName))
    {
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
// For removing the path from file names.
string GetFileName(const string& svInput, bool bRemoveExtension, bool bWindows)
{
    const string::size_type nPos = svInput.rfind(bWindows ? '\\' : '/');

    if (nPos != string::npos)
    {
        if (bRemoveExtension)
        {
            return RemoveExtension(svInput.substr(nPos + 1));
        }

        return svInput.substr(nPos + 1);
    }
    else // File name is not within a path.
    {
        if (bRemoveExtension)
        {
            return RemoveExtension(svInput);
        }
    }

    return svInput;
}

///////////////////////////////////////////////////////////////////////////////
// For removing file names from the path.
string RemoveFileName(const string& svInput, const bool bWindows)
{
    const string::size_type nPos = svInput.find_last_of(bWindows ? '\\' : '/');

    if (nPos == string::npos)
    {
        return "";
    }

    return svInput.substr(0, nPos);
}

///////////////////////////////////////////////////////////////////////////////
// For creating a file name with the current (now) date and time
string CreateTimedFileName()
{
    const auto now = std::chrono::system_clock::now();

    // Get number of milliseconds for the current second (remainder after division into seconds).
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    // Convert to std::time_t in order to convert to std::tm (broken time).
    const auto timer = std::chrono::system_clock::to_time_t(now);

    ostringstream oss;

    oss << std::put_time(std::localtime(&timer), "%Y-%m-%d_%H-%M-%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return oss.str(); // 'YY-MM-DD_HH-MM-SS.MMM'.
}

///////////////////////////////////////////////////////////////////////////////
// For creating universally unique identifiers.
string CreateUUID()
{
    UUID uuid;
    UuidCreate(&uuid);

    char* str;
    UuidToStringA(&uuid, (RPC_CSTR*)&str);
    string result(str);

    RpcStringFreeA((RPC_CSTR*)&str);
    return result;
}

///////////////////////////////////////////////////////////////////////////////
// For creating directories for output streams.
void CreateDirectories(string svInput, string* pszOutput, bool bWindows)
{
    if (bWindows)
    {
        StringReplace(svInput, "\\ \\", "\\");
        StringReplace(svInput, " \\", "");
    }
    else
    {
        StringReplace(svInput, "/ /", "/");
        StringReplace(svInput, " /", "");
    }

    fs::path fspPathOut(svInput);

    if (pszOutput)
    {
        *pszOutput = fspPathOut.u8string();
    }

    fspPathOut = fspPathOut.parent_path();
    fs::create_directories(fspPathOut);
}

///////////////////////////////////////////////////////////////////////////////
// For appending a slash at the end of the string if not already present.
void AppendSlash(string& svInput, const char separator)
{
    char lchar = svInput[svInput.size() - 1];

    if (lchar != '\\' && lchar != '/')
    {
        svInput.push_back(separator);
    }
}

///////////////////////////////////////////////////////////////////////////////
// For converting file paths to windows file paths.
string ConvertToWinPath(const string& svInput)
{
    string result = svInput;

    // Flip forward slashes in file path to windows-style backslash
    for (size_t i = 0; i < result.size(); i++)
    {
        if (result[i] == '/')
        {
            result[i] = '\\';
        }
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// For converting file paths to unix file paths.
string ConvertToUnixPath(const string& svInput)
{
    string result = svInput;

    // Flip windows-style backslashes in file path to forward slash
    for (size_t i = 0; i < result.size(); i++)
    {
        if (result[i] == '\\')
        {
            result[i] = '/';
        }
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// For comparing two strings (case insensitive).
bool IsEqualNoCase(const string& svInput, const string& svSecond)
{
    return std::equal(svInput.begin(), svInput.end(), svSecond.begin(), svSecond.end(),
        [](unsigned char ci, unsigned char cs)
        {
            return std::toupper(ci) == std::toupper(cs);
        });
}

///////////////////////////////////////////////////////////////////////////////
// For checking if input is a valid Base64.
bool IsValidBase64(const string& svInput, string* const psvOutput)
{
    static const std::regex rx(R"((?:[A-Za-z0-9+\/]{4}?)*(?:[A-Za-z0-9+\/]{2}==|[A-Za-z0-9+\/]{3}=))");
    std::smatch mh;

    if (std::regex_search(svInput, mh, rx))
    {
        if (psvOutput)
        {
            *psvOutput = mh[0].str();
        }

        return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
// For encoding data in Base64.
string Base64Encode(const string& svInput)
{
    string result;
    int val = 0, valb = -6;

    for (unsigned char c : svInput)
    {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0)
        {
            result.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }

    if (valb > -6)
    {
        result.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val << 8) >> (valb + 8)) & 0x3F]);
    }

    while (result.size() % 4)
    {
        result.push_back('=');
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// For decoding data in Base64.
string Base64Decode(const string& svInput)
{
    string result;
    vector<int> T(256, -1);

    int val = 0, valb = -8;

    for (int i = 0; i < 64; i++)
    {
        T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;
    }

    for (unsigned char c : svInput)
    {
        if (T[c] == -1)
        {
            break;
        }

        val = (val << 6) + T[c];
        valb += 6;

        if (valb >= 0)
        {
            result.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// For encoding data in UTF-8.
string UTF8Encode(const wstring& wsvInput)
{
    string result;
    const int nLen = WideCharToMultiByte(CP_UTF8, 0, wsvInput.c_str(), int(wsvInput.length()), NULL, 0, NULL, NULL);

    if (nLen > 0)
    {
        result.resize(nLen);
        WideCharToMultiByte(CP_UTF8, 0, wsvInput.c_str(), int(wsvInput.length()), &result[0], nLen, NULL, NULL);
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// For decoding data in UTF-8.
//string UTF8Decode(const string& svInput)
//{
//    struct destructible_codecvt : public std::codecvt<char32_t, char, std::mbstate_t>
//    {
//        using std::codecvt<char32_t, char, std::mbstate_t>::codecvt;
//        ~destructible_codecvt() = default;
//    };
//    std::wstring_convert<destructible_codecvt, char32_t> utf32_converter;
//    return utf32_converter.from_bytes(svInput);
//    return "";
//}

///////////////////////////////////////////////////////////////////////////////
// For checking if a string is a number.
bool StringIsDigit(const string& svInput)
{
    for (char const& c : svInput)
    {
        if (std::isdigit(c) == 0)
        {
            return false;
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// For comparing input strings alphabetically.
bool CompareStringAlphabetically(const string& svA, const string& svB)
{
    int i = 0;

    while (svA[i] != '\0' && svA[i] == svB[i])
    {
        i++;
    }

    return (svA[i] - svB[i]) < 0;
}

///////////////////////////////////////////////////////////////////////////////
// For comparing input strings lexicographically.
bool CompareStringLexicographically(const string& svA, const string& svB)
{
    return svA < svB;
}

///////////////////////////////////////////////////////////////////////////////
// For replacing parts of a given string by reference.
bool StringReplace(string& svInput, const string& svFrom, const string& svTo)
{
    const string::size_type nPos = svInput.find(svFrom);

    if (nPos == string::npos)
    {
        return false;
    }

    svInput.replace(nPos, svFrom.length(), svTo);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// For replacing parts of a given string by value.
string StringReplaceC(const string& svInput, const string& svFrom, const string& svTo)
{
    const string::size_type nPos = svInput.find(svFrom);

    if (nPos == string::npos)
    {
        return svInput;
    }

    string result = svInput;
    result.replace(nPos, svFrom.length(), svTo);

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// For escaping special characters in a string.
string StringEscape(const string& svInput)
{
    string result;
    result.reserve(svInput.size());

    for (const char c : svInput)
    {
        switch (c)
        {
        //case '\'':  result += "\\'";  break;
        case '\a':  result += "\\a";  break;
        case '\b':  result += "\\b";  break;
        case '\f':  result += "\\f";  break;
        case '\n':  result += "\\n";  break;
        case '\r':  result += "\\r";  break;
        case '\t':  result += "\\t";  break;
        case '\v':  result += "\\v";  break;
        default:    result += c;      break;
        }
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// For un-escaping special characters in a string.
string StringUnescape(const string& svInput)
{
    string result;
    result.reserve(svInput.size());

    bool escaped = false;

    for (const char c : svInput)
    {
        if (escaped)
        {
            switch (c)
            {
            case 'a':  result += '\a';  break;
            case 'b':  result += '\b';  break;
            case 'f':  result += '\f';  break;
            case 'n':  result += '\n';  break;
            case 'r':  result += '\r';  break;
            case 't':  result += '\t';  break;
            case 'v':  result += '\v';  break;
            case '\\': result += '\\';  break;
            default:   result += '\\'; result += c; break;
            }
            escaped = false;
        }
        else
        {
            if (c == '\\')
            {
                escaped = true;
            }
            else
            {
                result += c;
            }
        }
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// For counting the number of delimiters in a given string.
size_t StringCount(const string& svInput, const char cDelim)
{
    size_t result = 0;

    for (size_t i = 0; i < svInput.size(); i++)
    {
        if (svInput[i] == cDelim)
        {
            result++;
        }
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// For splitting a string into substrings by delimiter.
vector<string> StringSplit(string svInput, const char cDelim, const size_t nMax)
{
    string svSubString;
    vector<string> vSubStrings;

    svInput = svInput + cDelim;

    for (size_t i = 0; i < svInput.size(); i++)
    {
        if ((i != (svInput.size() - 1) && vSubStrings.size() >= nMax)
            || svInput[i] != cDelim)
        {
            svSubString += svInput[i];
        }
        else
        {
            if (svSubString.size() != 0)
            {
                vSubStrings.push_back(svSubString);
            }

            svSubString.clear();
        }
    }

    return vSubStrings;
}

///////////////////////////////////////////////////////////////////////////////
// For trimming leading characters from input.
string& StringLTrim(string& svInput, const char* const pszToTrim, const bool bTrimBefore)
{
    size_t n = 0;

    if (!bTrimBefore)
    {
        n = svInput.find_first_not_of(pszToTrim);
    }
    else // Remove everything before criteria as well.
    {
        n = svInput.find_first_of(pszToTrim);
        n = svInput.find_first_not_of(pszToTrim, n);
    }

    if (n != string::npos)
    {
        svInput.erase(0, n);
    }

    return svInput;
}

///////////////////////////////////////////////////////////////////////////////
// For trimming trailing characters from input.
string& StringRTrim(string& svInput, const char* const pszToTrim, const bool bTrimAfter)
{
    size_t n = 0;

    if (!bTrimAfter)
    {
        n = svInput.find_last_not_of(pszToTrim) + 1;
    }
    else // Remove everything after criteria as well.
    {
        n = svInput.find_first_of(pszToTrim) + 1;
    }

    if (n > 0)
    {
        svInput.erase(n);
        if (bTrimAfter)
        {
            svInput.at(svInput.size() - 1) = '\0';
        }
    }

    return svInput;
}

///////////////////////////////////////////////////////////////////////////////
// For trimming leading and trailing characters from input.
string& StringTrim(string& svInput, const char* const pszToTrim, const bool bTrimAll)
{
    return StringRTrim(StringLTrim(svInput, pszToTrim, bTrimAll), pszToTrim, bTrimAll);
}

///////////////////////////////////////////////////////////////////////////////
// For converting a string to an array of bytes.
vector<int> StringToBytes(const char* const szInput, const bool bNullTerminator)
{
    const char* const pszStringEnd = szInput + strlen(szInput);
    vector<int> vBytes;

    for (const char* pszCurrentByte = szInput; pszCurrentByte < pszStringEnd; ++pszCurrentByte)
    {
        // Dereference character and push back the byte.
        vBytes.push_back(*pszCurrentByte);
    }

    if (bNullTerminator)
    {
        vBytes.push_back('\0');
    }

    return vBytes;
};

///////////////////////////////////////////////////////////////////////////////
// For converting a string to an array of bytes.
pair<vector<uint8_t>, string> StringToMaskedBytes(const char* szInput, bool bNullTerminator)
{
    const char* pszStringStart = const_cast<char*>(szInput);
    const char* pszStringEnd = pszStringStart + strlen(szInput);
    vector<uint8_t> vBytes;
    string svMask;

    for (const char* pszCurrentByte = pszStringStart; pszCurrentByte < pszStringEnd; ++pszCurrentByte)
    {
        // Dereference character and push back the byte.
        vBytes.push_back(*pszCurrentByte);
        svMask += 'x';
    }

    if (bNullTerminator)
    {
        vBytes.push_back(0x0);
        svMask += 'x';
    }
    return make_pair(vBytes, svMask);
};

///////////////////////////////////////////////////////////////////////////////
// For converting a 32-bit integer into a 4-char ascii string
void FourCCToString(FourCCString_t& buf, const int n)
{
    buf[0] = (char)((n & 0x000000ff) >> 0);
    buf[1] = (char)((n & 0x0000ff00) >> 8);
    buf[2] = (char)((n & 0x00ff0000) >> 16);
    buf[3] = (char)((n & 0xff000000) >> 24);
    buf[4] = '\0';
};

///////////////////////////////////////////////////////////////////////////////
// For converting a string pattern with wildcards to an array of bytes.
vector<int> PatternToBytes(const char* szInput)
{
    const char* pszPatternStart = const_cast<char*>(szInput);
    const char* pszPatternEnd = pszPatternStart + strlen(szInput);
    vector<int> vBytes;

    for (const char* pszCurrentByte = pszPatternStart; pszCurrentByte < pszPatternEnd; ++pszCurrentByte)
    {
        if (*pszCurrentByte == '?')
        {
            ++pszCurrentByte;
            if (*pszCurrentByte == '?')
            {
                ++pszCurrentByte; // Skip double wildcard.
            }
            vBytes.push_back(-1); // Push the byte back as invalid.
        }
        else
        {
            vBytes.push_back(strtoul(pszCurrentByte, const_cast<char**>(&pszCurrentByte), 16));
        }
    }
    return vBytes;
};

///////////////////////////////////////////////////////////////////////////////
// For converting a string pattern with wildcards to an array of bytes and mask.
pair<vector<uint8_t>, string> PatternToMaskedBytes(const char* szInput)
{
    const char* pszPatternStart = const_cast<char*>(szInput);
    const char* pszPatternEnd = pszPatternStart + strlen(szInput);

    vector<uint8_t> vBytes;
    string svMask;

    for (const char* pszCurrentByte = pszPatternStart; pszCurrentByte < pszPatternEnd; ++pszCurrentByte)
    {
        if (*pszCurrentByte == '?')
        {
            ++pszCurrentByte;
            if (*pszCurrentByte == '?')
            {
                ++pszCurrentByte; // Skip double wildcard.
            }
            vBytes.push_back(0); // Push the byte back as invalid.
            svMask += '?';
        }
        else
        {
            vBytes.push_back(uint8_t(strtoul(pszCurrentByte, const_cast<char**>(&pszCurrentByte), 16)));
            svMask += 'x';
        }
    }
    return make_pair(vBytes, svMask);
};

///////////////////////////////////////////////////////////////////////////////
// For converting a integer into digits.
vector<int> IntToDigits(int iValue)
{
    vector<int> vDigits;

    for (; iValue > 0; iValue /= 10)
    {
        vDigits.push_back(iValue % 10);
    }

    std::reverse(vDigits.begin(), vDigits.end());
    return vDigits;
}

///////////////////////////////////////////////////////////////////////////////
// For printing __m128i data types.
void PrintM128i8(__m128i in)
{
    alignas(16) uint8_t v[16];
    _mm_store_si128(reinterpret_cast<__m128i*>(v), in);
    printf("v16_u8: %x %x %x %x | %x %x %x %x | %x %x %x %x | %x %x %x %x\n",
        v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7],
        v[8], v[9], v[10], v[11], v[12], v[13], v[14], v[15]);
}
void PrintM128i16(__m128i in)
{
    alignas(16) uint16_t v[8];
    _mm_store_si128(reinterpret_cast<__m128i*>(v), in);
    printf("v8_u16: %x %x | %x %x | %x %x | %x %x\n", v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]);
}
void PrintM128i32(__m128i in)
{
    alignas(16) uint32_t v[4];
    _mm_store_si128(reinterpret_cast<__m128i*>(v), in);
    printf("v4_u32: %x | %x | %x | %x\n", v[0], v[1], v[2], v[3]);
}
void PrintM128i64(__m128i in)
{
    alignas(16) uint64_t v[2];  // uint64_t might give format-string warnings with %llx; it's just long in some ABIs
    _mm_store_si128(reinterpret_cast<__m128i*>(v), in);
    printf("v2_u64: %llx %llx\n", v[0], v[1]);
}

///////////////////////////////////////////////////////////////////////////////
// For appending characters to a printf buffer.
void AppendPrintf(char* pBuffer, size_t nBufSize, char const* pFormat, ...)
{
    char scratch[1024];
    va_list argptr;
    va_start(argptr, pFormat);
    _vsnprintf(scratch, sizeof(scratch) - 1, pFormat, argptr);
    va_end(argptr);
    scratch[sizeof(scratch) - 1] = 0;

    strncat(pBuffer, scratch, nBufSize);
}

///////////////////////////////////////////////////////////////////////////////
// For escaping the '%' character for *rintf.
string PrintPercentageEscape(const string& svInput)
{
    string result;
    result.reserve(svInput.size());

    for (const char c : svInput)
    {
        switch (c)
        {
        case '%':  result += "%%";  break;
        default:   result += c;     break;
        }
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
// For formatting a STL string to a prettified representation of input bytes.
string FormatBytes(size_t nBytes)
{
    char szBuf[128];
    const char* szSuffix[] = { "B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB" };
    const int iBase = 1024;
    size_t c = nBytes ? (std::min)((size_t)(log((double)nBytes) / log((double)iBase)), (size_t)sizeof(szSuffix) - 1) : 0;
    snprintf(szBuf, sizeof(szBuf), "%1.2lf %s", nBytes / pow((double)iBase, c), szSuffix[c]);
    return string(szBuf);
}

///////////////////////////////////////////////////////////////////////////////
// For formatting a STL string using C-style format specifiers (va_list version).
string FormatV(const char* szFormat, va_list args)
{
    // Initialize use of the variable argument array.
    va_list argsCopy;
    va_copy(argsCopy, args);

    // Dry run to obtain required buffer size.
    const int iLen = std::vsnprintf(nullptr, 0, szFormat, argsCopy);
    va_end(argsCopy);

    assert(iLen >= 0);
    string result;

    if (iLen <= 0)
    {
        result.clear();
    }
    else
    {
        // NOTE: reserve enough buffer size for the string + the terminating
        // NULL character, then resize it to just the string len so we don't
        // count the NULL character in the string's size (i.e. when calling
        // string::size()).
        result.reserve(iLen+1);
        result.resize(iLen);

        std::vsnprintf(&result[0], iLen+1, szFormat, args);
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// For formatting a STL string using C-style format specifiers.
string Format(const char* szFormat, ...)
{
    string result;

    va_list args;
    va_start(args, szFormat);
    result = FormatV(szFormat, args);
    va_end(args);

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// For comparing two IPv6 addresses.
int CompareIPv6(const IN6_ADDR& ipA, const IN6_ADDR& ipB)
{
    // Return 0 if ipA == ipB, -1 if ipA < ipB and 1 if ipA > ipB.
    for (int i = 0; i < 16; ++i)
    {
        if (ipA.s6_addr[i] < ipB.s6_addr[i])
        {
            return -1;
        }
        else if (ipA.s6_addr[i] > ipB.s6_addr[i])
        {
            return 1;
        }
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// For obtaining the current timestamp.
uint64_t GetUnixTimeStamp()
{
    __time64_t time;
    _time64(&time);

    return time;
}

///////////////////////////////////////////////////////////////////////////////
// For obtaining a duration from a certain interval.
std::chrono::nanoseconds IntervalToDuration(const float flInterval)
{
    using namespace std::chrono;
    using fsec = duration<float>;
    return round<nanoseconds>(fsec{ flInterval });
}
