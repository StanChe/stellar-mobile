#include "CurlReader.h"

#include "curlReader.h"
#include <string>
#include <curl/curl.h> 
namespace stellar_mobile {

	std::string CurlReader::Data;
	std::mutex CurlReader::mtx;

	size_t CurlReader::writeCallback(char* buf, size_t size, size_t nmemb, void* up)
	{ //callback must have this declaration
		//buf is a pointer to the data that curl has for us
		//size*nmemb is the size of the buffer
		for (unsigned int c = 0; c<size*nmemb; c++)
		{
			Data.push_back(buf[c]);
		}
		return size*nmemb; //tell curl how many bytes we handled
	}
	std::string CurlReader::performRequest(std::string url, std::string params) 
	{
		CURL *curl;
		curl_global_init(CURL_GLOBAL_DEFAULT);
		curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
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