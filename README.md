# Nintendo GameBoy Emulator

> [!NOTE]
> At the moment we are thinking about how we can refactor all this mess of code from 20 years ago (and whether it is necessary), because the DMG-CPU chip is being analyzed in a neighboring repository. hurray! not even 25 years have passed :)
> I would like to update this emulator to be clock-accurate and playable (with acceptable fps) at the same time. Edits will be made point by point until the source code is "bent" towards what we see inside the chip.

This is original "black-and-white" GameBoy (DMG-01) emulator.
It supports variety of mappers, boot from internal ROM and has nice-looking LCD effect.
Compatibility is pretty high.

It is very portable with minimal code modifications and can be used for educational purposes.

Controls:
- Arrows: Arrows %)
- A: SELECT
- S: START
- Z: B
- X: A
- Enter: press all buttons at once (to do game save or reset etc.)
- F8: Switch frame limiter on/off (Limited to 1000 FPS)
- F9: Switch LCD effect on/off
- F12: Turn sound on/off

## Build for Windows

Use Windows and VS2022. Open dmgemu.sln and click the Build button with your left heel.

## Build for Linux

In general, the build process is typical for Linux. First you get all the sources from Git. Then you call CMake/make

```
# Get source
# Choose a suitable folder to store a clone of the repository, cd there and then
git clone https://github.com/emu-russia/dmgemu.git
cd dmgemu

# Preliminary squats
mkdir build
cd build
cmake ..
make

# Find the executable file in the depths of the build folder
./dmgemu zelda.gb
```

If something doesn't work, you do it. You have red eyes for a reason. :penguin:

## Build for NetBSD

In general, the build does not differ from the Linux build, but there are some minor peculiarities:
- You need to use `gmake` instead of `make`
- In CMakeLists.txt you need to add the `link_libraries(/usr/pkg/lib)` directive
- In the pch.h header, change the path to the SDL2.h header: `#include </usr/pkg/include/SDL2/SDL2.h>`

## Some screenshots

![whc4e0b23f6744b0](/imgstore/whc4e0b23f6744b0.png)
![whc4e0b23fd946e8](/imgstore/whc4e0b23fd946e8.png)

![whc4e0b23ac916b8](/imgstore/whc4e0b23ac916b8.png)
![whc4e0b23a683fc0](/imgstore/whc4e0b23a683fc0.png)

Enjoy! :smile:

Thanks to `E}|{`, for invaluable help with LCD and sound!

Thanks to Gumpei Yokoi for this great handheld system and Nintendo for bunch of really awesome games.
