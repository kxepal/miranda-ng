Prerequisites
-------------
You will need a `git` utility.

You will need MSYS installation with the following package set:
* mingw32-base
* msys-base
* mingw32-autotools

Please, carefully read the MSYS build instructions at http://www.mingw.org/wiki/Getting_Started

### libsoduim

Download latest libsodium release from https://download.libsodium.org/libsodium/releases/ (use the mingw version). Unpack it, use win32 directory.

### ProjectTox-Core

Clone the `ProjectTox-Core` repository:

    $ git clone https://github.com/irungentoo/ProjectTox-Core.git

Build the `toxcore` (execute all commands using the `msys bash`):

    $ cd /path/to/ProjectTox-Core
    $ autoreconf -i
    $ ./configure --prefix=/absolute/path/to/ProjectTox-Core/dist --with-libsodium-headers=/absolute/path/to/libsodium/include --with-libsodium-libs=/absolute/path/to/libsodium/lib --enable-static=no LDFLAGS=-static-libgcc
    $ make
    $ make install

Now the directory `/absolute/path/to/ProjectTox-Core/dist` have the tox DLL file. You have to prepare the corresponding `.lib` file for the DLL. To do it, start PowerShell and enter a Visual Studio command environment. Then execute the following script inside a `/dist` directory:

    PS>  'EXPORTS' | Out-File .\libtoxcore-0.def -Encoding ascii; dumpbin.exe /exports .\libtoxcore-0.dll | % { ($_ -split '\s+')[4] } | Out-File .\libtoxcore-0.def -Append -Encoding ascii

It prepares the `def` file for the next step - the `lib` tool:

    PS> lib /def:libtoxcore-0.def /out:libtoxcore-0.lib /machine:x86

You'll get the `libtoxcore-0.lib` import library in the current directory. Link it together with `Tox.dll`. When executing, plugin will need the `libsodium-4.dll` and `libtoxcore-0.dll` libraries.