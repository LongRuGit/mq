#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "ini_file.h"

namespace WSMQ
{

void TrimString(char *szString)
{
    int     iLen;
    int     n, iLeftStart;

    iLen = strlen(szString);
    if(iLen == 0)
        return;

    /* trime right first */
    for(n=iLen-1; n>=0; --n)
    {
        // trim space and tab
        if(szString[n] == ' ' || szString[n] == '\t' || szString[n] == '\r' || szString[n] == '\n')
            szString[n] = 0;
        else
            break;
    }

    iLen = strlen(szString);
    if(iLen == 0)
        return;

    /* trim left */
    for(n=0; n<iLen; ++n)
    {
        if(szString[n] != ' ' &&  szString[n] != '\t' && szString[n] != '\r' && szString[n] != '\n')
            break;
    }

    /* no space at the left */
    if(n == 0)
        return;

    iLeftStart = n;

    /* plus NULL */
    memmove(szString, &(szString[iLeftStart]), iLen-iLeftStart+1);
}

CIniFile::CIniFile(const char *szIniFile)
{
    FILE *pFile;

    m_szBuffer = NULL;
    m_iSize = 0;

    struct stat stFileStat;
    if(stat(szIniFile, &stFileStat) != 0 )
    {
        return;
    }

    m_iSize = stFileStat.st_size;

    if (m_iSize <= 0)
        return;

    m_szBuffer = (char *) malloc(m_iSize + 1);
    if (m_szBuffer == NULL)
        return;
    memset(m_szBuffer, 0, m_iSize + 1);

    pFile = fopen(szIniFile, "r");
    size_t siRead = fread(m_szBuffer, m_iSize, 1, pFile);
    if (siRead != (size_t)m_iSize)
    {
        /*
        free(m_szBuffer);
        m_szBuffer = NULL;
        */
    }
    fclose(pFile);
}

CIniFile::~CIniFile()
{
    if (m_szBuffer != NULL)
    {
        free(m_szBuffer);
    }
}

int CIniFile::GetString(const char *szSection, const char *szItem,
        const char *szDefault, char *szValue, const int iValueLen)
{
    char *pszSection = NULL, *pszNextSection = NULL;
    char *pszItem = NULL, *pszValue = NULL, *pszTestSection;
    char *pszTemp;
    int n, iPos, iSectionLen, iItemLen;

    strncpy(szValue, szDefault, iValueLen);
    iSectionLen = strlen(szSection);
    iItemLen = strlen(szItem);

    if (!IsValid())
        return E_INI_FILE;

    // locate section first
    pszTemp = m_szBuffer;
    while (pszTemp < m_szBuffer + m_iSize)
    {
        pszSection = strstr(pszTemp, szSection);
        if (pszSection == NULL)
        {
            return E_INI_FILE;
        }

        // section name at the beginning of ini file, no '['.
        if (pszSection == m_szBuffer)
        {
            return E_INI_FILE;
        }

        if (*(pszSection - 1) != '[' || *(pszSection + iSectionLen) != ']')
        {
            pszTemp = pszSection + iSectionLen;
            continue;
        }
        // make sure the section name is at the beginning of the line
        if (pszSection > m_szBuffer + 1)
        {
            if (*(pszSection - 2) != '\n')
            {
                pszTemp = pszSection + iSectionLen;
                continue;
            }
        }

        pszTemp = pszSection;
        break;
    }

    // locate the entry of next section
    pszTestSection = pszTemp;
    while (pszTestSection != NULL)
    {
        pszNextSection = strstr(pszTestSection, "[");
        if (pszNextSection != NULL)
        {
            // '[' must at the first character of the line
            if (*(pszNextSection - 1) == '\n')
            {
                break;
            }
            else
            {
                // maybe '[' is at the comment or somewhere else, search backward
                pszTestSection = pszNextSection + 1;
            }
        }
        else
        {
            break;
        }
    }
    // no more section, assign the end of the buffer to pszNextSection
    if (pszNextSection == NULL)
    {
        pszNextSection = m_szBuffer + m_iSize;
    }

    // locate item
    while (pszTemp < m_szBuffer + m_iSize)
    {
        pszItem = strstr(pszTemp, szItem);
        if (pszItem == NULL)
        {
            return E_INI_FILE;
        }

        // no matched item in the section, this is the item belonging to following section
        if (pszItem >= pszNextSection)
        {
            return E_INI_FILE;
        }

        // not the begining of the line
        if (*(pszItem - 1) != '\n')
        {
            pszTemp = pszItem + iItemLen;
            continue;
        }
        // not the exactly matched word
        if (*(pszItem + iItemLen) != ' ' && *(pszItem + iItemLen) != '=' && *(pszItem
                + iItemLen) != '\t')
        {
            pszTemp = pszItem + iItemLen;
            continue;
        }

        // extract value
        pszValue = strstr(pszItem, "=");
        if (pszValue == NULL)
        {
            return E_INI_FILE;
        }
        // skip '='
        pszValue++;

        // the end of file
        if (pszValue >= m_szBuffer + m_iSize)
        {
            return E_INI_FILE;
        }

        // copy the value
        iPos = 0;
        // reserve one byte to store NULL
        for (n = 0; iPos < iValueLen - 1 && pszValue[n] != '\r' && pszValue[n]
                != '\n' && pszValue[n] != 0; ++n)
        {
            // skip left space or tab
            if ((pszValue[n] == ' ' || pszValue[n] == '\t') && iPos == 0)
                continue;
            // skip comment
            if (pszValue[n] == ';')
            {
                if (n > 0)
                {
                    // skip the second ';'
                    if (pszValue[n + 1] == ';')
                    {
                        szValue[iPos] = pszValue[n];
                        iPos++;
                        n++;
                        continue;
                    }
                }
                break;
            }

            szValue[iPos] = pszValue[n];
            iPos++;
        }
        szValue[iPos] = 0;
        break;
    }

    // trim string
    TrimString(szValue);

    return SUCCESS;
}

int CIniFile::GetInt(const char *szSection, const char *szItem,
        const int iDefault, int *piValue)
{
    char szInt[32] =
    { 0 };

    if (!IsValid())
        return E_INI_FILE;

    if (GetString(szSection, szItem, "", szInt, sizeof(szInt)) != 0)
    {
        *piValue = iDefault;
        return E_INI_FILE;
    }
    if (strncmp(szInt, "0x", 2) == 0 || strncmp(szInt, "0X", 2) == 0)
        sscanf(szInt + 2, "%x", piValue);
    else
        *piValue = atoi(szInt);

    return SUCCESS;
}

int CIniFile::GetULongLong(const char *szSection, const char *szItem, unsigned long long ullDefault,
        unsigned long long *pullValue)
{
    char szULongLong[32] =
    { 0 };

    if (!IsValid())
        return E_INI_FILE;

    if (GetString(szSection, szItem, "", szULongLong, sizeof(szULongLong)) != 0)
    {
        *pullValue = ullDefault;
        return E_INI_FILE;
    }
    *pullValue = strtoull(szULongLong, NULL, 10);

    return SUCCESS;
}


int CIniFile::IsValid()
{
    if (m_szBuffer == NULL || m_iSize <= 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

}

