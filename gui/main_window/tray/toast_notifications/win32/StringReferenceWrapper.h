/*#pragma once

typedef HRESULT (__stdcall *WindowsCreateStringReferenceInstance)(_In_reads_opt_(length+1) PCWSTR sourceString,
    UINT32 length, 
    _Out_ HSTRING_HEADER * hstringHeader,
    _Out_ HSTRING * string);
static WindowsCreateStringReferenceInstance createStringRef;

typedef HRESULT (__stdcall *WindowsDeleteStringInstance)(HSTRING string);

class StringReferenceWrapper
{  
public:  
    StringReferenceWrapper(_In_reads_(length) PCWSTR stringRef, _In_ UINT32 length) throw()  
    {  
        if (!createStringRef)
        {
            HINSTANCE dll = LoadLibraryW(L"api-ms-win-core-winrt-string-l1-1-0.dll");
            if (dll)
            {
                createStringRef = (WindowsCreateStringReferenceInstance)GetProcAddress(dll, "WindowsCreateStringReference");
            }
        }

        if (createStringRef)
            createStringRef(stringRef, length, &_header, &_hstring);  
    }

    StringReferenceWrapper(const QString& str) throw()  
    {  
        if (!createStringRef)
        {
            HINSTANCE dll = LoadLibraryW(L"api-ms-win-core-winrt-string-l1-1-0.dll");
            if (dll)
            {
                createStringRef = (WindowsCreateStringReferenceInstance)GetProcAddress(dll, "WindowsCreateStringReference");
            }
        }

        if (createStringRef)
            createStringRef((const wchar_t*)str.utf16(), str.length(), &_header, &_hstring);  
    }

    ~StringReferenceWrapper()
    {
        static WindowsDeleteStringInstance deleteStringRef;
        if (!deleteStringRef)
        {
            HINSTANCE dll = LoadLibraryW(L"api-ms-win-core-winrt-string-l1-1-0.dll");
            if (dll)
            {
                deleteStringRef = (WindowsDeleteStringInstance)GetProcAddress(dll, "WindowsDeleteString");
            }
        }

        if (deleteStringRef)
            deleteStringRef(_hstring);
    }

    template <size_t N>  
    StringReferenceWrapper(_In_reads_(N) wchar_t const (&stringRef)[N]) throw()  
    {  
        UINT32 length = N-1; 
        if (!createStringRef)
        {
            HINSTANCE dll = LoadLibraryW(L"api-ms-win-core-winrt-string-l1-1-0.dll");
            if (dll)
            {
                createStringRef = (WindowsCreateStringReferenceInstance)GetProcAddress(dll, "WindowsCreateStringReference");
            }
        }

        if (createStringRef)
            createStringRef(stringRef, length, &_header, &_hstring); 
    }

    template <size_t _>  
    StringReferenceWrapper(_In_reads_(_) wchar_t (&stringRef)[_]) throw()  
    {  
        UINT32 length;  
        SizeTToUInt32(wcslen(stringRef), &length); 
        if (!createStringRef)
        {
            HINSTANCE dll = LoadLibraryW(L"api-ms-win-core-winrt-string-l1-1-0.dll");
            if (dll)
            {
                createStringRef = (WindowsCreateStringReferenceInstance)GetProcAddress(dll, "WindowsCreateStringReference");
            }
        }

        if (createStringRef)
            createStringRef(stringRef, length, &_header, &_hstring);
    }
 
    HSTRING Get() const throw()  
    {  
        return _hstring;  
    }  
   
  
private:               
    HSTRING             _hstring;  
    HSTRING_HEADER      _header;  
};  
*/