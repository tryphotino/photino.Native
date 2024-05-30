Hello Photino Community! We have a new poll question, regarding where and how you use Photino:

[PHOTINO USAGE POLL](https://github.com/tryphotino/photino.NET/discussions/172)

# Build native, cross-platform desktop apps

Photino is a lightweight open-source framework for building native,  
cross-platform desktop applications with Web UI technology.

Photino enables developers to use fast, natively compiled languages like C#, C++, Java and more. Use your favorite development frameworks like .NET 5, and build desktop apps with Web UI frameworks, like Blazor, React, Angular, Vue, etc.!

Photino uses the OSs built-in WebKit-based browser control for Windows, macOS and Linux.
Photino is the lightest cross-platform framework. Compared to Electron, a Photino app is up to 110 times smaller! And it uses far less system memory too!

## Photino.Native
Clone and contribute to this project if you would like to expand and enhance the core functionality of Photino. The native C++ projects handles system calls and exposes native OS operations to the parent Photino .NET 5 wrapper, which can be found here:
https://github.com/tryphotino/photino.NET
If you would like to bring photino into another, currently not supported environment OS, or provide support for other languages such as Java, Rust, or Go, you will also likely need to build this repo.

All features contributed to the Photino.Native project require implementation for Windows, macOS, and Linux.

In all other cases you will probbaly not need the source code for this project, but instead should add and consume this library through its Nuget Package:
https://www.nuget.org/packages/Photino.Native/


## Building
The GitHub repository includes .yml files for automated CI/CD builds, packaging and deployments via Azure DevOps Pipelines. Please refer to these files for the latest information on dependencies and build commands.

* Windows - Open the solution in Visual Studio 2019 or later with the **Desktop development with C++** workload installed. The **Linux development with C++** workload is recommended, but not required. You *must* build in x64 configuration (*not* AnyCPU which is the default). You *must* install either the [WebView2 runtime]( https://go.microsoft.com/fwlink/p/?LinkId=2124703 ) or the [Edge Dev Channel]( https://www.microsoftedgeinsider.com/en-us/download ). Beta and Canary should work as well, but that hasn't been verified. Just having Edge Chromium installed will not work. The project will build, but will not work properly in a development environment.
  
* Linux - (Tested with Ubuntu 18.04 and 20.04) Install dependencies: `sudo apt-get update && sudo apt-get install libgtk-3-dev libwebkit2gtk-4.0-dev libnotify-dev`. To compile, run `gcc -std=c++11 -shared -DOS_LINUX Exports.cpp Photino.Linux.cpp -o x64/$(buildConfiguration)/Photino.Native.so 'pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.0 libnotify' -fPIC`

* Mac - Install Xcode. To compile, run `gcc -shared -lstdc++ -DOS_MAC -framework Cocoa -framework WebKit Photino.Mac.mm Exports.cpp Photino.Mac.AppDelegate.mm Photino.Mac.UiDelegate.mm Photino.Mac.UrlSchemeHandler.mm -o x64/$(buildConfiguration)/Photino.Native.dylib`.
