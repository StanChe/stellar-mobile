#include "curlReader.h"
#include <string>
#include <curl/curl.h> 
#include <mutex>
using namespace std;
namespace beast {

	string Data;
	std::mutex mtx;
	size_t writeCallback(char* buf, size_t size, size_t nmemb, void* up)
	{ //callback must have this declaration
		//buf is a pointer to the data that curl has for us
		//size*nmemb is the size of the buffer
		for (unsigned int c = 0; c<size*nmemb; c++)
		{
			Data.push_back(buf[c]);
		}
		return size*nmemb; //tell curl how many bytes we handled
	}
	string CurlReader::performRequest(string url, string params) 
	{
		CURL *curl;
		curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params.c_str());
		/* Perform the request, res will get the return code */
		mtx.lock();
		Data = "";
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L); //tell curl to output its progress
		curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		std::string result = Data;
		mtx.unlock();
		return result;
	}
}