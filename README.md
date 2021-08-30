
<h1><img src="https://github.com/galdar496/heatray/blob/master/Resources/logo.png" />    Heatray</h1>

**Note**: *Heatray is currently being completely rewritten with an emphasis on being physically correct, statistically accurate, and with lessons learned from years of building production offline and realtime renderers.*

## Overview
Heatray is a C++ path tracer utilizing [OpenRL](https://en.wikipedia.org/wiki/OpenRL) to perform raytracing. An OpenGL viewer is used to visualize the passes as they are performed in realtime.
## Latest Image
![](https://github.com/galdar496/heatray/blob/master/ExampleImages/latest.PNG)
## Features
* Progressive rendering for quick visualization
* Quasi-Monte Carlo via various low-discrepancy sequences
* Next-event estimation
* Environment lighting
* Depth of field
	* Configurable Bokeh shapes
* ACES filmic tonemapping
* Interactive UI via [imgui](https://github.com/ocornut/imgui)
* Materials:
    * Physically-based
    * Glass
## Theory
Checkout [this link](http://galdar496.github.io/heatray/) to get an overview of the mathematics behind Heatray.
## Meet the Developers
[Cody White](https://www.linkedin.com/in/cody-white-78476019/)

[Aaron Dwyer](https://www.linkedin.com/in/aadwyer/)
## Notes
Heatray was written and developed for macOS Catalina and Windows 10. All libraries required to run the code are included in the repository.
