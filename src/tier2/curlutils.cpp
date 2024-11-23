//===========================================================================//
//
// Purpose: A set of utilities to perform requests
//
//===========================================================================//
#include "tier1/cvar.h"
#include "tier2/curlutils.h"

size_t CURLReadFileCallback(void* data, const size_t size, const size_t nmemb, FILE* stream)
{
    const size_t numBytesRead = fread(data, size, nmemb, stream);
    return numBytesRead;
}

size_t CURLWriteFileCallback(void* data, const size_t size, const size_t nmemb, FILE* stream)
{
    const size_t numBytesWritten = fwrite(data, size, nmemb, stream);
    return numBytesWritten;
}

size_t CURLWriteStringCallback(char* data, const size_t size, const size_t nmemb, string* userp)
{
    userp->append(data, size * nmemb);
    return size * nmemb;
}

void CURLInitCommonOptions(CURL* curl, const char* remote,
    const void* readData, const void* writeData,
    const CURLParams& params, const CURLProgress* progressData)
{
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, params.timeout);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, params.verbose);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, params.failOnError);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, params.followRedirect);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, params.verifyPeer);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_URL, remote);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, writeData);
    curl_easy_setopt(curl, CURLOPT_READDATA, readData);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "R5R HTTPS/1.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, params.writeFunction);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, params.readFunction);

    if (params.statusFunction)
    {
        Assert(progressData);

        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0l);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, progressData);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, params.statusFunction);
    }
}

static CURL* EasyInit()
{
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        Error(eDLL_T::COMMON, NO_ERROR, "CURL: %s\n", "Easy init failed");
    }

    return curl;
}

static FILE* OpenFile(const char* filePath, const char* options)
{
    FILE* file = fopen(filePath, options);
    if (!file)
    {
        Error(eDLL_T::COMMON, NO_ERROR, "CURL: %s\n", "Open file failed");
    }

    return file;
}

static bool FileStat(const char* fileName, struct _stat64& stat)
{
    if (_stat64(fileName, &stat) != 0)
    {
        Error(eDLL_T::COMMON, NO_ERROR, "CURL: %s\n", "File status query failed");
        return false;
    }

    return true;
}

curl_slist* CURLSlistAppend(curl_slist* slist, const char* string)
{
    slist = curl_slist_append(slist, string);
    if (!slist)
    {
        Error(eDLL_T::COMMON, NO_ERROR, "CURL: %s\n", "Slist append failed");
    }

    return slist;
}

bool CURLUploadFile(const char* remote, const char* filePath,
    const char* options, void* userData, const bool usePost,
    const curl_slist* slist, const CURLParams& params)
{
    CURL* curl = EasyInit();
    if (!curl)
    {
        return false;
    }

    FILE* file = OpenFile(filePath, options);
    if (!file)
    {
        return false;
    }

    struct _stat64 fileStatus;
    if (!FileStat(filePath, fileStatus))
    {
        fclose(file);
        return false;
    }

    CURLProgress progressData;

    progressData.curl = curl;
    progressData.name = filePath;
    progressData.user = userData;
    progressData.size = fileStatus.st_size;

    string response;
    CURLInitCommonOptions(curl, remote, file, &response, params, &progressData);

    if (slist)
    {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    }

    const bool largeFile = fileStatus.st_size > INT_MAX;
    CURLoption fileSizeOption;

    if (usePost)
    {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        fileSizeOption = largeFile
            ? CURLOPT_POSTFIELDSIZE_LARGE
            : CURLOPT_POSTFIELDSIZE;
    }
    else
    {
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        fileSizeOption = largeFile
            ? CURLOPT_INFILESIZE_LARGE
            : CURLOPT_INFILESIZE;
    }

    curl_easy_setopt(curl, fileSizeOption, (curl_off_t)fileStatus.st_size);

    CURLcode res = curl_easy_perform(curl);
    const bool success = res == CURLE_OK;

    if (!success)
    {
        Error(eDLL_T::COMMON, NO_ERROR, "CURL: Upload of file '%s' failed; %s\n",
            filePath, curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
    fclose(file);

    return success;
}

bool CURLDownloadFile(const char* remote, const char* savePath, const char* fileName,
    const char* options, curl_off_t dataSize, void* userData, const CURLParams& params, CUtlString* const pErrorMessage)
{
    CURL* curl = EasyInit();
    if (!curl)
    {
        return false;
    }

    string filePath = savePath;

    AppendSlash(filePath);
    filePath.append(fileName);

    FILE* file = OpenFile(filePath.c_str(), options);
    if (!file)
    {
        curl_easy_cleanup(curl);
        return false;
    }

    CURLProgress progressData;

    progressData.curl = curl;
    progressData.name = fileName;
    progressData.user = userData;
    progressData.size = dataSize;

    CURLInitCommonOptions(curl, remote, nullptr, file, params, &progressData);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {   
        if (pErrorMessage)
            pErrorMessage->Format("CURL: Download of file '%s' failed; %s", fileName, curl_easy_strerror(res));

        Error(eDLL_T::COMMON, NO_ERROR, "CURL: Download of file '%s' failed; %s\n",
            fileName, curl_easy_strerror(res));

        curl_easy_cleanup(curl);
        fclose(file);

        return false;
    }

    curl_easy_cleanup(curl);
    fclose(file);

    return true;
}

CURL* CURLInitRequest(const char* remote, const char* request,
    string& outResponse, curl_slist*& slist, const CURLParams& params)
{ 
    slist = CURLSlistAppend(slist, "Content-Type: application/json");
    if (!slist)
    {
        return nullptr;
    }

    CURL* curl = EasyInit();
    if (!curl)
    {
        return nullptr;
    }

    CURLInitCommonOptions(curl, remote, nullptr, &outResponse, params, nullptr);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    if (request)
    {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);
    }

    return curl;
}

CURLcode CURLSubmitRequest(CURL* curl, curl_slist*& slist)
{
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(slist);

    return res;
}

CURLINFO CURLRetrieveInfo(CURL* curl)
{
    CURLINFO status;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    curl_easy_cleanup(curl);

    return status;
}

bool CURLHandleError(CURL* curl, const CURLcode res, string& outMessage, const bool logError)
{
    if (res == CURLE_OK)
        return true;

    const char* curlError = curl_easy_strerror(res);

    if (logError)
        Error(eDLL_T::COMMON, NO_ERROR, "CURL: %s\n", curlError);

    outMessage = curlError;
    curl_easy_cleanup(curl);

    return false;
}

void CURLFormatUrl(string& outUrl, const char* host, const char* api)
{
    outUrl = Format("%s%s%s", "https://", host, api);
}
