#ifndef STELLAR_MOBILE_STELLAR_MOBILE_H
#define STELLAR_MOBILE_STELLAR_MOBILE_H
#include <string>
#include "rapidjson/include/rapidjson/document.h"
#include <map>

namespace stellar_mobile
{
	class StellarMobile
{
private :
		static void doAccountTx(rapidjson::Document& params, rapidjson::Value& result, rapidjson::MemoryPoolAllocator<>& allocator);
		static void doSubmit(rapidjson::Document& params, rapidjson::Value& result, rapidjson::MemoryPoolAllocator<>& allocator);
		static void doTransactionSign(rapidjson::Document& params, rapidjson::Value& result, rapidjson::MemoryPoolAllocator<>& allocator);
		static void doCommand(std::string& rawParams, rapidjson::Value& result, rapidjson::MemoryPoolAllocator<>& allocator);
		typedef void (*doFuncPtr) (rapidjson::Document& params, rapidjson::Value& result, rapidjson::MemoryPoolAllocator<>& allocator);
		class FuncContainer
		{
			public:
				FuncContainer();
				doFuncPtr const& get (std::string const& name) const;
			private:
				typedef std::map<std::string, doFuncPtr> Map;
				Map map;
				void add (std::string const& name, doFuncPtr func);
		};
		class MethodNotFound : std::invalid_argument{
		public :
			explicit MethodNotFound() : invalid_argument("Method not found;"){}

		};
		static FuncContainer Container;
public:
		static std::string doCommand(std::string params);
	};
}
#endif
