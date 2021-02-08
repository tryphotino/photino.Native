# Build native, cross-platform desktop apps

Photino is a lightweight open-source framework for building native,  
cross-platform desktop applications with Web UI technology.

Photino enables developers to use fast, natively compiled languages like C#, C++, Java and more. Use your favorite development frameworks like .NET 5, and build desktop apps with Web UI frameworks, like Blazor, React, Angular, Vue, etc.!

Photino uses the OSs built-in WebKit-based browser control for Windows, macOS and Linux.
Photino is the lightest cross-platform framework. Compared to Electron, a Photino app is up to 110 times smaller! And it uses far less system memory too!

# Photino.Native
Clone and contribute to this project if you would like to expand and enhance the core functionality of Photino. The native C++ projects handles system calls and exposes native OS operations to the parent Photino .NET 5 wrapper, which can be found here:
https://github.com/tryphotino/photino.NET
If you would like to bring photino into another, currently not supported environment OS, or provide support for other languages such as Java, Rust, or Go, you will also likely need to build this repo.

All features contributed to the Photino.Native project require implementation for Windows, macOS, and Linux.

In all other cases you will probbaly not need the source code for this project, but instead should add and consume this library through its Nuget Package:
https://www.nuget.org/packages/Photino.Native/
