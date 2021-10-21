
<h1><img src="https://github.com/galdar496/heatray/blob/master/Resources/logo.png" />    Heatray</h1>

**Note**: *Heatray is currently being completely rewritten with an emphasis on being physically correct, statistically accurate, and with lessons learned from years of building production offline and realtime renderers.*

Join our discord server! https://discord.gg/Ux9G7QR2vP

## Overview
Heatray is a C++ path tracer utilizing [OpenRL](https://en.wikipedia.org/wiki/OpenRL) to perform raytracing. An OpenGL viewer is used to visualize the passes as they are performed in realtime.
## Latest Images
![](https://github.com/galdar496/heatray/blob/master/ExampleImages/example1.PNG)
![](https://github.com/galdar496/heatray/blob/master/ExampleImages/example2.png)
![](https://github.com/galdar496/heatray/blob/master/ExampleImages/example3.png)
![](https://github.com/galdar496/heatray/blob/master/ExampleImages/example4.png)
![](https://github.com/galdar496/heatray/blob/master/ExampleImages/example5.png)
![](https://github.com/galdar496/heatray/blob/master/ExampleImages/example6.png)
![](https://github.com/galdar496/heatray/blob/master/ExampleImages/example7.png)
![](https://github.com/galdar496/heatray/blob/master/ExampleImages/example8.png)
## Features
* Progressive rendering for quick visualization
* Quasi-Monte Carlo via various low-discrepancy sequences
* Next-event estimation
* Environment lighting
* Depth of field
	* Configurable Bokeh shapes
* ACES filmic tonemapping + color processing pipeline
* Interactive UI via [imgui](https://github.com/ocornut/imgui)
* Materials:
    * Physically-based
	   * Roughness/Metallic
	   * Clearcoat
	   * Multiscattering
    * Glass
## Theory
Checkout [this link](http://galdar496.github.io/heatray/) to get an overview of the mathematics behind Heatray.
## Meet the Developers
[Cody White](https://www.linkedin.com/in/cody-white-78476019/)

[Aaron Dwyer](https://www.linkedin.com/in/aadwyer/)
## Notes
Heatray was written and developed for macOS Catalina and Windows 10. All libraries required to run the code are included in the repository.
