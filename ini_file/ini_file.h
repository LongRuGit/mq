
/**
 * @file    ini_file.h
 * @brief   Ini文件读写类，从cpini(Jeffrey Du)改造过来
 * @author  jamieli@tencent.com
 * @date    2008-12-05
 */

#ifndef _INI_FILE_
#define _INI_FILE_

namespace WSMQ
{

class CIniFile
{
public:
    const static int SUCCESS = 0;
    const static int ERROR = -1;

    const static int E_INI_FILE = -401;

public:

    /**
     @brief Load ini file into buffer.
     @param sIniFile: Ini file name.
     */
    CIniFile(const char *szIniFile);

    /**
     @brief Release buffer.
     */
    ~CIniFile();

    /**
     @brief Read the value of specific item and fill it into string buffer.
     @param sSection: Ini file section name.
     @param sItem: Ini file item name.
     @param sDefault: The default value. When the function fail to locate the item, it will
     fill the default value into sValue and return 1.
     @param sValue: The buffer to store value.
     @param nValueLen: The length of the sValue buffer.
     @return 0=OK, 1=FAIL
     */
    int GetString(const char *sSection, const char *sItem,
            const char *sDefault, char *sValue, const int nValueLen);

    /**
     @param sSection: Ini file section name.
     @param sItem: Ini file item name.
     @param nDefault: The default value. When the function fail to locate the item, it will
     fill the default value into sValue and return 1.
     @param nValue: The buffer to store value.
     @brief Read the value of specific item and fill it into integer buffer.
     @return 0=OK, 1=FAIL
     */
    int GetInt(const char *sSection, const char *sItem, const int nDefault,
            int *nValue);

    int GetULongLong(const char *szSection, const char *szItem, unsigned long long ullDefault,
            unsigned long long *pullValue);

    /**
     @brief To see if the cpIni class successfully load ini file into buffer.
     @return 1=OK, 0=FAIL
     */
    int IsValid();

private:
    CIniFile(const CIniFile&);
    CIniFile& operator=(const CIniFile&);

private:

    /// Store Ini file text buffer
    char *m_szBuffer;
    int m_iSize;
};

}

#endif
