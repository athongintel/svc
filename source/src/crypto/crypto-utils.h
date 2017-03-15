#ifndef __TOM_CRYPTO_UTILS__
#define __TOM_CRYPTO_UTILS__
	
	
		#include <fcntl.h>		//-- For O_RDWR
		#include <unistd.h>		//-- For open(), creat()
		#include <gmp.h>	
		#include <string>		
		#include <cstring>		//-- for 'memcpy'

	namespace crypto{	
		#define BIT(x) 0x01<<x	
		#define GET_BE32(a) ((((uint32_t) (a)[0]) << 24) | (((uint32_t) (a)[1]) << 16) | (((uint32_t) (a)[2]) << 8) | ((uint32_t) (a)[3]))
		#define PUT_BE32(a, val) do {                          \
					(a)[0] = (uint8_t) ((((uint32_t) (val)) >> 24) & 0xff);   \
					(a)[1] = (uint8_t) ((((uint32_t) (val)) >> 16) & 0xff);   \
					(a)[2] = (uint8_t) ((((uint32_t) (val)) >> 8) & 0xff);    \
					(a)[3] = (uint8_t) (((uint32_t) (val)) & 0xff);           \
			} while (0)

		extern void generateRandomData(uint32_t length, uint8_t* data);
		extern void generateRandomNumber(mpz_t* number, int securityParam);
	}
#endif // UTILS_H
