# Flagball

Flagball is a mod for [Teeworlds][teeworlds], a 2D multiplayer shooter. In
Flagball, there are two goals, one for the red team, the other is for the blue
team. The team goal is located at the back of the enemy base. There is a ball
represented with either a red or a blue flag. The ball can be picked by a tee,
either by touching it (as in the standard CTF game), or by hooking the flag. The
ball carrier cannot shoot ordinary weapons. Instead, when the ball carrier
presses the shoot button, they throw the ball (flag) in the direction they aim.
The ball may also be passed to other players (including the enemy) directly
through hooking. The ball carrier drops the ball on every damage. When the ball
carrier reaches their goal, they score and die to respawn back at their base. If
the carrier throws the ball into the goal, they score too, but their team get
less points. If the server option `sv_fb_owngoal` is activated, the players
that try to score into their own goals will be punished and the enemy team will
score. If the server option `sv_fb_laser_momentum` is set to a nonzero value,
the laser can be used to kick the ball with a given force.

[teeworlds]: https://teeworlds.com/

In the map, the goals are the first two deathmatch spawn points. Playing with
two balls (two flags) is possible.

Use `scripts/commands.pl` to get the list of server configuration variables and
available rcon commands. Pipe it to `scripts/multimarkdown` (requires Perl5
`Text::MultiMarkdown` library) to get the HTML output. You may find the current
list at [my server][commands].

[commands]: http://jini-zh.org/teeworlds/flagball/commands.html

The original author of Flagball is datag. See [the thread on
teeworlds.com][flagball-thread].

[flagball-thread]: https://www.teeworlds.com/forum/viewtopic.php?id=1843

### Random map rotation

Random map rotation makes maps appear in a random sequence. It is activated by
assigning the following value to `sv_maprotation`:

    sv_maprotation "!random flagball.mrt"

Where `flagball.mrt` is a plain text file made of two columns: map name and map
[weight][weight], for example:
    
    fb_sandstorm 0.4
    fb_skyways   0.35
    fb_sol       0.22
    fb_jungle    0.17

Weights are normalized to obtain probabilities.

[weight]: http://en.wikipedia.org/wiki/Weighting

Following is the original teeworlds README.

<a href="https://repology.org/metapackage/teeworlds/versions">
    <img src="https://repology.org/badge/vertical-allrepos/teeworlds.svg" alt="Packaging status" align="right">
</a>

Teeworlds [![CircleCI](https://circleci.com/gh/teeworlds/teeworlds.svg?style=svg)](https://circleci.com/gh/teeworlds/teeworlds) [![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/github/teeworlds/teeworlds?branch=master&svg=true)](https://ci.appveyor.com/project/heinrich5991/teeworlds)
=========

A retro multiplayer shooter
---------------------------

Teeworlds is a free online multiplayer game, available for all major
operating systems. Battle with up to 16 players in a variety of game
modes, including Team Deathmatch and Capture The Flag. You can even
design your own maps!

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software. See license.txt for full license
text including copyright information.

Please visit https://www.teeworlds.com/ for up-to-date information about
the game, including new versions, custom maps and much more.

Originally written by Magnus Auvinen.


Building on Linux or macOS
==========================

Installing dependencies
-----------------------

    # Debian/Ubuntu
    sudo apt install build-essential cmake git libfreetype6-dev libsdl2-dev libpnglite-dev libwavpack-dev python3

    # Fedora
    sudo dnf install @development-tools cmake gcc-c++ git freetype-devel mesa-libGLU-devel pnglite-devel python3 SDL2-devel wavpack-devel

    # Arch Linux (doesn't have pnglite in its repositories)
    sudo pacman -S --needed base-devel cmake freetype2 git glu python sdl2 wavpack

    # macOS
    brew install cmake freetype sdl2


Downloading repository
----------------------

    git clone https://github.com/teeworlds/teeworlds --recurse-submodules
    cd teeworlds

    # If you already cloned the repository before, use:
    # git submodule update --init


Building
--------

    mkdir -p build
    cd build
    cmake ..
    make

On subsequent builds, you only have to repeat the `make` step.

You can then run the client with `./teeworlds` and the server with
`./teeworlds_srv`.


Build options
-------------

The following options can be passed to the `cmake ..` command line (between the
`cmake` and `..`) in the "Building" step above.

`-GNinja`: Use the Ninja build system instead of Make. This automatically
parallizes the build and is generally **faster**. (Needs `sudo apt install
ninja-build` on Debian, `sudo dnf install ninja-build` on Fedora, and `sudo
pacman -S --needed ninja` on Arch Linux.)

`-DDEV=ON`: Enable debug mode and disable some release mechanics. This leads to
**faster** builds.

`-DCLIENT=OFF`: Disable generation of the client target. Can be useful on
headless servers which don't have graphics libraries like SDL2 installed.


Building on Windows with Visual Studio
======================================

Download and install some version of [Microsoft Visual
Studio](https://www.visualstudio.com/) (as of writing, MSVS Community 2017)
with the following components:

* Desktop development with C++ (on the main page)
* Python development (on the main page)
* Git for Windows (in Individual Components → Code tools)

Run Visual Studio. Open the Team Explorer (View → Team Explorer, Ctrl+^,
Ctrl+M). Click Clone (in the Team Explorer, Connect → Local Git Repositories).
Enter `https://github.com/teeworlds/teeworlds` into the first input box. Wait
for the download to complete (terminals might pop up).

Wait until the CMake configuration is done (watch the Output windows at the
bottom).

Select `teeworlds.exe` in the Select Startup Item… combobox next to the green
arrow. Wait for the compilation to finish.

For subsequent builds you only have to click the button with the green arrow
again.


Building on Windows with MinGW
==============================

Download and install MinGW with at least the following components:

- mingw-developer-toolkit-bin
- mingw32-base-bin
- mingw32-gcc-g++-bin
- msys-base-bin

Also install [Git](https://git-scm.com/downloads) (for downloading the source
code), [Python](https://www.python.org/downloads/) and
[CMake](https://cmake.org/download/).

Open CMake ("CMake (cmake-gui)" in the start menu). Click "Browse Source"
(first line) and select the directory with the Teeworlds source code. Next,
click "Browse Build" and create a subdirectory for the build (e.g. called
"build"). Then click "Configure". Select "MinGW Makefiles" as the generator and
click "Finish". Wait a bit (until the progress bar is full). Then click
"Generate".

You can now build Teeworlds by executing `mingw32-make` in the build directory.


Building with bam, guides for all operating systems
===================================================

You can also compile Teeworlds with bam, a custom build system. Instructions
for that can be found at https://www.teeworlds.com/?page=docs&wiki=hacking.
