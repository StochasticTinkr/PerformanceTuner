# PerformanceTuner
Fallout 4 Mod for dynamically reducing shadow distance.

Current release page: http://ec2-13-59-185-176.us-east-2.compute.amazonaws.com/

Built with Visual Studio 2015

Source Overview

* Main.cpp - Implementation of dxgi.dll proxy. Uses two virtual method table hooks on IDXGISwapChain and IDXGIFactory
* Controller.cpp - The main implementation of the performance tuner
* Fallout.cpp - Interface for communicating with the game, memory offsets of game variables etc
