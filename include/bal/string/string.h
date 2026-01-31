#pragma once

#include "primitives.h"
#include "uint64.h"
#include "int64.h"
#include "double.h"
#include "embedded_string.h"

// Class to handle string operations
class String
{
private:
    template <TCHAR TChar>
    static BOOL IsSpace(TChar c); // Check if character is a whitespace
    template <TCHAR TChar>
    static BOOL IsDigit(TChar c); // Check if character is a digit

public:
    // Getting the length of a string
    template <TCHAR TChar>
    static USIZE Length(const TChar *pChar);
    // Convert character to lowercase
    template <TCHAR TChar>
    static TChar ToLowerCase(TChar c);
    // Find the address of a character in a string
    template <TCHAR TChar>
    static const TChar *AddressOf(TChar c, const TChar *pChar);
    // Check if a string starts with a given substring
    template <TCHAR TChar>
    static BOOL StartsWith(const TChar *pChar, const TChar *pSubString);
    // Compare two strings
    template <TCHAR TChar>
    static BOOL Compare(const TChar *s1, const TChar *s2, BOOL ignoreCase = FALSE);
    // Copy a string from src to dest
    template <TCHAR TChar>
    static TChar *Copy(TChar *dest, const TChar *src);
    // Trim whitespace from the end of a string
    template <TCHAR TChar>
    static USIZE TrimEnd(TChar *str);

    // Note: naming may be improved later
    template <typename T>
    static T ParseString(const CHAR *str);

    // UTF-16 (WCHAR) <-> UTF-8 (CHAR) conversion
    static USIZE WideToUtf8(PCWCHAR wide, PCHAR utf8, USIZE utf8BufferSize);
    static USIZE Utf8ToWide(PCCHAR utf8, PWCHAR wide, USIZE wideBufferSize);
};

// Implementation of String methods
template <TCHAR TChar>
TChar String::ToLowerCase(TChar c)
{   // Check if the character is uppercase, convert to lowercase
    if (c >= (TChar)'A' && c <= (TChar)'Z')
    {
        return c + ((TChar)'a' - (TChar)'A');
    }
    return c;
}

template <TCHAR TChar>
USIZE String::Length(const TChar *p)
{
    USIZE i = 0; // Counter to store length of the string

    // Loop through the string until we reach the null terminator, so it counts the number of characters
    while (p[i] != (TChar)'\0')
    {
        i++;
    }
    return i; // Return the length of the string
}

template <TCHAR TChar>
const TChar *String::AddressOf(TChar c, const TChar *pChar)
{

    USIZE i = 0;             // Counter to store the index of the string
    while (pChar[i] != '\0') // Check if the string is not over
    {
        if (pChar[i] == c) // If the character is found
        {
            return &pChar[i]; // Return the address of the character
        }
        i++;
    }
    return NULL; // Character not found, address is null
}

template <TCHAR TChar>
TChar *String::Copy(TChar *dest, const TChar *src)
{
    SSIZE i = 0; // Counter to store the index of the string
    // Copy each character from src to dest until null terminator is reached
    while (src[i] != (TChar)'\0')
    {
        dest[i] = src[i]; // Copy character
        i++; // Move to next character
    }
    // Null-terminate the destination string
    dest[i] = (TChar)'\0';
    return dest;
}

template <TCHAR TChar>
USIZE String::TrimEnd(TChar *str)
{
    // If string is null, return 0
    if (!str)
        return 0;
    // Get the length of the string
    INT32 len = (INT32)String::Length(str);
    if (len == 0)
        return 1; // Empty string only \0

    USIZE lenghtAfterTrim = len + 1;

    // Start from the end and move backward
    TChar *p = str + len - 1;
    while (p >= str && String::IsSpace((TChar)*p))
    {
        *p = (TChar)'\0'; // replace trailing space with null terminator
        p--;
        lenghtAfterTrim--;
    }
    return lenghtAfterTrim;
}

template <TCHAR TChar>
BOOL String::StartsWith(const TChar *pChar, const TChar *pSubString)
{
    USIZE i = 0;                                      // Counter to store the index of the string
    while (pChar[i] != '\0' && pSubString[i] != '\0') // Check if the strings are not over
    {
        if (pChar[i] != pSubString[i]) // Check if the characters are not equal
        {
            return FALSE;
        }
        i++; // Move to next character
    }

    if (pSubString[i] != '\0') // Substring is longer than the string
    {
        return FALSE;
    }

    return TRUE; // String contains the substring, return TRUE
}

template <TCHAR TChar>
BOOL String::Compare(const TChar *s1, const TChar *s2, BOOL ignoreCase)
{
    const TChar *str1 = s1;
    const TChar *str2 = s2;
    // Loop through each character in both strings
    while (*str1 && *str2)
    {
        // Convert to lowercase cause-insensitive comparison
        TChar c1 = ignoreCase ? ToLowerCase(*str1) : *str1;
        TChar c2 = ignoreCase ? ToLowerCase(*str2) : *str2;
        // Compare the characters
        if (c1 != c2)
        {
            return FALSE; // They differ in case-folded form
        }
        // Move to the next characters
        str1++;
        str2++;
    }
    return (*str1 == *str2); // Both must land on the null terminator together
}

template <TCHAR TChar>
BOOL String::IsSpace(TChar c)
{
    // Check for standard whitespace characters
    return (c == (TChar)' ' ||  // space
            c == (TChar)'\t' || // horizontal tab
            c == (TChar)'\n' || // newline
            c == (TChar)'\v' || // vertical tab
            c == (TChar)'\f' || // form feed
            c == (TChar)'\r');  // carriage return
}

template <TCHAR TChar>
BOOL String::IsDigit(TChar c)
{
    // Check if character is a digit (0-9)
    return (c >= (TChar)'0' && c <= (TChar)'9');
}
