/*
[CyoDisk] cyodisk.cpp

The MIT License (MIT)

Copyright (c) 2014-2016 Graham Bull

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cassert>
#include <iostream>
#include <list>
#include <minmax.h>
#include <map>
#include <string>

namespace
{
    enum class Units
    {
        bytes = 0,
        kilobytes,
        kibibytes,
        megabytes,
        mebibytes,
        gigabytes,
        gibibytes
    };

    class Output
    {
    public:
        Output( Units units, int depth, bool noProgress )
            : depth_( depth ), noProgress_( noProgress )
        {
            struct Data
            {
                __int64 div;
                const wchar_t* units;
                int width;
            };
            const Data data[] =
            {
                { 1, nullptr, 15 }, //bytes
                { 1000, L" KB", 14 }, //kilobytes
                { 1024, L" KiB", 15 }, //kibibytes
                { 1000 * 1000, L" MB", 10 }, //megabytes
                { 1024 * 1024, L" MiB", 11 }, //mebibytes
                { 1000 * 1000 * 1000, L" GB", 6 }, //gigabytes
                { 1024 * 1024 * 1024, L" GiB", 7 } //gibibytes
            };
            assert(Units::bytes <= units && units <= Units::gibibytes);

            const Data& ref = data[(int)units];

            div_ = ref.div;
            add_ = (div_ / 2);
            units_ = ref.units;
            width_ = ref.width;

            ResetProgress();
        }

        void CurrentFolder( const std::wstring& folder, bool isLink )
        {
            currFolder_ = folder;
            isLink_ = isLink;
        }

        void AddFolder( const std::wstring& subpath, const std::wstring& name, bool isLink, __int64 size, int level )
        {
            if (level < depth_)
            {
                PathData pd = { name, isLink, size, level };
                folders_[subpath] = pd;
            }

            if (level == 0)
            {
                Dump();

                ResetProgress();
            }
        }

        void AddFile( __int64 size, int level )
        {
            totalSize_ += size;

            if (level == 0)
                folderSize_ += size;

            Progress();
        }

        void Finish()
        {
            Dump();

            int length;
            std::wstring niceSize = NiceSize( folderSize_, &length );
            if (!noProgress_)
                std::wcout << L'\r';
            std::wcout << niceSize << L"  ." << std::endl;
            std::wcout << std::wstring( width_, L'-' ) << std::endl;
            std::wcout << NiceSize( totalSize_, nullptr ) << std::endl;
        }

    private:
        struct PathData
        {
            std::wstring name;
            bool isLink;
            __int64 size;
            int level;
        };
        typedef std::map<std::wstring, PathData> Folders;

        const int depth_;
        const bool noProgress_;
        __int64 folderSize_ = 0;
        __int64 totalSize_ = 0;
        __int64 add_;
        __int64 div_;
        const wchar_t* units_;
        int width_;
        Folders folders_;
        std::wstring currFolder_;
        bool isLink_;
        bool progress_;
        DWORD lastProgress_;
        int progressPhase_;

        void Dump()
        {
            bool first = true;
            for (Folders::const_iterator i = folders_.begin(); i != folders_.end(); ++i)
            {
                int length;
                std::wstring niceSize = NiceSize( i->second.size, &length );
                if (!noProgress_)
                    std::wcout << L'\r';
                std::wcout << niceSize << L"  ";
                for (int j = 0; j < i->second.level; ++j)
                    std::wcout << L"  ";
                if (i->second.isLink)
                    std::wcout << L'[';
                std::wcout << i->second.name;
                if (i->second.isLink)
                    std::wcout << L']';
                std::wcout << std::endl;
                first = false;
            }

            folders_.clear();
        }

        void ResetProgress()
        {
            currFolder_ = L"";
            isLink_ = false;
            progress_ = false;
            lastProgress_ = 0;
            progressPhase_ = 0;
        }

        void Progress()
        {
            if (noProgress_)
                return;

            DWORD now = ::GetTickCount();

            if (lastProgress_ == 0)
            {
                lastProgress_ = now;
                return;
            }

            if (!progress_)
            {
                if (now - lastProgress_ >= 2000)
                    progress_ = true;
            }

            if (!noProgress_
                && progress_
                && (now - lastProgress_ >= 500))
            {
                const wchar_t* const c_progress = L"-\\|/";
                static const int c_maxProgress = (int)std::wcslen( c_progress );
                std::wcout << L'\r' << c_progress[ progressPhase_ ] << L' ';
                if (isLink_)
                    std::wcout << L'[';
                std::wcout << currFolder_;
                if (isLink_)
                    std::wcout << L']';
                std::wcout << std::flush;
                progressPhase_ = (progressPhase_ + 1) % c_maxProgress;
                lastProgress_ = now;
            }
        }

        std::wstring NiceSize( __int64 size, int* length ) const
        {
            wchar_t str[ 100 ];

            if (size >= 0)
            {
                size = ((size + add_) / div_);
                _i64tow( size, str, 10 );

                int offset = 0;
                __int64 temp = size;
                while (temp >= 1000)
                {
                    ++offset;
                    temp /= 1000;
                }

                wchar_t* from = str;
                while (*from)
                    ++from;
                wchar_t* to = (from + offset);

                int len = 0;
                while (from > str)
                {
                    *to-- = *from--;
                    if (++len >= 4)
                    {
                        *to-- = L',';
                        len = 1;
                    }
                }

                if (units_ != nullptr)
                    std::wcscat( str, units_ );
            }
            else
            {
                str[ 0 ] = L'?';
                str[ 1 ] = L'\x0';
            }

            int len = (int)std::wcslen( str );
            if (length != nullptr)
                *length = len;

            wchar_t ret[ 100 ];
            wchar_t* p = ret;
            for (int i = len; i < width_; ++i)
                *p++ = ' ';
            std::wcscpy( p, str );
            return ret;
        }
    };

    __int64 RecurseFolder( const wchar_t* path, const wchar_t* subpath, bool noLinks, int level, Output& output )
    {
        __int64 totalSize = 0;

        std::wstring fileSpec = path;
        fileSpec += L"\\*.*";

        WIN32_FIND_DATAW fd = { 0 };
        HANDLE hSearch = FindFirstFileW( fileSpec.c_str(), &fd );
        if (hSearch == INVALID_HANDLE_VALUE)
            return -1;

        do
        {
            const wchar_t* next = fd.cFileName;
            if (*next == L'.')
            {
                ++next;
                if (*next == L'.')
                    ++next;
                if (*next == L'\x0')
                    continue;
            }

            std::wstring pathname = path;
            pathname += L'\\';
            pathname += fd.cFileName;

            std::wstring subpathname = subpath;
            if (!subpathname.empty())
                subpathname += L'\\';
            subpathname += fd.cFileName;

            if (fd.dwFileAttributes & (FILE_ATTRIBUTE_DEVICE | FILE_ATTRIBUTE_OFFLINE))
                continue;

            bool isLink = (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
            if (isLink && noLinks)
                continue;

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (level == 0)
                    output.CurrentFolder( fd.cFileName, isLink );

                __int64 folderSize = RecurseFolder( pathname.c_str(), subpathname.c_str(), noLinks, level + 1, output );
                output.AddFolder( subpathname, fd.cFileName, isLink, folderSize, level );
                if (folderSize >= 1)
                    totalSize += folderSize;
            }
            else
            {
                LARGE_INTEGER liFileSize;
                liFileSize.HighPart = fd.nFileSizeHigh;
                liFileSize.LowPart = fd.nFileSizeLow;
                __int64 fileSize = liFileSize.QuadPart;
                output.AddFile( fileSize, level );
                totalSize += fileSize;
            }
        }
        while (FindNextFileW( hSearch, &fd ));

        ::FindClose( hSearch );

        return totalSize;
    }

}

int wmain( int argc, wchar_t* argv[] )
{
    Units units = Units::mebibytes;
    bool noProgress = false;
    int depth = 1;
    bool noLinks = false;
    for (int i = 1; i < argc; ++i)
    {
        if (wcscmp( argv[ i ], L"/?" ) == 0
            || wcscmp( argv[ i ], L"-?" ) == 0
            || _wcsicmp( argv[ i ], L"--help" ) == 0)
        {
            std::wcout << L"Usage:\n\n  CYODISK [/units] [/NOPROGRESS] [/NOLINKS] [/DEPTH depth]\n" \
                L"\nwhere units is one of: BYTES, KB, KiB, MB, MiB, GB, or GiB (default is MiB)" << std::endl;
            return -1;
        }
        else if (_wcsicmp( argv[ i ], L"/bytes" ) == 0)
            units = Units::bytes;
        else if (_wcsicmp( argv[ i ], L"/kb" ) == 0)
            units = Units::kilobytes;
        else if (_wcsicmp( argv[ i ], L"/kib" ) == 0)
            units = Units::kibibytes;
        else if (_wcsicmp( argv[ i ], L"/mb" ) == 0)
            units = Units::megabytes;
        else if (_wcsicmp( argv[ i ], L"/mib" ) == 0)
            units = Units::mebibytes;
        else if (_wcsicmp( argv[ i ], L"/gb" ) == 0)
            units = Units::gigabytes;
        else if (_wcsicmp( argv[ i ], L"/gib" ) == 0)
            units = Units::gibibytes;
        else if(_wcsicmp( argv[ i ], L"/noprogress" ) == 0)
            noProgress = true;
        else if (_wcsicmp( argv[ i ], L"/nolinks" ) == 0)
            noLinks = true;
        else if (_wcsicmp( argv[ i ], L"/depth" ) == 0 && i + 1 < argc)
        {
            if (_wcsicmp( argv[ ++i ], L"max" ) == 0)
                depth = INT_MAX;
            else
                depth = _wtoi( argv[ i ]);
        }
        else
        {
            std::wcerr << L"Invalid argument: " << argv[ i ] << std::endl;
            return 1;
        }
    }

    wchar_t szPath[ MAX_PATH ];
    GetCurrentDirectoryW( MAX_PATH, szPath );

    Output output( units, depth, noProgress );

    RecurseFolder( szPath, L"", noLinks, 0, output );

    output.Finish();

    return 0;
}
