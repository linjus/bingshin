# bingshin

Copyright (c) 2013 Choonghwan Lee

bingsin is an OpenGL-based dynamic map library and applications for webOS, .NET and Windows. bingshin is made available under LGPL v3. Please see LICENSE for more details.

## Features
* hardware-accelerated rendering (OpenGL)
* smooth zoom-in/out and movement
* multi-platform support (webOS, .NET and Win32)

## Status
This software was written to implement an offline map for webOS and .NET (WPF). Now that Microsoft releases Bing Maps WPF Control and webOS is not widely used, the author lost interest and, as a result, this project is no longer maintained.

That said, this software is still useful for those who want to run an offline map under their webOS phones.

## How to Use (webOS)
1. Using the downloader, download all the necessary tiles under the target machine
2. Depending on the platform, run the proper map application.

## Implementations
This software consists of three parts:
* libraries : all the core features, such as drawing and moving tiles
* applications : map applications built on libraries, for each platform
* downloader : Silverlight application for downloading map tiles

Here's the list of top-level directories:
* control/ : implements the dynamic map library in C in a platform-independent way
* control_clr/ : implements the .NET-specific dynamic map library in C++/CLI
* control_pdk/ : implements the webOS's PDK-specific dynamic map library in C
* control_win32/ : implements the Win32-specific dynamic map library in C
* control_ios/ : implements the iOS-specific dynamic map library in Objective-C
* clrview/ : implements the .NET map application in C#
* pdk/ : implements the webOS's PDK plugin in C
* mojo/ : implements the webOS map application (in conjunction with pdk/) in Javascript
* winview/ : implements the Windows map application in C++
* downloader/ : implements the downloader using Silverlight in C#

