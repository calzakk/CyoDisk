/*
[CyoDisk] cyodisk.cpp

The MIT License (MIT)

Copyright (c) 2014-2021 Graham Bull

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
#include <vector>

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
        gibibytes,
        terabytes,
        tebibytes
    };

    class Output
    {
    public:
        Output( Units units, int depth, bool noZero, bool noFree, bool noProgress )
            : depth_( depth ),
            noZero_( noZero ),
            noFree_( noFree ),
            noProgress_( noProgress )
        {
            struct Data
            {
                __int64 div;
                const char* units;
                int width;
            };
            const Data data[] =
            {
                { 1LL, nullptr, 19 }, //bytes
                { 1000LL, " KB", 18 }, //kilobytes
                { 1024LL, " KiB", 19 }, //kibibytes
                { 1000LL * 1000, " MB", 14 }, //megabytes
                { 1024LL * 1024, " MiB", 15 }, //mebibytes
                { 1000LL * 1000 * 1000, " GB", 10 }, //gigabytes
                { 1024LL * 1024 * 1024, " GiB", 11 }, //gibibytes
                { 1000LL * 1000 * 1000 * 1000, " TB", 6 }, //terabytes
                { 1024LL * 1024 * 1024 * 1024, " TiB", 7 } //tebibytes
            };
            assert(Units::bytes <= units && units <= Units::tebibytes);

            auto& ref = data[(int)units];

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
            std::string niceSize = NiceSize( folderSize_, &length, noZero_ );
            if (!niceSize.empty())
            {
                if (!noProgress_)
                    std::cout << '\r';
                std::cout << niceSize << "  ." << std::endl;
            }
            std::cout << std::string( width_, '-' ) << std::endl;
            std::cout << NiceSize( totalSize_, nullptr, false ) << std::endl;

            if (!noFree_)
            {
                ULARGE_INTEGER totalFreeSpace;
                if (::GetDiskFreeSpaceExW(NULL, NULL, NULL, &totalFreeSpace))
                    std::cout << NiceSize(totalFreeSpace.QuadPart, nullptr, false) << " free" << std::endl;
            }
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
        const bool noZero_;
        const bool noFree_;
        const bool noProgress_;
        __int64 folderSize_ = 0;
        __int64 totalSize_ = 0;
        __int64 add_;
        __int64 div_;
        const char* units_;
        int width_;
        Folders folders_;
        std::wstring currFolder_;
        bool isLink_;
        bool progress_;
        ULONGLONG lastProgress_;
        int progressPhase_;

        void Dump()
        {
            bool first = true;
            for (Folders::const_iterator i = folders_.begin(); i != folders_.end(); ++i)
            {
                int length;
                std::string niceSize = NiceSize( i->second.size, &length, noZero_ );
                if (niceSize.empty())
                    continue;

                if (!noProgress_)
                    std::cout << '\r';
                std::cout << niceSize << "  ";
                for (int j = 0; j < i->second.level; ++j)
                    std::cout << "  ";
                if (i->second.isLink)
                    std::cout << '[';
                std::cout << ToMultiByte(i->second.name);
                if (i->second.isLink)
                    std::cout << ']';
                std::cout << std::endl;
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

            ULONGLONG now = ::GetTickCount64();

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
                const char* const c_progress = "-\\|/";
                static const int c_maxProgress = (int)std::strlen( c_progress );
                std::cout << '\r' << c_progress[ progressPhase_ ] << ' ';
                if (isLink_)
                    std::cout << '[';
                std::cout << ToMultiByte(currFolder_);
                if (isLink_)
                    std::cout << ']';
                std::cout << std::flush;
                progressPhase_ = (progressPhase_ + 1) % c_maxProgress;
                lastProgress_ = now;
            }
        }

        std::string ToMultiByte(const std::wstring& str)
        {
            auto destSize = (str.length() * 2);
            std::vector<char> dest(destSize, ' ');
            auto numBytes = ::WideCharToMultiByte(CP_ACP, 0, str.c_str(), -1, &dest[0], (int)destSize, NULL, FALSE);
            return &dest[0];
        }

        std::string NiceSize( __int64 size, int* length, bool noZero ) const
        {
            char str[ 100 ];

            if (size >= 0)
            {
                size = ((size + add_) / div_);
                if (size == 0 && noZero)
                    return "";

                _i64toa( size, str, 10 );

                int offset = 0;
                __int64 temp = size;
                while (temp >= 1000)
                {
                    ++offset;
                    temp /= 1000;
                }

                char* from = str;
                while (*from)
                    ++from;
                char* to = (from + offset);

                int len = 0;
                while (from > str)
                {
                    *to-- = *from--;
                    if (++len >= 4)
                    {
                        *to-- = ',';
                        len = 1;
                    }
                }

                if (units_ != nullptr)
                    std::strcat( str, units_ );
            }
            else
            {
                str[ 0 ] = '?';
                str[ 1 ] = '\x0';
            }

            int len = (int)std::strlen( str );
            if (length != nullptr)
                *length = len;

            char ret[ 100 ];
            char* p = ret;
            for (int i = len; i < width_; ++i)
                *p++ = ' ';
            std::strcpy( p, str );
            return ret;
        }
    };

    __int64 RecurseFolder( const wchar_t* path, const wchar_t* subpath, bool noLinks, bool noOffline, int level, Output& output )
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

            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DEVICE) != 0)
                continue;

            bool isOffline = (fd.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE) != 0;
            if (isOffline && noOffline)
                continue;

            bool isLink = (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
            if (isLink && noLinks)
                continue;

            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
            {
                if (level == 0)
                    output.CurrentFolder( fd.cFileName, isLink );

                __int64 folderSize = RecurseFolder( pathname.c_str(), subpathname.c_str(), noLinks, noOffline, level + 1, output );
                output.AddFolder( subpathname, fd.cFileName, isLink, folderSize, level );
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
    bool noZero = false;
    bool noFree = false;
    bool noOffline = true;
    for (int i = 1; i < argc; ++i)
    {
        if (wcscmp( argv[ i ], L"/?" ) == 0
            || wcscmp( argv[ i ], L"-?" ) == 0
            || _wcsicmp( argv[ i ], L"--help" ) == 0)
        {
            std::cout << "CYODISK [options...]\n" \
                "\n" \
                "Options:\n" \
                "  /units        One of: BYTES, KB, KiB, MB, MiB (default), GB, GiB, TB, TiB.\n" \
                "  /NOPROGRESS   Don't output any progress info; useful when redirecting the\n" \
                "                output to a file.\n" \
                "  /NOLINKS      Don't follow symbolic links or directory junctions.\n" \
                "  /NOZERO       Don't display folders with a size of 0 in the selected units.\n" \
                "  /NOFREE       Don't display amount of free disk space.\n" \
                "  /OFFLINE      Include offline files that aren't stored on the disk.\n" \
                "  /DEPTH depth  0: don't list subfolders;\n" \
                "                1: list subfolders (default);\n" \
                "                2: list subfolders and their subfolders; etc." << std::endl;
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
        else if (_wcsicmp( argv[ i ], L"/tb" ) == 0)
            units = Units::terabytes;
        else if (_wcsicmp( argv[ i ], L"/tib" ) == 0)
            units = Units::tebibytes;
        else if(_wcsicmp( argv[ i ], L"/noprogress" ) == 0)
            noProgress = true;
        else if (_wcsicmp(argv[i], L"/nolinks") == 0)
            noLinks = true;
        else if (_wcsicmp(argv[i], L"/nozero") == 0)
            noZero = true;
        else if (_wcsicmp(argv[i], L"/nofree") == 0)
            noFree = true;
        else if (_wcsicmp(argv[i], L"/offline") == 0)
            noOffline = false;
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

    Output output( units, depth, noZero, noFree, noProgress );

    RecurseFolder( szPath, L"", noLinks, noOffline, 0, output );

    output.Finish();

    return 0;
}
