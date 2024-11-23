#ifndef TIER2_CURLUTILS_H
#define TIER2_CURLUTILS_H

struct CURLProgress
{
	CURLProgress()
		: curl(nullptr)
		, name(nullptr)
		, user(nullptr)
		, size(0)
	{}

	CURL* curl;
	const char* name;
	void* user; // custom pointer to anything.
	size_t size;
};

struct CURLParams
{
	CURLParams()
		: readFunction(nullptr)
		, writeFunction(nullptr)
		, statusFunction(nullptr)
		, timeout(0)
		, verifyPeer(false)
		, followRedirect(false)
		, verbose(false)
		, failOnError(false)
	{}

	void* readFunction;
	void* writeFunction;
	void* statusFunction;

	int timeout;
	bool verifyPeer;
	bool followRedirect;
	bool verbose;
	bool failOnError;
};

size_t CURLReadFileCallback(void* data, const size_t size, const size_t nmemb, FILE* stream);
size_t CURLWriteFileCallback(void* data, const size_t size, const size_t nmemb, FILE* stream);
size_t CURLWriteStringCallback(char* contents, const size_t size, const size_t nmemb, string* userp);

curl_slist* CURLSlistAppend(curl_slist* slist, const char* string);

bool CURLUploadFile(const char* remote, const char* filePath, const char* options,
	void* userData, const bool usePost, const curl_slist* slist, const CURLParams& params);
bool CURLDownloadFile(const char* remote, const char* savePath, const char* fileName,
	const char* options, curl_off_t dataSize, void* userData, const CURLParams& params, CUtlString* const pErrorMsg = nullptr);

CURL* CURLInitRequest(const char* remote, const char* request, string& outResponse,
	curl_slist*& slist, const CURLParams& params);

CURLcode CURLSubmitRequest(CURL* curl, curl_slist*& slist);
CURLINFO CURLRetrieveInfo(CURL* curl);

bool CURLHandleError(CURL* curl, const CURLcode res, string& outMessage, const bool logError);
void CURLFormatUrl(string& outUrl, const char* host, const char* api);

#endif // !TIER2_CURLUTILS_H