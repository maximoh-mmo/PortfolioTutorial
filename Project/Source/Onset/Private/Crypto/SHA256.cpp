#include "Crypto/SHA256.h"
#include <CoreMinimal.h>
#include "HAL/UnrealMemory.h"

const unsigned int FSHA256::SHA256_K[64] = //UL = uint32
            {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
             0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
             0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
             0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
             0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
             0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
             0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
             0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
             0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
             0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
             0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
             0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
             0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
             0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
             0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
             0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

bool FSHA256::GetSHA256Signature(const void* Data, uint32 ByteSize, FSHA256Signature& OutSignature)
{
    FMemory::Memzero(OutSignature.Signature);
    
    if (!Data || ByteSize == 0)
    {
        return false;
    }
    
    FSHA256 Hasher;
    Hasher.Init();
    Hasher.Update(static_cast<const unsigned char*>(Data), ByteSize);
    Hasher.Final(OutSignature.Signature);
    
    return true;
}

void FSHA256::Transform(const unsigned char *Message, unsigned int Block_Nb)
{
    uint32 w[64];
    uint32 wv[8];
    uint32 t1, t2;
    const unsigned char *Sub_Block;
    int i;
    int j;
    for (i = 0; i < static_cast<int>(Block_Nb); i++) {
        Sub_Block = Message + (i << 6);
        for (j = 0; j < 16; j++) {
            SHA2_PACK32(&Sub_Block[j << 2], &w[j]);
        }
        for (j = 16; j < 64; j++) {
            w[j] =  SHA256_F4(w[j -  2]) + w[j -  7] + SHA256_F3(w[j - 15]) + w[j - 16];
        }
        for (j = 0; j < 8; j++) {
            wv[j] = H[j];
        }
        for (j = 0; j < 64; j++) {
            t1 = wv[7] + SHA256_F2(wv[4]) + SHA2_CH(wv[4], wv[5], wv[6])
                + SHA256_K[j] + w[j];
            t2 = SHA256_F1(wv[0]) + SHA2_MAJ(wv[0], wv[1], wv[2]);
            wv[7] = wv[6];
            wv[6] = wv[5];
            wv[5] = wv[4];
            wv[4] = wv[3] + t1;
            wv[3] = wv[2];
            wv[2] = wv[1];
            wv[1] = wv[0];
            wv[0] = t1 + t2;
        }
        for (j = 0; j < 8; j++) {
            H[j] += wv[j];
        }
    }
}
 
void FSHA256::Init()
{
    H[0] = 0x6a09e667;
    H[1] = 0xbb67ae85;
    H[2] = 0x3c6ef372;
    H[3] = 0xa54ff53a;
    H[4] = 0x510e527f;
    H[5] = 0x9b05688c;
    H[6] = 0x1f83d9ab;
    H[7] = 0x5be0cd19;
    Len = 0;
    Tot_Len = 0;
}

void FSHA256::Update(const unsigned char* Message, uint32 Length)
{
    const unsigned char *Shifted_Message;
    const unsigned int Tmp_Len = SHA224_256_Block_Size - Len;
    unsigned int Rem_Len = Length < Tmp_Len ? Length : Tmp_Len;
    memcpy(&Block[Len], Message, Rem_Len);
    if (Len + Length < SHA224_256_Block_Size) {
        Len += Length;
        return;
    }
    const unsigned int New_Len = Length - Rem_Len;
    const unsigned int Block_Nb = New_Len / SHA224_256_Block_Size;
    Shifted_Message = Message + Rem_Len;
    Transform(Block, 1);
    Transform(Shifted_Message, Block_Nb);
    Rem_Len = New_Len % SHA224_256_Block_Size;
    memcpy(Block, &Shifted_Message[Block_Nb << 6], Rem_Len);
    Len = Rem_Len;
    Tot_Len += (Block_Nb + 1) << 6;
}
 
void FSHA256::Final(unsigned char *Digest)
{
    int i;
    const unsigned int Block_Nb = (1 + ((SHA224_256_Block_Size - 9)
        < (Len % SHA224_256_Block_Size)));
    const unsigned int Len_B = (Tot_Len + Len) << 3;
    const unsigned int PM_Len = Block_Nb << 6;
    memset(Block + Len, 0, PM_Len - Len);
    Block[Len] = 0x80;
    SHA2_UNPACK32(Len_B, Block + PM_Len - 4);
    Transform(Block, Block_Nb);
    for (i = 0 ; i < 8; i++) {
        SHA2_UNPACK32(H[i], &Digest[i << 2]);
    }
}

FString SHA256(const FString& Input)
{
    unsigned char Digest[FSHA256::Digest_Size] = {};

    FSHA256 Ctx = FSHA256();
    Ctx.Init();
    Ctx.Update(reinterpret_cast<const unsigned char*>(*Input), Input.Len());
    Ctx.Final(Digest);
 
    char buf[2*FSHA256::Digest_Size+1];
    buf[2*FSHA256::Digest_Size] = 0;
    for (int i = 0; i < FSHA256::Digest_Size; i++)
        sprintf(buf+i*2, "%02x", Digest[i]);
    return FString(buf);
}
