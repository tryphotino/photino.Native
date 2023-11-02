CC=c++
CFLAGS=-std=c++2a -Wall -O2 -shared -fPIC
# CFLAGS=-std=c++2a -Wall -O0 -g -shared -fPIC

SRC=./Photino.Native
SRC_SHARED=$(SRC)/Shared
SRC_WIN=$(SRC)/Windows
SRC_MAC=$(SRC)/macOS
SRC_LIN=$(SRC)/Linux

DEST_PATH=./lib
DEST_PATH_X64=$(DEST_PATH)/x64
DEST_PATH_ARM64=$(DEST_PATH)/arm64

DEST_FILE=Photino.Native

all:
	# "make all is unavailable, use [windows|mac|linux](-x64|arm64)."

windows: clean-x64 build-photino-windows

mac-universal: clean-x64 build-photino-mac-universal

linux-x64: clean-x64 install-linux-dependencies build-photino-linux-x64
linux-arm64: clean-arm64 install-linux-dependencies build-photino-linux-arm64

build-photino-windows:
	# "build-photino-windows is not defined"

build-photino-mac-universal:
	cp $(SRC)/Exports.cpp $(SRC)/Exports.mm &&\
	$(CC) -o $(DEST_PATH_X64)/$(DEST_FILE).dylib\
		  $(CFLAGS)\
		  -arch x86_64\
		  -arch arm64\
		  -framework Cocoa\
		  -framework WebKit\
		  -framework UserNotifications\
		  $(SRC)/Photino.Mac.AppDelegate.mm\
		  $(SRC)/Photino.Mac.UiDelegate.mm\
		  $(SRC)/Photino.Mac.UrlSchemeHandler.mm\
		  $(SRC)/Photino.Mac.NSWindowBorderless.mm\
		  $(SRC)/Photino.Mac.Dialog.mm\
		  $(SRC)/Photino.Mac.mm\
		  $(SRC)/Exports.mm &&\
	rm $(SRC)/Exports.mm

install-linux-dependencies:
	sudo apt-get update\
	&& sudo apt-get install libgtk-3-dev libwebkit2gtk-4.1-dev libnotify4 libnotify-dev

build-photino-linux-x64:
	$(CC) -o $(DEST_PATH_X64)/$(DEST_FILE).so\
		  $(CFLAGS)\
		  $(SRC)/Photino.Linux.Dialog.cpp\
		  $(SRC)/Photino.Linux.cpp\
		  $(SRC)/Exports.cpp\
		  `pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.1 libnotify`

build-photino-linux-arm64:
	$(CC) -o $(DEST_PATH_ARM64)/$(DEST_FILE).so\
		  $(CFLAGS)\
		  $(SRC)/Photino.Linux.Dialog.cpp\
		  $(SRC)/Photino.Linux.cpp\
		  $(SRC)/Exports.cpp\
		  `pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.1 libnotify`

clean-x64:
	rm -rf $(DEST_PATH_X64)/* & mkdir -p $(DEST_PATH_X64)

clean-arm64:
	rm -rf $(DEST_PATH_ARM64)/* & mkdir -p $(DEST_PATH_ARM64)