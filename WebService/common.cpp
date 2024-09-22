#include "common.h"
#include <Windows.h>
#include <Shlwapi.h>


// ×Ö·û´®×ªBYTE 
unsigned char make_byte(const char* pbuf)
{
    unsigned char bret = 0;
    for (int i=0; i<2; i++)
    {
        if (pbuf[i]>='0' && pbuf[i]<='9')
        {
            bret = (bret<<4)|(pbuf[i] - '0');
        }
        else if ( (pbuf[i]|0x20)>='a' && (pbuf[i]|0x20)<='f' )
        {
            bret = (bret<<4)|((pbuf[i]|0x20)-'a'+10);
        }
    }
    return bret;
}

// ½âÂëURL±àÂë 
char* UrlDecode(const char* src, unsigned int srclen, char* dst, unsigned int dstlen)
{
    // ×ª»»%ºÅ 
    const char* p = src;
    char* q = dst;
    while(p < src+srclen)
    {
        if (*p == '%')
        {
            *q++ = make_byte(p+1);
            p += 3;
        }
        else if (*p == '/')
        {
            *q++ = '\\';
            p++;
        }
        else
        {
            *q++ = *p++;
        }
    }
    *q = '\0';

    // UTF-8 ×ª GBK 
    int uSize = MultiByteToWideChar(CP_UTF8, 0, dst, -1, NULL, 0);
    wchar_t* pwText = (wchar_t*)malloc((uSize+1)*sizeof(wchar_t));
    if (pwText)
    {
        MultiByteToWideChar(CP_UTF8, 0, dst, -1, pwText, uSize);
        WideCharToMultiByte(CP_ACP, 0, pwText, -1, dst, dstlen, NULL, NULL);
        free(pwText);
    }
    return dst;
}

// Êý¾Ý¼ÆËãsha1 
bool calcBufferSha1(unsigned char* buffer, unsigned long len, unsigned char out[20])
{
    bool retval = false;
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    DWORD error = 0;
    do
    {
        if( !CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) ) 
        {
            break;
        }
        CryptCreateHash(hProv, CALG_SHA1, NULL, 0, &hHash);
        DWORD ilen = 20;
        if( CryptHashData(hHash, buffer, len, 0) && 
            CryptGetHashParam(hHash, HP_HASHVAL, (PBYTE)out, &ilen, 0) )
        {
            retval = true;
        }
    }while(0);

    if(hHash) 
        CryptDestroyHash(hHash);
    if(hProv) 
        CryptReleaseContext(hProv, 0);
    return retval;
}

// ÅÐ¶ÏÊÇ·ñutf-8´® 
bool isUTF8(const void* data, unsigned int length) 
{
    if (data == NULL || length == 0)
    {
        return false;
    }

    int charByteCounter = 1;
    unsigned char* pData = (unsigned char*)data;

    for (int i = 0; i < length; i++) {
        byte currentByte = pData[i];

        if (charByteCounter == 1) {
            if ((currentByte & 0x80) == 0) {
                continue;
            }

            while (((currentByte << 1) & 0x80) != 0) {
                charByteCounter++;
                currentByte <<= 1;
            }

            if (charByteCounter < 2 || charByteCounter > 6) {
                return false;
            }
        } else {
            if ((currentByte & 0xC0) != 0x80) {
                return false;
            }
            charByteCounter--;
        }
    }

    if (charByteCounter > 1) {
        return false;
    }

    return true;
}