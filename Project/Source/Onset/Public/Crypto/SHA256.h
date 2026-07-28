#ifndef SHA256_H
#define SHA256_H

#include "CoreTypes.h"
#include "CoreFwd.h"
#include "GenericPlatform/GenericPlatformMisc.h"

class FSHA256
{
protected:
	const static uint32 SHA256_K[];
	static constexpr unsigned int SHA224_256_Block_Size = (512/8);
public:
	void Init();
	void Update(const unsigned char* Message, uint32 Length);
	void Final(unsigned char *Digest);
	static constexpr unsigned int Digest_Size = (256 / 8);
	static bool GetSHA256Signature(const void* Data, uint32 ByteSize, FSHA256Signature& OutSignature);
 
protected:
	void Transform(const unsigned char *Message, unsigned int Block_Nb);
	unsigned int Tot_Len = 0;
	unsigned int Len = 0;
	unsigned char Block[2 * SHA224_256_Block_Size] = {};
	uint32 H[8] = {};
};
 
FString SHA256(const FString& Input);
 
#define SHA2_SHFR(x, n)    (x >> n)
#define SHA2_ROTR(x, n)   ((x >> n) | (x << ((sizeof(x) << 3) - n)))
#define SHA2_ROTL(x, n)   ((x << n) | (x >> ((sizeof(x) << 3) - n)))
#define SHA2_CH(x, y, z)  ((x & y) ^ (~x & z))
#define SHA2_MAJ(x, y, z) ((x & y) ^ (x & z) ^ (y & z))
#define SHA256_F1(x) (SHA2_ROTR(x,  2) ^ SHA2_ROTR(x, 13) ^ SHA2_ROTR(x, 22))
#define SHA256_F2(x) (SHA2_ROTR(x,  6) ^ SHA2_ROTR(x, 11) ^ SHA2_ROTR(x, 25))
#define SHA256_F3(x) (SHA2_ROTR(x,  7) ^ SHA2_ROTR(x, 18) ^ SHA2_SHFR(x,  3))
#define SHA256_F4(x) (SHA2_ROTR(x, 17) ^ SHA2_ROTR(x, 19) ^ SHA2_SHFR(x, 10))
#define SHA2_UNPACK32(x, str)                 \
{                                             \
*((str) + 3) = (uint8) ((x)      );       \
*((str) + 2) = (uint8) ((x) >>  8);       \
*((str) + 1) = (uint8) ((x) >> 16);       \
*((str) + 0) = (uint8) ((x) >> 24);       \
}
#define SHA2_PACK32(str, x)                   \
{                                             \
*(x) =   ((uint32) *((str) + 3)      )    \
| ((uint32) *((str) + 2) <<  8)    \
| ((uint32) *((str) + 1) << 16)    \
| ((uint32) *((str) + 0) << 24);   \
}
#endif