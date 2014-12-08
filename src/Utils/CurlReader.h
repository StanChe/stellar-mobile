#ifndef STELLAR_CURL_READER_H
#define STELLAR_CURL_READER_H

#include <string>
#include <mutex>

namespace stellar_mobile {
	class CurlReader {
	public:
		std::string performRequest(std::string url, std::string params);
	private:
		static std::string Data;
		static std::mutex mtx;
		static size_t CurlReader::writeCallback(char* buf, size_t size, size_t nmemb, void* up);
	};
}

#endif
