CC=c++
CFLAGS=-std=c++2a -Wall -O2 -shared -fPIC
# CFLAGS=-std=c++2a -Wall -O0 -g -shared -fPIC

SRC=./Photino.Native
SRC_SHARED=$(SRC)/Shared
SRC_WIN=$(SRC)/Windows
SRC_MAC=$(SRC)/macOS
SRC_LIN=$(SRC)/Linux

DEST_PATH=./lib

DEST_PATH_PROD=$(DEST_PATH)/prod
DEST_PATH_DEV=$(DEST_PATH)/dev
DEST_FILE=Photino.Native

all:
	# "make all is unavailable, use [windows|mac|linux](-dev)."

windows: clean build-photino-windows
mac: clean build-photino-mac
linux: clean build-photino-linux

windows-dev: clean-dev build-photino-windows
mac-dev: clean-dev build-photino-mac-dev
linux-dev: clean-dev install-linux-dependencies build-photino-linux-dev

build-photino-windows:
	# "build-photino-windows is not defined"

build-photino-windows-dev:
	# "build-photino-windows-dev is not defined"

build-photino-mac:
	# "build-photino-mac is not defined"

build-photino-mac-dev:
	cp $(SRC)/Exports.cpp $(SRC)/Exports.mm &&\
	$(CC) -o $(DEST_PATH_DEV)/$(DEST_FILE).dylib\
		  $(CFLAGS)\
		  -framework Cocoa\
		  -framework WebKit\
		  -framework UserNotifications\
		  $(SRC)/Photino.Mac.AppDelegate.mm\
		  $(SRC)/Photino.Mac.UiDelegate.mm\
		  $(SRC)/Photino.Mac.UrlSchemeHandler.mm\
		  $(SRC)/Photino.Mac.NSWindowBorderless.mm\
		  $(SRC)/Photino.Mac.mm\
		  $(SRC)/Exports.mm &&\
	rm $(SRC)/Exports.mm

install-linux-dependencies:
	sudo apt-get update\
	&& sudo apt-get install libgtk-3-dev libwebkit2gtk-4.0-dev libnotify-dev

build-photino-linux:
	# "build-photino-linux is not defined"

build-photino-linux-dev:
	$(CC) -o $(DEST_PATH_DEV)/$(DEST_FILE).so\
		  $(CFLAGS)\
		  $(SRC)/Photino.Linux.cpp\
		  $(SRC)/Exports.cpp\
		  `pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.0`

clean:
	rm -rf $(DEST_PATH_PROD)/* & mkdir -p $(DEST_PATH_PROD)

clean-dev:
	rm -rf $(DEST_PATH_DEV)/* & mkdir -p $(DEST_PATH_DEV)