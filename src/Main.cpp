#include "StellarMobile.h"
#include "rapidjson\include\rapidjson\document.h"
#include "Utils\Helper.h"
#include <iostream>

int main(char* args)
{
	std::locale::global(std::locale(""));
	//std::string res = stellar_mobile::StellarMobile::doCommand("{\"method\":\"account_tx\",\"params\":[{\"account\":\"ganVp9o5emfzpwrG5QVUXqMv8AgLcdvySb\",\"limit\":4}]}");
	std::string res = stellar_mobile::StellarMobile::doCommand("{  \"method\": \"submit\",  \"params\": [    {      \"secret\": \"sfwtwgV3zHekZMm6F2cNPzEGzogQqPMEZcdVftKnrstngZvotYr\",      \"tx_json\": {        \"TransactionType\": \"Payment\",        \"Account\": \"gM4Fpv2QuHY4knJsQyYGKEHFGw3eMBwc1U\",        \"Destination\": \"g4eRqgZfzfj3132y17iaf2fp6HQj1gofjt\",        \"Amount\": 5      }    }  ]}");
	rapidjson::Document d;
	d.Parse(res.c_str());
	std::string blob = stellar_mobile::Helper::AsString(d["result"]["tx_blob"]);
	std::string toSubmit = "{  \"method\": \"submit\",  \"params\": [    {      \"tx_blob\": " + blob + "    }  ]}";
	std::string submtion = stellar_mobile::StellarMobile::doCommand(toSubmit);
	std::cout << submtion;
	int r;
	std::cin >> r;
}