add_library(Shaders STATIC
    accumulator.rlsl
    brdfs.rlsl
    directionalLight.rlsl
    displayGL.frag
    displayGL.vert
    environmentLight.rlsl
    glass.rlsl
    globalData.rlsl
    lightDefines.rlsl
    lightSampling.rlsl
    microfacet.rlsl
    passthrough.rlsl
    perspective.rlsl
    physicallyBased.rlsl
    pointLight.rlsl
    rayAttributes.rlsl
    sequence.rlsl
    showNormals.rlsl
    spotLight.rlsl
    utility.rlsl
    vertex.rlsl
)

set_target_properties(Shaders PROPERTIES LINKER_LANGUAGE CXX)
