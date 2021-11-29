//
//  Session.h
//  Heatray
//
//  Support for serialization and deserialization of a Heatray session.
//

#pragma once

//-------------------------------------------------------------------------
// Define session variables to read from an XML session file.

#include <string>
#include <assert.h>

// Session variables that should be present in the session xml file. The parameters are marked as:
// "Variable Group", "Variable Name" (as it is present in the session file) "VariableType", and "Variable Default Value".

#define HEATRAY_SESSION_VARIABLES \
    X(RenderOptions, InteractiveMode, kBool, false)					 \
    X(RenderOptions, MaxRenderPasses, kUInt, 32)					 \
    X(RenderOptions, MaxChannelValue, kFloat, 1.0f)					 \
    X(RenderOptions, Scene, kString, "Multi-Material")				 \
    X(RenderOptions, MaxRayDepth, kUInt, 10)						 \
    X(RenderOptions, SampleMode, kUInt, 0)							 \
    X(RenderOptions, BokehShape, kUInt, 0)							 \
    \
    X(RenderOptions, EnvironmentMap, kString, "studio.hdr")			 \
    X(RenderOptions, EnvironmentBuiltIn, kBool, true)				 \
    X(RenderOptions, EnvironmentExposureCompensation, kFloat, 0.0f)  \
    X(RenderOptions, EnvironmentThetaRotation, kFloat, 0.0f)		 \
    \
    X(RenderOptions, CameraAspectRatio, kFloat, -1.0f)		\
    X(RenderOptions, CameraFocusDistance, kFloat, 1.0f)		\
    X(RenderOptions, CameraFocalLength, kFloat, 50.0f)		\
    X(RenderOptions, CameraApertureRadius, kFloat, 0.0f)	\
    X(RenderOptions, CameraFStop, kFloat, 0.0f)				\
    \
    X(Camera, OrbitDistance, kFloat, 10.0f)    \
    X(Camera, OrbitPhi, kFloat, 10.0f)         \
    X(Camera, OrbitTheta, kFloat, 10.0f)       \
    X(Camera, OrbitTargetX, kFloat, 10.0f)     \
    X(Camera, OrbitTargetY, kFloat, 10.0f)	   \
    X(Camera, OrbitTargetZ, kFloat, 10.0f)     \
    X(Camera, OrbitMaxDistance, kFloat, 10.0f) \
    \
    X(Scene, Units, kUInt, 0)             \
    X(Scene, SwapYZ, kBool, false)        \
    X(Scene, AABB_MinX, kFloat, 0.0f)     \
    X(Scene, AABB_MinY, kFloat, 0.0f)     \
    X(Scene, AABB_MinZ, kFloat, 0.0f)     \
    X(Scene, AABB_MaxX, kFloat, 0.0f)     \
    X(Scene, AABB_MaxY, kFloat, 0.0f)     \
    X(Scene, AABB_MaxZ, kFloat, 0.0f)     \
    X(Scene, DistanceScale, kFloat, 1.0f) \
    \
    X(PostProcessing, TonemapEnable, kBool, false) \
    X(PostProcessing, Exposure, kFloat, 0.0f)      \
    X(PostProcessing, Brightness, kFloat, 0.0f)    \
    X(PostProcessing, Contrast, kFloat, 0.0f)      \
    X(PostProcessing, Hue, kFloat, 0.0f)           \
    X(PostProcessing, Saturation, kFloat, 0.0f)    \
    X(PostProcessing, Vibrance, kFloat, 0.0f)      \
    X(PostProcessing, Red, kFloat, 0.0f)           \
    X(PostProcessing, Green, kFloat, 0.0f)         \
    X(PostProcessing, Blue, kFloat, 0.0f) 
    

////////////////////////////////////////////////////////////////////////////////

class Session
{
public:

    //-------------------------------------------------------------------------
    // All session variables present in the system.
    enum class SessionVariable : size_t
    {
#define X(variableGroup, variableName, type, defaultValue) k##variableName,

        HEATRAY_SESSION_VARIABLES
        kNumSessionVariables

#undef X
    };

    Session();
    ~Session();

    //-------------------------------------------------------------------------
    // Read the session file and populate the list of session variables. Returns
    // true if the file was successfully parsed.
    bool parseSessionFile(const std::string& filename);

    //-------------------------------------------------------------------------
    // Write a new XML session file to disk using the current values of the 
    // session variables. Returns true if the new session file was succesfully
    // written.
    bool writeSessionFile(const std::string& filename) const;
 
    //-------------------------------------------------------------------------
    // Get the value of a specific session variable. If parseSessionFile() has 
    // not been called before making a call to these functions then only the 
    // variable default values will be returned.
    void getVariableValue(const SessionVariable& variable, int& value) const;
    void getVariableValue(const SessionVariable& variable, uint32_t& value) const;
    void getVariableValue(const SessionVariable& variable, bool& value) const;
    void getVariableValue(const SessionVariable& variable, float& value) const;
    void getVariableValue(const SessionVariable& variable, std::string& value) const;

    //-------------------------------------------------------------------------
    // Set the value of a specific session variable. This can be used to 
    // manually override session variable values (usually in the case of writing 
    // the session file to disk).
    void setVariableValue(const SessionVariable& variable, const int value) const;
    void setVariableValue(const SessionVariable& variable, const uint32_t value) const;
    void setVariableValue(const SessionVariable& variable, const bool value) const;
    void setVariableValue(const SessionVariable& variable, const float value) const;
    void setVariableValue(const SessionVariable& variable, const std::string& value) const;

private:

    constexpr static const char* const s_rootSessionNodeName = "HeatraySession";
    constexpr static const char* const s_attributeName = "value";
};

