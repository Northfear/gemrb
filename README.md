# GemRB port for PS Vita

## Install
Download gemrb_data.zip and gemrb.vpk files from https://github.com/Northfear/gemrb-vita/releases.

Install gemrb.vpk to your Vita. Extract "GemRB" folder from gemrb_data.zip to ux0:data.

Copy original game folder to ux0:data/GemRB/ and edit ux0:data/GemRB/GemRB.cfg file (set correct ```GameType``` and ```GamePath``` parameters. Game auto detection isn't working, so set ```GameType``` manually).

LiveArea launcher supports launching different games from it by selecting appropriate game title.

Here are the game config names and default game folders (where you should copy your game data) that are used by each title (configs should be located in ux0:data/GemRB/):

- Baldur's Gate - ```BG1.cfg```. Default game path: ```ux0:data/GemRB/BG1```
- Baldur's Gate 2 - ```BG2.cfg```. Default game path: ```ux0:data/GemRB/BG2```
- Icewind Dale - ```IWD.cfg```. Default game path: ```ux0:data/GemRB/IWD```
- Icewind Dale 2 - ```IWD2.cfg```. Default game path: ```ux0:data/GemRB/IWD2```
- Planescape: Torment - ```PST.cfg```. Default game path: ```ux0:data/GemRB/PST```

[rePatch reDux0](https://github.com/dots-tb/rePatch-reDux0) OR [FdFix](https://github.com/TheOfficialFloW/FdFix) plugin is required for proper suspend/resume support (only use one at a time).

bg2fixpack is recommended for BG2 (not sure about fixpacks for any other games).

## Building

### Prerequisites
- DolceSDK
- [libSDL 1.2](https://github.com/Northfear/SDL-1.2-vita)
- SDL_mixer
- [libpython](https://github.com/uyjulian/python_vita)
- [libc_bridge](https://github.com/TheOfficialFloW/gtasa_vita/tree/master/libc_bridge)
- [iconv](https://github.com/isage/iconv-vita) (optional)

### Build & installation
```
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$DOLCESDK/share/dolce.toolchain.cmake -DSDL_BACKEND=SDL -DSTATIC_LINK=enabled -DDISABLE_WERROR=enabled -DCMAKE_MAKE_PROGRAM=make -DVITA=true -DUSE_OPENAL=true -DUSE_FREETYPE=true -DCMAKE_BUILD_TYPE=None -DNOCOLOR=1
make
```
Generated VPK file is located in build/gemrb folder. Install it to your Vita system.

Copy folders ```GUIScripts```, ```override``` and ```unhardcoded``` from gemrb folder into ux0:data/GemRB/

Rename build/gemrb/GemRB.cfg.sample into GemRB.cfg, copy it to ux0:data/GemRB/ and change the following options:

```
Bpp=16 #recommended for better performance.
AudioDriver=sdlaudio
CachePath=ux0:data/GemRB/Cache2/
GemRBPath=ux0:data/GemRB/
```

Debug output can be previewed with psp2shell

https://github.com/Cpasjuste/psp2shell

## Port info

### Input

Keyboard input is done with D-Pad (on character creation and game saves. 'a-z', '0-9', 'space' and some special characters for console commands are supported).

- Left - Remove character
- Right - Add new character
- Down - Next character (alphabetically)
- Up - Previous character
- R1, L1 - Switch current character between uppercase/lowercase

### Vita specific options

Pointer movement speed can be changed with ```GamepadPointerSpeed``` parameter in GemRB.cfg.

Use ```Fullscreen=1``` to scale game area to native Vita resolution or ```Fullscreen=0``` to keep game area at the center of the screen.

```VitaBilinear=1``` (option is active only when used with ```Fullscreen=1```) uses bilinear filter while scaling the game surface. ```VitaBilinear=0``` uses the nearest filtering.

```VitaKeepAspectRatio=1``` keeps aspect ratio of original image when scaling. ```VitaKeepAspectRatio=0``` just scales it to 960x544.

Both openal and sdlaudio audio plugins are supported. ```openal``` has enviromental audio support and a bit better audio quality in general. ```sdlaudio``` is using less resources while providing most of the functionality. You can change audio backend by editing config file:

```
AudioDriver=openal
```

### Performance

Widescreen mod is supported, but performance with native resolution can be poor in areas with a lot of characters.

```Bpp=16``` option is recommended for better performance. The game is pretty IO heavy, loading can take quite some time. Disabling walking and combat sounds may reduce stutters during initial load of audio files into the cache.

## Controls
- Left analog stick - Pointer movement
- × - Left mouse button
- ○ - Right mouse button
- □ - Open map (M key)
- △ - Open inventory (I key)
- D-Pad Up - Open character record (R key)
- D-Pad Down - Open journal (J key)
- D-Pad Left - Priest spells (P key)
- D-Pad Right - Wizard spells (W key)
- Right analog stick  - Map scrolling
- R1 - Pause
- L1 - Highlight items
- SELECT - Open menu
- START - Escape

Console:
- L1 + R1 + △ - Open/Close console
- START - Enter command (while console is opened)

You can remap □, △ and D-Pad buttons by editing ```Keymap.ini``` in individual game folder and assigning M, I, R, J, P, W keys to your liking.


# GemRB

[![GitHub build status](https://github.com/gemrb/gemrb/actions/workflows/builder.yml/badge.svg)](https://github.com/gemrb/gemrb/actions/workflows/builder.yml)
[![AppVeyor build status](https://ci.appveyor.com/api/projects/status/k5atpwnihjjiv993?svg=true)](https://ci.appveyor.com/project/lynxlynxlynx/gemrb)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/17070b6b1608402b884123d8ecefa2a4)](https://www.codacy.com/app/gemrb/gemrb?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=gemrb/gemrb&amp;utm_campaign=Badge_Grade)
[![Coverity Badge](https://scan.coverity.com/projects/288/badge.svg)](https://scan.coverity.com/projects/gemrb)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=gemrb_gemrb&metric=alert_status)](https://sonarcloud.io/dashboard?id=gemrb_gemrb)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/3101/badge)](https://bestpractices.coreinfrastructure.org/projects/3101)

## Introduction

[GemRB](https://gemrb.org) (Game Engine Made with preRendered Background) is a portable open-source
reimplementation of the Infinity Engine that underpinned Baldur's Gate,
Icewind Dale and Planescape: Torment. It sports a cleaner design, greater
extensibility and several innovations.
Would you like to create a game like Baldur's Gate?

To try it out you either need some of the ORIGINAL game's data or you can
get a tiny sneak peek by running the included trivial game demo.

The original game data has to be installed if you want to see anything but
the included trivial demo. On non-windows systems either copy it over from
a windows install, use a compatible installer, WINE or extract it manually
from the CDs using the unshield tool.

Documentation can be found on the [website](https://gemrb.org/Documentation),
while the sources are in gemrb/docs/en/ and the 
[gemrb.6 man page](https://gemrb.org/Manpage.html).

If you want to help out, start by reading this
list of [options, tips and priorities](https://github.com/gemrb/gemrb/blob/master/CONTRIBUTING.md).

## Supported platforms

Architectures and platforms that successfully run or ran GemRB:
* Linux x86, x86-64, ppc, mips (s390x builds, but no running info)
* FreeBSD, OpenBSD, NetBSD
* MS Windows
* various Macintosh systems (even pre x86)
* some smart phones (Symbian, Android, other Linux-based, iOS)
* some consoles (OpenPandora, Dingoo)
* some exotic OSes (ReactOS, SyllableOS, Haiku, AmigaOS)

If you have something to add to the list or if an entry doesn't work any more, do let us know!

## Requirements

See the INSTALL [file](https://github.com/gemrb/gemrb/blob/master/INSTALL).

## Contacts

There are several ways you can get in touch:
* [Homepage](https://gemrb.org)
* [GemRB forum](https://www.gibberlings3.net/forums/forum/91-gemrb/)
* [IRC channel](http://webchat.freenode.net/?channels=GemRB), #GemRB on the FreeNode IRC network
* [Discord channel](https://discord.gg/64rEVAk) (Gibberlings3 server)
* [Bug tracker](https://github.com/gemrb/gemrb/issues/new/choose)


## Useful links

Original engine research and data manipulation software:
* [IESDP](https://gibberlings3.github.io/iesdp/), documentation for the Infinity Engine file formats and more
* [Near Infinity](https://github.com/NearInfinityBrowser/NearInfinity/wiki), Java viewer and editor for data files
* [DLTCEP](https://www.gibberlings3.net/forums/forum/137-dltcep/), MS Windows viewer and editor for data files
* [iesh](https://github.com/gemrb/iesh), IE python library and shell (for exploring data files)

Tools that can help with data installation:
* [WINE](http://www.winehq.org), Open Source implementation of the Windows API, useful for installing the games
* [Unshield](http://synce.sourceforge.net/synce/unshield.php), extractor for .CAB files created by InstallShield
