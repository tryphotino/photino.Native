set -x
gcc -std=c++11 -shared -DOS_LINUX Exports.cpp Photino.Linux.cpp -o x64/Photino.Native.so `pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.0 libnotify` -fPIC
cp ./x64/Photino.Native.so ../Photino.Test/bin/Debug/net6.0/Photino.Native.so
