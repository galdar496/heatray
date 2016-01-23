//
//  Triangle.h
//  Heatray
//
//  Created by Cody White on 5/12/14.
//
//

#pragma once

#include "../math/Vector.h"

namespace gfx
{

/// Encapsulation of a triangle. Essentially just three vertices.
struct Triangle
{
    /// Default constructor.
    Triangle()
    {
    }
    
    /// Copy constructor.
    Triangle(const Triangle &other)
    {
        vertices[0] = other.vertices[0];
        vertices[1] = other.vertices[1];
        vertices[2] = other.vertices[2];
        
        normals[0] = other.normals[0];
        normals[1] = other.normals[1];
        normals[2] = other.normals[2];
        
        tex_coords[0] = other.tex_coords[0];
        tex_coords[1] = other.tex_coords[1];
        tex_coords[2] = other.tex_coords[2];
        
        tangents[0] = other.tangents[0];
        tangents[1] = other.tangents[1];
        tangents[2] = other.tangents[2];
    }
    
    /// Assignment operator.
    Triangle & operator=(const Triangle &other)
    {
        if (this != &other)
        {
            vertices[0] = other.vertices[0];
            vertices[1] = other.vertices[1];
            vertices[2] = other.vertices[2];
            
            normals[0] = other.normals[0];
            normals[1] = other.normals[1];
            normals[2] = other.normals[2];
            
            tex_coords[0] = other.tex_coords[0];
            tex_coords[1] = other.tex_coords[1];
            tex_coords[2] = other.tex_coords[2];
            
            tangents[0] = other.tangents[0];
            tangents[1] = other.tangents[1];
            tangents[2] = other.tangents[2];
        }
        
        return *this;
    }
    
    /// Calculate the tangents for this triangle and store them into the 'tangents' member variable.
    void calculateTangents()
    {
        tangents[0] = getTangent(vertices[0], vertices[1], vertices[2], tex_coords[0], tex_coords[1], tex_coords[2]);
        tangents[0] = tangents[0] - (normals[0] * tangents[0].Dot(normals[0]));
        tangents[0].Normalize();
        
        tangents[1] = getTangent(vertices[1], vertices[2], vertices[0], tex_coords[1], tex_coords[2], tex_coords[0]);
        tangents[1] = tangents[1] - (normals[1] * tangents[1].Dot(normals[1]));
        tangents[1].Normalize();
        
        tangents[2] = getTangent(vertices[2], vertices[0], vertices[1], tex_coords[2], tex_coords[0], tex_coords[1]);
        tangents[2] = tangents[2] - (normals[2] * tangents[2].Dot(normals[2]));
    }
    
    math::Vec3f getTangent(const math::Vec3f &v1, const math::Vec3f &v2, const math::Vec3f &v3,
                           const math::Vec2f &t1, const math::Vec2f &t2, const math::Vec2f &t3)
    {
        math::Vec3f tangent;
        math::Vec3f q1 = v2 - v1;
        math::Vec3f q2 = v3 - v1;
        math::Vec2f s  = t2 - t1;
        math::Vec2f t  = t3 - t1;
        
        float det = 1.0f / (s[0] * t[1] - s[1] * t[0]);
        tangent = (q1 * -t[1] + q2 * s[1]) * det;
        return tangent;
    }
    
    math::Vec3f vertices[3];
    math::Vec3f normals[3];
    math::Vec3f tangents[3];
    math::Vec2f tex_coords[3];
};
    
} // namespace gfx
