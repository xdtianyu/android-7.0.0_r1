/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <printf.h>
#include <cpu/inc/cpuMath.h>


static uint32_t StrPrvPrintfEx_number(printf_write_c putc_, void* userData, uint64_t number, bool base10, bool zeroExtend, bool isSigned, uint32_t padToLength, bool caps, bool* bail)
{
    char buf[64];
    uint32_t idx = sizeof(buf) - 1;
    uint32_t chr, i;
    bool neg = false;
    uint32_t numPrinted = 0;

    *bail = false;

    if (padToLength > sizeof(buf) - 1)
        padToLength = sizeof(buf) - 1;

    buf[idx--] = 0;    //terminate

    if (isSigned) {

        if (((int64_t)number) < 0) {

            neg = true;
            number = -number;
        }
    }

    do{
        if (base10) {
            uint64_t t = U64_DIV_BY_CONST_U16(number, 10);
            chr = (number - t * 10) + '0';
            number = t;
        }
        else {
            chr = number & 0x0F;
            number >>= 4;
            chr = (chr >= 10) ? (chr + (caps ? 'A' : 'a') - 10) : (chr + '0');
        }

        buf[idx--] = chr;

        numPrinted++;

    } while (number);

    if (neg) {

        buf[idx--] = '-';
        numPrinted++;
    }

    if (padToLength > numPrinted) {

        padToLength -= numPrinted;
    }
    else {

        padToLength = 0;
    }

    while (padToLength--) {

        buf[idx--] = zeroExtend ? '0' : ' ';
        numPrinted++;
    }

    idx++;


    for(i = 0; i < numPrinted; i++) {

        if (!putc_(userData,(buf + idx)[i])) {

            *bail = true;
            break;
        }
    }


    return i;
}

static uint32_t StrVPrintf_StrLen_withMax(const char* s, uint32_t max)
{
    uint32_t len = 0;

    while ((*s++) && (len < max)) len++;

    return len;
}

static uint32_t StrVPrintf_StrLen(const char* s)
{
    uint32_t len = 0;

    while (*s++) len++;

    return len;
}

static inline char prvGetChar(const char** fmtP)
{

    return *(*fmtP)++;
}

uint32_t cvprintf(printf_write_c putc_f, void* userData, const char* fmtStr, va_list vl)
{

    char c, t;
    uint32_t numPrinted = 0;
    uint64_t val64;

#define putc_(_ud,_c)                \
        do {                 \
            if (!putc_f(_ud,_c))    \
                goto out;    \
        } while(0)

    while ((c = prvGetChar(&fmtStr)) != 0) {

        if (c == '\n') {

            putc_(userData,c);
            numPrinted++;
        }
        else if (c == '%') {

            bool zeroExtend = false, useLong = false, useLongLong = false, useSizeT = false, bail = false, caps = false;
            uint32_t padToLength = 0, len, i;
            const char* str;

more_fmt:

            c = prvGetChar(&fmtStr);

            switch(c) {

                case '%':

                    putc_(userData,c);
                    numPrinted++;
                    break;

                case 'c':

                    t = va_arg(vl,unsigned int);
                    putc_(userData,t);
                    numPrinted++;
                    break;

                case 's':

                    str = va_arg(vl,char*);
                    if (!str) str = "(null)";
                    if (padToLength)
                        len = StrVPrintf_StrLen_withMax(str,padToLength);
                    else
                        padToLength = len = StrVPrintf_StrLen(str);

                    if (len > padToLength)
                        len = padToLength;
                    else {
                        for(i = len; i < padToLength; i++)
                            putc_(userData, ' ');
                    }
                    numPrinted += padToLength;
                    for(i = 0; i < len; i++)
                        putc_(userData,*str++);

                    numPrinted += len;
                    break;

                case '0':
                case '.':

                    if (!zeroExtend && !padToLength) {

                        zeroExtend = true;
                        goto more_fmt;
                    }

                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':

                    padToLength = (padToLength * 10) + c - '0';
                    goto more_fmt;

#define GET_UVAL64() \
        useSizeT ? va_arg(vl, size_t) :                 \
        useLongLong ? va_arg(vl, unsigned long long) :  \
        useLong ? va_arg(vl, unsigned long) :           \
        va_arg(vl, unsigned int)

#define GET_SVAL64() \
        useSizeT ? va_arg(vl, size_t) :                 \
        useLongLong ? va_arg(vl, signed long long) :    \
        useLong ? va_arg(vl, signed long) :             \
        va_arg(vl, signed int)

                case 'u':

                    val64 = GET_UVAL64();
                    numPrinted += StrPrvPrintfEx_number(putc_f, userData, val64, true, zeroExtend,0,padToLength,0,&bail);
                    if (bail)
                        goto out;
                    break;

                case 'd':
                case 'i':

                    val64 = GET_SVAL64();
                    numPrinted += StrPrvPrintfEx_number(putc_f, userData, val64, true, zeroExtend, true, padToLength, false, &bail);
                    if (bail)
                        goto out;
                    break;

                case 'X':
                    caps = true;

                case 'x':

                    val64 = GET_UVAL64();
                    numPrinted += StrPrvPrintfEx_number(putc_f, userData, val64, false, zeroExtend, false, padToLength, caps, &bail);
                    if (bail)
                        goto out;
                    break;

                case 'p':
                    putc_(userData,'0');
                    putc_(userData,'x');
                    numPrinted += 2;
                    val64 = (uintptr_t)va_arg(vl, const void*);
                    numPrinted += StrPrvPrintfEx_number(putc_f, userData, val64, false, zeroExtend, false, padToLength, caps, &bail);
                    if (bail)
                        goto out;
                    break;

#undef GET_UVAL64
#undef GET_SVAL64

                case 'L':
                case 'l':
                    if (useLong)
                        useLongLong = true;
                    useLong = true;
                    goto more_fmt;

                case 'z':
                    useSizeT = true;
                    goto more_fmt;

                default:

                    putc_(userData,c);
                    numPrinted++;
                    break;

            }
        }
        else {

            putc_(userData,c);
            numPrinted++;
        }
    }

out:

    return numPrinted;
}
