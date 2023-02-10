add_library(RLWrapper INTERFACE
    Shader.h
    Error.h
    Buffer.h
    Program.h
    Texture.h
    Framebuffer.h
    PixelPackBuffer.h
    Primitive.h
)

set_target_properties(RLWrapper PROPERTIES LINKER_LANGUAGE CXX)
