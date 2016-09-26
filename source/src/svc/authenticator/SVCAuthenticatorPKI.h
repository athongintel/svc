#ifndef __SVC_AUTHENTICATOR_PKI__
#define __SVC_AUTHENTICATOR_PKI__

	#include "SVCAuthenticator.h"

	class SVCAuthenticatorPKI : SVCAuthenticator{

		public:
			SVCAuthenticatorPKI(string caPath, string certpath, string keyPath);
			virtual ~SVCAuthenticatorPKI();
			
			virtual bool verify(std::string randomSecret, std::string challenge, std::string proof);
			virtual std::string generateRandomSecret();
			virtual std::string generateChallenge(std::string randomSecret);
			virtual std::string resolveChallenge(string challenge);
			virtual std::string generateProof(std::string solution);
	}

#endif
