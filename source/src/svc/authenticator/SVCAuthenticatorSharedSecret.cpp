#include "SVCAuthenticatorSharedSecret.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

const string SVCAuthenticatorSharedSecret::NULL_STRING = "";

SVCAuthenticatorSharedSecret::SVCAuthenticatorSharedSecret(string secretPath){	
	//-- read shared secret in hex string format
	uint8_t sharedKey[KEY_LENGTH] = "";
	ifstream key(secretPath);
	stringstream buffer;
	buffer << key.rdbuf();
	
	//printf("\nread shared secret: %s", buffer.str().c_str());
	
	//-- convert form hex to binary form	
	stringToHex(buffer.str().c_str(), sharedKey);
	
	//-- create aesgcm and hash instances
	this->aesGCM = new AESGCM(sharedKey, SECU_128);
	this->sha256 = new SHA256();
}

SVCAuthenticatorSharedSecret::~SVCAuthenticatorSharedSecret(){
	delete this->aesGCM;
	delete this->sha256;
}

string SVCAuthenticatorSharedSecret::generateChallenge(const string& challengeSecret){
	string rs;
	//-- random iv string
	uint16_t ivLen = KEY_LENGTH;
	uint8_t iv[ivLen] = "";
	generateRandomData(ivLen, iv);
	//-- encrypt this string with aesgcm, shared key
	uint32_t encryptedLen;
	uint8_t* encrypted;
	uint8_t* tag;
	uint16_t tagLen;

	this->aesGCM->encrypt(iv, ivLen, (uint8_t*)challengeSecret.c_str(), challengeSecret.size(), NULL, 0, &encrypted, &encryptedLen, &tag, &tagLen);
	uint32_t challengeLen = 8 + encryptedLen + ivLen + tagLen;
	uint8_t* challengeBuf = (uint8_t*)malloc(challengeLen);
	
	uint8_t* p = challengeBuf;
	memcpy(p, &encryptedLen, 4);
	p+=4;
	memcpy(p, encrypted, encryptedLen);	
	p+=encryptedLen;
	
	memcpy(p, &ivLen, 2);
	p+=2;
	memcpy(p, iv, ivLen);
	p+=ivLen;
	
	memcpy(p, &tagLen, 2);
	p+=2;
	memcpy(p, tag, tagLen);
	rs = hexToString(challengeBuf, challengeLen);
	
	//-- clear then return
	free(encrypted);
	free(tag);
	free(challengeBuf);
	return rs;
}

string SVCAuthenticatorSharedSecret::resolveChallenge(const std::string& challenge){
	string rs;
	
	uint8_t* challengeBuf = (uint8_t*)malloc(65535);
	uint32_t challengeLen = stringToHex(challenge, challengeBuf);
	
	uint8_t* iv;
	uint16_t ivLen;
	
	uint8_t* encrypted;
	uint8_t* tag;
	uint16_t tagLen;
	uint8_t* p = challengeBuf;
	uint8_t* challengeSecret;
	uint32_t challengeSecretLen;
	
	if (challengeLen>0){		
		encrypted = p+4;
		uint32_t encryptedLen = *((uint32_t*)p);
		p += 4 + encryptedLen;
		
		iv = p+2;
		ivLen = *((uint16_t*)p);
		p += 2 + ivLen;
		
		tag = p+2;
		tagLen = *((uint16_t*)p);
				
		if (this->aesGCM->decrypt(iv, ivLen, encrypted, encryptedLen, NULL, 0, tag, tagLen, &challengeSecret, &challengeSecretLen)){
			rs = string((char*)challengeSecret, challengeSecretLen);
			free(challengeSecret);
		}
		else{			
			rs = NULL_STRING;
		}		
	}
	else{
		rs = NULL_STRING;
	}	
	free(challengeBuf);
	return rs;
}

string SVCAuthenticatorSharedSecret::generateProof(const string& challengeSecret){
	//-- hash the solution HASH_TIME times
	string rs;
	if (challengeSecret.size()>0){		
		for (int i=0; i<HASH_TIME; i++){
			rs = this->sha256->hash(challengeSecret);
		}
	}
	else{
		rs = NULL_STRING;
	}
	return rs;
}

bool SVCAuthenticatorSharedSecret::verifyProof(const string& challengeSecret, const string& proof){
	string comparison;
	if (proof.size()>0){
		for (int i=0; i<HASH_TIME; i++){
			comparison = this->sha256->hash(challengeSecret);
		}
		return (comparison == proof);
	}
	else{
		return false;
	}
}

