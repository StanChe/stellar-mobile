#include "Config.h"
#include "Helper.h"

#include <fstream>

namespace stellar_mobile 
{
	const char* Config::Data::CONFIG_FILE_NAME = "stellar-mobile.cfg";
	std::string Config::Data::StellardAddress;
	Config::Data::Map Config::Data::map;
	Config::Data Config::DataContainer = Data();
	Config::Data::Data()
	{
		initConfSetters();
		std::ifstream ifsConfig (CONFIG_FILE_NAME, std::ios::in);
		if (!ifsConfig || ifsConfig.fail())
			throw std::exception("Faild to load config file.");
        std::string strRawConfigFile;
        strRawConfigFile.assign ((std::istreambuf_iterator<char> (ifsConfig)),
                              std::istreambuf_iterator<char> ());
        if (ifsConfig.bad ())
            throw std::exception("Faild to read config file.");
        std::string strConfigFile = Helper::replace('\r', "", strRawConfigFile);
		std::vector<std::string> arrayConfigFile;
		Helper::split(strConfigFile, '\n', arrayConfigFile);
		std::vector<std::string> sectionValues;
		std::string sectionName;
		for (unsigned int i = 0; i < arrayConfigFile.size(); i++) 
		{
			std::string strValue = Helper::trim(arrayConfigFile[i]);
			if (strValue.empty () || strValue[0] == '#')
			{
			}
			else if (strValue[0] == '[' && strValue.length() > 2 && strValue[strValue.length () - 1] == ']')
			{
				if (!sectionName.empty())
				{
					setConfParam setter = get(sectionName);
					setter(sectionValues);
				}
				sectionValues.clear();
				std::string rawSectionName = strValue.substr(1, strValue.length() - 2);
				sectionName = Helper::toLower(rawSectionName);
			}
			else
			{
				sectionValues.push_back(strValue);
			}
		}
		if (!sectionName.empty())
				{
					setConfParam setter = get(sectionName);
					setter(sectionValues);
				}
}

	Config::Data::setConfParam const& Config::Data::get(std::string const& name)
	{
		Map::const_iterator const iter (map.find (name));
		if (iter != map.end())
			return iter->second;
		std::string m = "Failed to find setter for cnfig field " + name + ".";
		throw std::exception(m.c_str());
	}

	void Config::Data::initConfSetters()
	{
		add("stellard", &setStellardAddress);
	}

	void Config::Data::add(std::string const& name, setConfParam func)
	{
		std::pair <Map::iterator, bool> result (
			map.emplace (std::piecewise_construct,
				forward_as_tuple (name), forward_as_tuple (func)));
		if (! result.second)
			throw std::invalid_argument ("duplicate config field name");
	}

	void Config::Data::setStellardAddress(std::vector<std::string>& data)
	{
		StellardAddress = std::string(data[0]);
	}
}