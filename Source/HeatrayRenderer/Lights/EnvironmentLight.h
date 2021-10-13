//
//  EnvironmentLight.h
//  Heatray
//
//  Handles loading of IBL data and the creation of any necessary OpenRL
//  objects to utilize environment lighting.
//

#pragma once

#include <RLWrapper/Primitive.h>
#include <Utility/TextureLoader.h>

#include <string>

// Forward declarations.
namespace openrl {
class Program;
class Texture;
} // namespace openrl.

class EnvironmentLight
{
public:
	EnvironmentLight();
	~EnvironmentLight() = default;

	///
	/// Alter the source of the environment light. Any primitive bindings
	/// etc will still be valid after this function executes.
	///
	void changeImageSource(const char* path, bool builtInMap);

	///
	/// Switches the internal image data to be a constant bright grey
	/// in order to enable a white furnace test for BRDF testing.
	///
	void enableWhiteFurnaceTest();

	void rotate(const float theta_radians);
	void setExposure(const float exposureCompensation);

	RLprimitive primitive() const { return m_primitive->primitive(); }
private:
	void setUniforms() const;

	std::shared_ptr<openrl::Primitive> m_primitive = nullptr;
	std::shared_ptr<openrl::Texture> m_texture = nullptr;
	std::shared_ptr<openrl::Program> m_program = nullptr;

	float m_exposureCompensation = 0.0f;
	float m_thetaRotation = 0.0f;

	std::string m_textureSourcePath;
};
