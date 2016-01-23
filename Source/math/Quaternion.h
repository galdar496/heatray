//
//  Quaternion.h
//  Heatray
//
//  Created by Cody White on 5/9/14.
//
//

#pragma once

#include "Matrix.h"
#include "Vector.h"
#include <math.h>
#include <float.h>

namespace math
{
  
///
/// Quaternion object
///

template <typename T>
class Quaternion
{
	public:

		///
        /// Definitions for internal use.
        ///
		typedef Vector<T, 3>    QVec;
		typedef Matrix<T, 4, 4> QMatrix;

		Quaternion()
		{
			// Load the identity quaternion by default.
			Identity();
		}
    
        ~Quaternion()
        {
        }

		Quaternion(const Quaternion &rhs)
		{
			*this = rhs;
		}

		///
        /// Initialized constructor.  Set create_rotation to true to turn the new
        /// quaternion into a rotation from the axis and angle passed int.
        ///
        /// @param radians Angle in radians for the quaternion.
        /// @param axis 3D vector for the quaternion.
        /// @param createRotation Flag to immediately turn this quaternion into a rotation quaternion or not.
        ///
		Quaternion(const T radians, const QVec &axis, const bool createRotation = false)
		{
			if (!createRotation)
			{
				m_angle = radians;
				m_axis = axis;
			}
			else
			{
				CreateFromAxisAngle(radians, axis);
			}
		}

        ////// Operator overloads begin ///////////
    
		inline Quaternion<T> operator+(const Quaternion &rhs) const
		{
			Quaternion<T> q(m_angle + rhs.m_angle, m_axis + rhs.m_axis);
			return q;
		}

		inline Quaternion<T> operator+=(const Quaternion &rhs)
		{
			*this = *this + rhs;
			return *this;
		}

		inline Quaternion<T> operator-(const Quaternion &rhs) const
		{
			Quaternion<T> q (m_angle - rhs.m_angle, m_axis - rhs.m_axis);
			return q;
		}

		inline Quaternion<T> operator-=(const Quaternion &rhs)
		{
			*this = *this - rhs;
			return *this;
		}

		inline Quaternion<T> operator*(const Quaternion &rhs) const
		{
			Quaternion<T> q;
			q.m_angle = (m_angle * rhs.m_angle) - m_axis.Dot(rhs.m_axis);
			q.m_axis  = m_axis.Cross(rhs.m_axis) + (rhs.m_axis * m_angle) + (m_axis * rhs.m_angle);
			return q;
		}

		inline Quaternion<T> operator*=(const Quaternion &rhs)
		{
			*this = *this * rhs;
			return *this;
		}

		inline Quaternion<T> operator*(const T scalar) const
		{
			Quaternion<T> q(m_angle * scalar, m_axis * scalar);
			return q;
		}

		inline Quaternion<T> operator*=(const T scalar) const
		{
			*this = *this * scalar;
			return *this;
		}

		inline Quaternion<T> operator/(const T scalar) const
		{
			Quaternion<T> q(m_angle / scalar, m_axis / scalar);
			return q;
		}

		inline Quaternion<T> operator/=(const T scalar) const
		{
			*this = *this / scalar;
			return *this;
		}

		inline Quaternion<T> & operator=(const Quaternion &rhs)
		{
			if (this != &rhs)
			{
				m_angle = rhs.m_angle;
				m_axis = rhs.m_axis;
			}

			return *this;
		}

		inline bool operator==(const Quaternion &rhs) const
		{
			return ((m_angle == m_angle) &&
					(m_axis == m_axis));
		}

		inline bool operator!=(const Quaternion &rhs) const
		{
			return !(*this == rhs);
		}
    
        ////// Operator overloads end ///////////

		///
        /// Get the conjugate of the quaternion.
        ///
		inline Quaternion<T> Conjugate() const
		{
			Quaternion<T> q(m_angle, m_axis * -(T)1.0);
			return q;
		}

		///
        /// Get the inverse of the quaternion.
        ///
		inline Quaternion<T> Inverse() const
		{
			T invMag = (T)1.0 / Magnitude();
			return Conjugate() * invMag;
		}

		///
        /// Invert the quaternion.
        ///
		inline void Invert()
		{
			*this = Inverse(*this);
		}

		///
        /// Normalize the quaternion.
        ///
		inline void Normalize()
		{
			T invMag = (T)1.0 / Magnitude();
            
			m_angle *= invMag;
			m_axis  *= invMag;
		}

