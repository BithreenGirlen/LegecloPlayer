#ifndef WIN_TEXT_H_
#define WIN_TEXT_H_

#include <string>

namespace win_text
{
    std::wstring WidenFromUtf8(const std::string& str);
    std::wstring WidenFromUtf8(const char* str, int length);

    std::string NarrowToUtf8(const std::wstring& wstr);
    std::string NarrowToUtf8(const wchar_t* wstr, int length);

    std::wstring WidenFromAnsi(const std::string& str);
    std::wstring WidenFromAnsi(const char* str, int length);

    std::string NarrowToAnsi(const std::wstring& wstr);
    std::string NarrowToAnsi(const wchar_t* wstr, int length);

    /// @brief Convert UTF-8 to UTF-16 in buffer
    /// @param dst A pointer to buffer that receives converted string without null termination
    /// @return 0 on failure; written length on success
    int WidenFromUtf8InBuffer(const char* str, int length, wchar_t* dst, int dstSize);
    /// @brief Convert UTF-16 to UTF-8 in buffer
    /// @param dst A pointer to buffer that receives converted string without null termination
    /// @return 0 on failure; written length on success
    int NarrowToUtf8InBuffer(const wchar_t* wstr, int length, char* dst, int dstSize);
}

#endif //WIN_TEXT_H_
