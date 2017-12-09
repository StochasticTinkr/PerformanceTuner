# Dynamic Performance for Fallout 4
Based originally on xwise dynamic performance tuner.  Mostly rewritten, and upgraded to Fallout 4 version 1.10.40.

Built with Visual Studio 2015

## Change Log
* 0.9.4 (Supports arbitray Fallout 4 versions, with additional ini files for the version)
  * Added ability to configure V-Sync overrides, including separate ones for "paused" "loading".
  * BUGFIX: Copy and paste error with settings the new settings names for momentum. You should be able to set them now.
* 0.9.3 (Supports arbitray Fallout 4 versions, with additional ini files for the version)
  * Added configuration to manage how fast the shadow dir distance changes. See "momentum" in settings.
* 0.9.2 (Supports arbitray Fallout 4 versions, with additional ini files for the version)
  * Moved the addresses out of the dll, into separate version-specific ini files.
* 0.9.1 (Supports Fallout 4 1.10.40)
  * Update README to include license and settings explanation
  * Add support for "bCapFramerate" and "bAdjustGRQuality" settings. (They were always enabled)
* 0.9.0 (Supports Fallout 4 1.10.40)
  * Initial public release with support for Fallout 4 1.10.40 

## Features
* Framerate cap - Will limit framerate.
* Load Acceleration - Removes framerate capping on loading screens to reduce load time.
* Adjust shadow distance as needed.
* Adjust godray/volumnetric quality. 

## Settings
This is an example of the ini file. The performance tuner will look for a while "dynaperf.ini" in the same directory as the Fallout4.exe


    [Simple]
    ; Target FPS is the highest FPS that the game will be allowed to run at, and is also the baseline for 100% load.
    ; This value is converted to microsonds. I call this the targetFrameTime.
    ; For example, with an FPS of 60, you have 16666us per frame. 
    fTargetFPS=60 
    
    ; Cap Framerate will control whether or not we sleep to cap the framerate. 
    bCapFramerate=1 
    
    ; Target Load is the percentage of desired "work" time for the target FPS. This can be > 100 if you want.
    ; The shadow distance is adjusted (within the bounds below) so that 
    ; "targetFrameTime * fTargetLoad/100 = game engine work time"
    fTargetLoad=98.0 

    ; The Momentum is how fast the shadow distance is changing, either up or down.  If the current frame-time is higher than the 
    ; target frame time (eg, the load is too high), the momentum starts in the negative direction, accerating until it reaches the minumum momentum value.
    ; If, instead, the current frame-time is lower (the load can increase), we start the momentum in the positive direction, accerating until it goes until
    ; it reaches the maximum value.
    ; Either way, if the current frame time crosses over the target frame time, the momentum is reset in the proper direction.

    ; Accelerate at 10% every time we're continuing downward.
    fMomentumDownAcceleration=1.1 
    ; Never go faster than 1000/frame downward.
    fMinMomentum=-1000 

    ; Accelerate at 2.5% ever time we're continuing upward.
    fMomentumUpAcceleration=1.025
    ; Never go faster than 500/frame upward.
    fMaxMomentum=500
	    
    ; fShadowDirDistance Min/Max set limits on what Performance Tuner will set shadow distance to.   
    fShadowDirDistanceMin=2500  
    fShadowDirDistanceMax=12000 
    
    ; bAdjustGRQuality will enable/disable updating of the GR quality.  
    bAdjustGRQuality=1
    ; The GR quality shadow distances tell the performance tuner what to set the quality to, given the current shadow
    ; distance.  It will choose the highest quality for the current shadow distance. 
    fGRQualityShadowDist1=3000  
    fGRQualityShadowDist2=6000  
    fGRQualityShadowDist3=10000 
    
    [Advanced]
    ; LoadCapping Enables FPS cap when in the loading screen.  
    ; Disabling FPS cap during load can speed up load times. 
    ; Works best if you don't use vsync, or set it to "Fast" in Nvidia Control Panel. 
    bLoadCapping=0 

    ; You can override the V-Sync behavior.  -1 means to use the default value from FO4's config, 0 means disable, and 1 means enable.
    ; You can use different settings for the override during load and paused states as well.

    ; This setting controls the VSync override for non-loading and non-paused states.
    ; Default's to -1.
    iPresentIntervalOverride=-1
    
    ; This setting controls the VSync override during loading screens.  
    ; Defaults to 0 if you have bLoadCapping=0, or to the value of iPresentIntervalOverride otherwise.
    iLoadingPresentIntervalOverride=0

    ; This setting controls the vSync override during paused states (pipboy, menu, etc...)
    ; Defaults to iPresentIntervalOverride.
    iPausedPresentIntervalOverride=-1

    ; UsePreciseCapping determines whether to have a busy wait to sleep to the microsecond, or to just use 
    ; the built in millisecond sleep.  
    ; I recommend leaving this off, but expirement with it if you wish.
    bUsePreciseCapping=0 
    
    ; Enables the console, and displays some internal state information.
    ; F4SE and some plugins for it will also output to the console if it is enabled
    ; If you use this, I recommend either having a second monitor, or using Borderless mode, so that you can look at it.
    bShowDiagnostics=0

    ; When diagnostics is enabled, a debug message is dumped periodically. You can change the frequency of this setting. 
    ; The higher the number, the more often per second the message shows.  You can also use partial number (eg, 0.5) to show less than once per second.
    fDebugMessageFrequency=1

