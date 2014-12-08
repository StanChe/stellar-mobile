#include <curl/curl.h>

#include "../StellarMobile.h"
#include "../rapidjson/include/rapidjson/writer.h"
#include "../rapidjson/include/rapidjson/stringbuffer.h"
#include "../Utils/ErrorCategory.h"
#include "../Utils/Helper.h";
#include "../Utils/JSS.h"
#include "../Utils/CurlReader.h"

#include <iostream>
#include "../Utils/Config.h"

namespace stellar_mobile
{
	StellarMobile::FuncContainer StellarMobile::Container;
	 void StellarMobile::doAccountTx(rapidjson::Document& request, rapidjson::Value& result, rapidjson::MemoryPoolAllocator<>& allocator)
	{
		if (!request.HasMember(JSS::PARAMS))
		{
			std::string errorParams[1] = { JSS::PARAMS };
			ErrorsContainer::Errors.get(INVALID_JSON, errorParams, 1).InjectTo(result, allocator);
			return;
		}
		rapidjson::Value& params = request[JSS::PARAMS];
		if (params.HasMember(JSS::PARAM_ACCOUNT))
		{
			std::string errorParams [1]= {JSS::PARAM_ACCOUNT};
			ErrorsContainer::Errors.get(INVALID_JSON, errorParams, 1).InjectTo(result, allocator);
			return;
		}
		Helper::redirectRequest(request, result, allocator);
	}

	

	void StellarMobile::doCommand(std::string& rawParams, rapidjson::Value& result, rapidjson::MemoryPoolAllocator<>& allocator)
	{
		rapidjson::Document params;
		if (params.Parse<0>(rawParams.c_str()).HasParseError())
		{
			ErrorsContainer::Errors.get(UNPARSABLE_RESPONSE_JSON).InjectTo(result, allocator);
			return;
		}
		if (!params.HasMember(JSS::METHOD_PAR))
		{
			std::string errorParams [1]= {JSS::METHOD_PAR};
			ErrorsContainer::Errors.get(INVALID_JSON, errorParams, 1).InjectTo(result, allocator);
			return;
		}
		rapidjson::Value& method = params[JSS::METHOD_PAR];
		if (!method.IsString())
		{
			std::string errorParams [1]= {JSS::METHOD_PAR};
			ErrorsContainer::Errors.get(INVALID_PARAM_TYPE, errorParams, 1).InjectTo(result, allocator);
			return;
		}
		Container.get(method.GetString())(params, result, allocator);
	}

	

	std::string StellarMobile::doCommand(std::string rawParams)
	{
		rapidjson::Document response;
		response.SetObject();
		rapidjson::Value result;
		rapidjson::MemoryPoolAllocator<>& allocator = response.GetAllocator();
		result.SetObject();
		try
		{
			doCommand(rawParams, result, allocator);
		}
		catch(MethodNotFound ex)
		{
			ErrorsContainer::Errors.get(UNKNOWN_METHOD).InjectTo(result, allocator);
		}
		catch(_exception ex) {
			ErrorsContainer::Errors.get(UNKNOWN).InjectTo(result, allocator);
		}
		Helper::AddMember(response, JSS::RESULT_FIELD, result, allocator);
		result.SetObject();
		if (result.HasMember(JSS::ERROR_NAME))
			response.AddMember("status", "error", allocator);
		return Helper::AsString(response);
		
	}

	StellarMobile::FuncContainer::FuncContainer()
	{
		add("account_tx", &StellarMobile::doAccountTx);
		add("submit", &StellarMobile::doSubmit);
	}

	StellarMobile::doFuncPtr const& StellarMobile::FuncContainer::get(std::string const& name) const
	{
		Map::const_iterator const iter (map.find (name));
		if (iter != map.end())
			return iter->second;
		throw MethodNotFound();
	}

	void StellarMobile::FuncContainer::add(std::string const& name, doFuncPtr func)
	{
		std::pair <Map::iterator, bool> result (
			map.emplace (std::piecewise_construct,
				std::forward_as_tuple (name), std::forward_as_tuple (func)));
		if (! result.second)
			throw std::invalid_argument ("duplicate error code");
	}
}