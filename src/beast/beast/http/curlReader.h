#ifndef BEAST_HTTP_CURLREADER_H_INCLUDED
#define BEAST_HTTP_CURLREADER_H_INCLUDED

namespace beast {
	class CurlReader {
	public:
		std::string performRequest(std::string url, std::string params);
	};
}
#endif