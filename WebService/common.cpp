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