# test-case
AMD R600 MESA fails to compile shader

This was based upon the GLSLnoise sample released by Stefan Gustavson (stegu@itn.liu.se)
into the public domain / MIT licensed and I have repurposed it since I am using the noise
library in another project.

You'll need GLFW installed to build:

	sudo apt-get install libglfw-dev
	
To build:

	make linux
	
To run:

	./GLSLnoise
	
Expected output:

![Jupiter-ish](/output.png?raw=true "Jupiter-ish")
