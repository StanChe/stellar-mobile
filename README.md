#stellard moblie - Stellar mobile library

This is the repository for Stellar's `stellard_mobile`, reference mobile library.

###External dependencies:
Your system should have libcurl4 and all development libraries needed for Stellard installed. Be sure to install libcurl (OpenSSL).
To install on Ubuntu run:
```
	sudo apt-get install libcurl4-openssl-dev
```
###Configuration
New field «stellard» was added to default stellard configuration file (All request to ledgers will be redirected to this address).
###Running
It is recommended to run «stellard for mobile» in standalone mode (command-line argument -a)
###How it works
1. Commands like «account_tx», which does not need secret key as argument, are redirected to address specified in «stellard» configuration field.
2. «Payment», «TrustSet», «OfferCreate», «OfferCancel» commands are performed locally (secret key is not sent to «stellard»), but all operations, like getting sequence or building path for payment, are redirected to «stellard».
3. «Submit» sends tx_blob to address specified in «stellard» configuration field. 

###For more information:
* https://www.stellar.org