		///
        // Load the identity quaternion into this quaternion.
        //
		inline void Identity()
		{
			m_angle = (T)1.0;
			m_axis = QVec((T)0.0, (T)0.0, (T)0.0);
		}

		///
        /// Get the magnitude.
        ///
		inline T Magnitude() const
		{
			return sqrtf((m_angle * m_angle) +
			             (m_axis[0] * m_axis[0]) +
                         (m_axis[1] * m_axis[1]) +
				         (m_axis[2] * m_axis[2]));
		}

		///
        /// Convert the quaternion into a 4x4 column-major matrix.
        ///
        /// @param m Result matrix passed in by reference.
        ///
		inline void ToMatrix(QMatrix &m) const
		{
			m(0, 0) = (T)1.0 - (T)2.0 * (m_axis[1] * m_axis[1] + m_axis[2] * m_axis[2]);
			m(1, 0) = (T)2.0 * (m_axis[0] * m_axis[1] - m_axis[2] * m_angle);
			m(2, 0) = (T)2.0 * (m_axis[0] * m_axis[2] + m_axis[1] * m_angle);
			m(3, 0) = (T)0.0;

			m(0, 1) = (T)2.0 * (m_axis[0] * m_axis[1] + m_axis[2] * m_angle);
			m(1, 1) = (T)1.0 - (T)2.0 * (m_axis[0] * m_axis[0] + m_axis[2] * m_axis[2]);
			m(2, 1) = (T)2.0 * (m_axis[2] * m_axis[1] - m_axis[0] * m_angle);
			m(3, 1) = (T)0.0;

			m(0, 2) = (T)2.0 * (m_axis[0] * m_axis[2] - m_axis[1] * m_angle);
			m(1, 2) = (T)2.0 * (m_axis[1] * m_axis[2] + m_axis[0] * m_angle);
			m(2, 2) = (T)1.0 - (T)2.0 * (m_axis[0] * m_axis[0] + m_axis[1] * m_axis[1]);
			m(3, 2) = (T)0.0;

			m(0, 3) = (T)0.0;
			m(1, 3) = (T)0.0;
			m(2, 3) = (T)0.0;
			m(3, 3) = (T)1.0;
		}

		///
        /// Dot product.
        ///
        /// @param rhs Quaternion to perform the dot product with.
        ///
		inline Quaternion<T> Dot(const Quaternion &rhs) const
		{
			T returnVal = m_angle * rhs.m_angle;
			returnVal += dot(m_axis, rhs.m_axis);
			return returnVal;
		}

		///
        /// Get the angle of rotation in radians.
        ///
		inline T GetAngle() const
		{
			return m_angle;
		}

		///
        /// Get the axis of rotation.
        ///
		inline const QVec & GetAxis() const
		{
			return m_axis;
		}

		///
        /// Create a rotation quaternion based on an angle and axis.
        ///
        /// @param angle Angle to rotate about (in radians).
        /// @param vec 3D vector to rotate about.
        ///
		inline void CreateFromAxisAngle(const T angle, const QVec &vec)
		{
			T halfAngle = angle * static_cast<T>(0.5);
			T sinAngle = sinf(halfAngle);

			// Compute the quaternion values.
			m_axis[0] = vec[0] * sinAngle;
			m_axis[1] = vec[1] * sinAngle;
			m_axis[2] = vec[2] * sinAngle;
			m_angle   = cosf(halfAngle);
		}

        ///
        /// Rotate a vector about this quaternion. This quaternion
        /// is considered to be already unit length (normalized).
        ///
        /// @return The rotated vector.
        ///
	    inline QVec Rotate(const QVec &vec)
		{
            // This is different from the canonical q * p * conjugate(q) for
            // speed reasons.
            // See https://molecularmusings.wordpress.com/2013/05/24/a-faster-quaternion-vector-multiplication/

            QVec v = cross(m_axis, vec) * (T)2.0;
            QVec result = (vec + (v * m_angle));
            result += cross(m_axis, v);
            return result;
		}

	private:

		// Member variables
		T m_angle;		///< Quaternion angle.
		QVec m_axis;	///< Quaternion axis.
};

// Easy defines
typedef Quaternion<float>  Quatf;
typedef Quaternion<double> Quatd;
typedef Quaternion<int>    Quati;

} // namespace math

