# This INTERFACE thing seems to be the recommended way to do a 
# header-only library.  It allows the headers to show up in an
# IDE.  Weirdly, the sources have to be added in a separate
# command from add_library.
add_library(RLWrapper INTERFACE)
target_sources(RLWrapper INTERFACE
	Shader.h
	Error.h
	Buffer.h
	Program.h
	Texture.h
	Framebuffer.h
	PixelPackBuffer.h
	Primitive.h
)
