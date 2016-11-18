# CyoDisk

CyoDisk is a simple command-line application (currently Windows only) that displays the size of the current folder and all subfolders.

## Usage

Build the solution (release build), copy the resultant CyoDisk.exe to a folder, and ensure that the folder is in the path. CyoDisk can then be run using:

    CYODISK [/units] [/NOPROGRESS] [/NOLINKS] [/DEPTH depth]

where units is one of: BYTES, KB, KiB, MB, MiB, GB, or GiB (default is MiB).

Options:

    /NOPROGRESS
    Don't output any progress info; useful when redirecting the output to a file.

    /NOLINKS
    Don't follow symbolic links or directory junctions.

    /DEPTH depth
    0: don't list subfolders; 1: list subfolders (default); 2: list subfolders and their subfolders; etc.

For example:

    cyodisk /gib /depth 2

will output sizes in gibibytes at a depth of 2 (subfolders plus their subfolders).

## Platforms

CyoDisk is currently a Windows-only application, and is built using Visual Studio 2015.

## License

### The MIT License (MIT)

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
