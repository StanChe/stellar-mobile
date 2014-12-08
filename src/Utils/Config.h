#ifndef STELLAR_MOBILE_CONFIG_H
#define STELLAR_MOBILE_CONFIG_H
#include <string>
#include <map>
#include <vector>

namespace stellar_mobile
{
	class Config
	{
	public:
		class Data
		{
			typedef void (*setConfParam) (std::vector<std::string>& data);
			typedef std::map<std::string, setConfParam> Map;
			static setConfParam const& get (std::string const& name);
			static void initConfSetters();
			static void add (std::string const& name, setConfParam func);
			static void setStellardAddress(std::vector<std::string>& data);
			static Map map;
		public:
			Data();
			static std::string StellardAddress;
			static const char* CONFIG_FILE_NAME;
		};
		static Data DataContainer;
	};
}
#endif
