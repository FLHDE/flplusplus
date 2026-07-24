# flplusplus

flplusplus is a client-side hook for Freelancer providing the following features:

- Timestamps for FLSpew.txt
- Redirecting the FLSpew.txt output to a console window 
- Removed MP3 codec error spam on systems using Wine
- Removed start location warning in FLSpew.txt
- Configurable LOD/Render distance (flplusplus.ini)
- Configurable save game location
- Configurable screenshot location
- PNG screenshots
- Zooming in and out in the Ship Dealer preview window using the scroll wheel
- Automatically regenerated Restart.fl file on startup
- Ability to change the IDS of the "failed to initialize saves directory" error message
- Touchpad support
- Cursor confinement in windowed mode
- Directly connecting to Freelancer Servers (flplusplus.ini) 
- Adjustable VSync and maximum FPS settings

flplusplus also provides the following .ini extensions:

### TTF Loading

In your `FONTS/fonts.ini` file you can create the following section.

```ini
[FontFiles]
path = FONTS/font.ttf
path = FONTS/font2.ttf
```

This will load the files `FONTS/font.ttf` and `FONTS/font2.ttf` into the process, allowing them to be used by Freelancer without installing them on the system.

### Thn Player

With flplusplus.dll in your dacom.ini, you can add `-thn` to your Freelancer.exe launch parameters to have it launch in THN player mode.
This allows you to select an arbitrary .thn script (as long as it is self-contained i.e. intros), and play it with Freelancer's thn engine.

![Screenshot from 2024-04-14 21-43-32](https://github.com/TheStarport/flplusplus/assets/10167247/11626748-4e9e-4cdb-bfa3-8bc37cfed1c0)

## Building

Build on linux using build_linux.sh (CMake 3.13 and mingw-w64 required)

Build on Windows using Visual Studio with CMake

## Thanks

Special thanks to:
- Jason "adoxa" Hood
- Cannon
- Laz
