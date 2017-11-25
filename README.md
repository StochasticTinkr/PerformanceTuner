# Dynamic Performance for Fallout 4
Based originally on xwise dynamic performance tuner.  Mostly rewritten, and upgraded to Fallout 4 version 1.10.40.

Built with Visual Studio 2015

## Change Log
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
    
    ; UsePreciseCapping determines whether to have a busy wait to sleep to the microsecond, or to just use 
    ; the built in millisecond sleep.  
    ; I recommend leaving this off, but expirement with it if you wish.
    bUsePreciseCapping=0 
    
    ; Enables the console, and displays some internal state information.
    ; F4SE and some plugins for it will also output to the console if it is enabled
    ; If you use this, I recommend either having a second monitor, or using Borderless mode, so that you can look at it.
    bShowDiagnostics=0
    
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
