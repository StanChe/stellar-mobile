#include "StellarMobile.h"
#include <iostream>

int main(char* args)
{
	std::locale::global(std::locale(""));
	//std::string res = stellar_mobile::StellarMobile::doCommand("{\"method\":\"account_tx\",\"params\":[{\"account\":\"ganVp9o5emfzpwrG5QVUXqMv8AgLcdvySb\",\"limit\":4}]}");
	std::string res = stellar_mobile::StellarMobile::doCommand("{\"method\":\"submit\",\"params\":[{\"tx_blob\": \"1200002280000000243ADE68B16140000000000F424068400000000000000A7320BE3900393891A2A2244E28A82C43BA94CA94DD6BFE36D523576A22BFF86055D4744052F5A5A7342F5E1254CFB29FE7DFAC36D12E0274CBC1569B35B8C9FC251996DA6E78BB84CCF91299CF9C60A2F95761F034DCA58B1C3C465B23754E89520DEA0C811437B1B26BE3C91C55D51586C3F0E5C4B03E9CEA7F8314DF8286CDBB009AA5C29F526B5C3B4C480B44EABE\"    }  ]}");
	std::cout << res;
	int r;
	std::cin >> r;
}