## Memory Addresses and Fallout 4 versions.
The reason a mod like this breaks whenever Bethesda updates Fallout4.exe is that the memory locations of the variables this mod uses
can be different.

To help mitigate this, and to give users the ability to fix this themselves without recompiling the DLL, I've moved the addresses into ini files.

This mod will look at the Fallout4.exe version information, and then look for a corresponding ini file.  The ini files must be named "fallout4-addresses-w.x.y.z.ini".
where w.x.y.z is replaced by the version number.  For example, for Fallout 4 version 1.10.40, the file should be called "fallout4-addresses-1.10.40.0.ini".

If it is unable to find that file, it will open a console and tell you which file it was actually looking for. 

### Contents of the addresses ini file
Here is the example fallout4-addresses-1.10.50.0.ini

    [Addresses]
    fShadowDirDistance=6769A1C
    iVolumetricQuality=3901FD8
    bGameUpdatePaused=5A9F000
    bIsMainMenu=5B473A8
    bIsLoading=5AF3E4C

### Create a new ini file yourself
I use "CheatEngine" to help me find the locations for these settings.  I told CheatEngine to automatically attach to Fallout4.exe whenever 
it is running, to make it simpler to do the following. I also configure Fallout 4 to use Borderless mode, and set bAlwaysActive=1, to prevent 
the game pausing when I alt-tab to CheatEngine.

For fShadowDirDistance address, I update the "fDirShadowDistance" value to something arbitrary, such as 3512 and start FO4.  I load a save and then
scan for that value as a first scan. Make sure to select "Float".  Quit the game, change the fDirShadowDistance to another arbitrary value.  Restart the game
and scan for the new value.  Repeat until there are < 10 values. Add these values to the address list, in CheatEngine, and try changing them to 0, until
you see the shadows disappear.  Label that one "Shadow Dir Distance" and delete the others.

For iVolumetricQuality, its a lot easier.  In the game, use the console to set the godray quality to arbitrary values; 'gr quality 3143' and scan. This value is an integer, use "4 byte" for the type.
Change the quality setting and rescan until you have exactly one address.  Add this to the CheatEngine address list, and change label it "Godray quality".

bIsLoading, bGameUpdatePaused, and bIsMainMenu are a little harder, but basically the same.  Search for a 4-byte value of either 0 or 1, when it is in that state. for example,
fast travel, search for 1 while the loading screen is on, and then once you've arrived, search for 0.  Do this until you find a suitable address. 
There may be more than one.

Once you've found all the addresses, save your address list.  CheatEngine will create a .ct file, which will contain the offsets (eg. Fallout4.exe+6769A1C). Use the value 
after the + sign for the addresses ini file. 

    
## Contributing
Features, enhancements, and bug-fixes are welcome.  Please match the current coding style.  If you do open a pull-request,
be prepared to receive feedback on the code, and suggestions on alternative approaches.  I'm a software engineer by trade,
and can be a bit pedantic at times. 

Also note, that any contributions will become Copyright of Daniel Pitts, unless another agreement is reached in writing.

## Credits
[xwise](https://github.com/xwize/) - for the first implementation.  
[Daniel "StochasticTinkr" Pitts](https://github.com/StochasticTinkr/) - for converting to OOP style programming, 
and updating the controller algorithm  
 

## License
This version is released with the MIT license, but this license may be changed in future releases.

Copyright 2017 Daniel Pitts

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
