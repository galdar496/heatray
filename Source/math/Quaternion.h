//
//  Quaternion.h
//  Heatray
//
//  Created by Cody White on 5/9/14.
//
//

#pragma once

#include <math.h>
#include <iostream>
#include <float.h>
#include "Matrix.h"
#include "Vector.h"

namespace math
{

/**
  * Templated quaternion class.
  */
template <typename T>
class Quaternion
{
	public:

		/** 
		  * Definitions for internal use. 
		  */
		typedef Vector<T, 3>    QVec;
		typedef Matrix<T, 4, 4> QMatrix;

		/** 
		  * Default constructor. 
		  */
		Quaternion()
		{
			// Load the identity quaternion.
			identity();
		}

		/** 
		  * Copy contructor.
		  * @param rhs Quaternion to copy.
		  */
		Quaternion(const Quaternion &rhs)
		{
			*this = rhs;
		}

		/** Initialized constructor.  Set create_rotation to true to turn the new
		  * quaternion into a rotation from the axis and angle passed int.
		  * @param radians Angle in radians for the quaternion.
		  * @param axis 3D vector for the quaternion.
		  * @param create_rotation Flag to immediately turn this quaternion into a rotation quaternion or not.
		  */
		Quaternion(const T radians, const QVec &axis, const bool create_rotation = false)
		{
			if (!create_rotation)
			{
				m_angle = radians;
				m_axis = axis;
			}
			else
			{
				createFromAxisAngle(radians, axis);
			}
		}

		/** 
		  * Default destructor. 
		  */
		~Quaternion() 
		{
		}

		/** 
		  * Operator +.
		  * @param rhs Quaternion to add to this one.
		  */
		inline Quaternion<T> operator+(const Quaternion &rhs) const
		{
			Quaternion<T> tmp(m_angle + rhs.m_angle, m_axis + rhs.m_axis);
			return tmp;
		}

		/** 
		  * Operator +=. 
		  * @param rhs Quaternion to add to this one.
		  */
		inline Quaternion<T> operator+=(const Quaternion &rhs)
		{
			*this = *this + rhs;
			return *this;
		}

		/**
		  * Operator -. 
		  * @param rhs Quaternion to subtract from this one.
		  */
		inline Quaternion<T> operator-(const Quaternion &rhs) const
		{
			Quaternion<T> tmp (m_angle - rhs.m_angle, m_axis - rhs.m_axis);
			return tmp;
		}

		/** 
		  * Operator -=.
		  * @param rhs Quaternion to subtract from this one.
		  */
		inline Quaternion<T> operator-=(const Quaternion &rhs)
		{
			*this = *this - rhs;
			return *this;
		}
		
		/** 
		  * Operator * (quaternion).
		  * @param rhs Quaternion to multiply into this one.
		  */
		inline Quaternion<T> operator*(const Quaternion &rhs) const
		{
			Quaternion<T> tmp;
			tmp.m_angle = (m_angle * rhs.m_angle) - math::dot(m_axis, rhs.m_axis);
			tmp.m_axis  = cross (m_axis, rhs.m_axis) + (rhs.m_axis * m_angle) + (m_axis * rhs.m_angle);
			return tmp;
		}

		/** 
		  * Operator *=.
		  * @param rhs Quaternion to multiply into this one.
		  */
		inline Quaternion<T> operator*=(const Quaternion &rhs)
		{
			*this = *this * rhs;
			return *this;
		}
		
		/** 
		  * Operator / (quaternion).
		  * @param rhs Quaternion to divide from this one.
		  */
		inline Quaternion<T> operator/(const Quaternion &rhs) const
		{
			Quaternion<T> tmp;
			tmp.m_angle = (m_angle * rhs.m_angle) + math::dot(m_axis, rhs.m_axis);
			tmp.m_axis  = cross(m_axis * -(T)1.0, rhs.m_axis) - (rhs.m_axis * m_angle) + (m_axis * rhs.m_angle);
			return tmp;
		}

		/** 
		  * Operator /=.
		  * @param rhs Quaternion to divide from this one.
		  */
		inline Quaternion<T> operator/=(const Quaternion &rhs)
		{
			*this = *this / rhs;
			return *this;
		}

		/** 
		  * Operator * (scalar).
		  * @param scalar Scalar value to multiply into this quaternion.
		  */
		inline Quaternion<T> operator*(const T scalar) const
		{
			Quaternion<T> tmp(m_angle * scalar, m_axis * scalar);
			return tmp;
		}

		/** 
		  * Operator *= (scalar).
		  * @param scalar Scalar value to multiply into this quaternion.
		  */
		inline Quaternion<T> operator*=(const T scalar) const
		{
			*this = *this * scalar;
			return *this;
		}

		/**
		  * Opeartor / (scalar).
		  * @param scalar Scalar value to divide from this quaternion.
		  */
		inline Quaternion<T> operator/(const T scalar) const
		{
			Quaternion<T> tmp(m_angle / scalar, m_axis / scalar);
			return tmp;
		}

		/** 
		  * Operator /= (scalar).
		  * @param scalar Scalar value to divide from this quaternion.
		  */
		inline Quaternion<T> operator/=(const T scalar) const
		{
			*this = *this / scalar;
			return *this;
		}

		/** 
		  * Operator =.
		  * @param rhs Quaternion to set this one equal to.
		  */
		inline Quaternion<T> & operator=(const Quaternion &rhs)
		{
			if (this != &rhs)
			{
				m_angle = rhs.m_angle;
				m_axis = rhs.m_axis;
			}

			return *this;
		}

		/** 
		  * Operator ==. 
		  * @param rhs Quaternion to test equal to.
		  */
		inline bool operator==(const Quaternion &rhs) const
		{
			return ((m_angle == m_angle) &&
					(m_axis == m_axis));
		}

		/** 
		  * Operator!=. 
		  * @param rhs Quaternion to test not equal to.
		  */
		inline bool operator!=(const Quaternion &rhs) const
		{
			return !(*this == rhs);
		}

		/** 
		  * Get the conjugate of the quaternion. 
		  */
		inline Quaternion<T> conjugate() const
		{
			Quaternion<T> tmp(m_angle, m_axis * -(T)1.0);
			return tmp;
		}

		/** 
		  * Get the inverse of the quaternion.
		  */
		inline Quaternion<T> inverse() const
		{
			T inv_mag = (T)1.0 / magnitude();
			return conjugate() * inv_mag;
		}

		/**
		  * Invert the quaternion.
		  */
		inline void invert()
		{
			*this = inverse(*this);
		}

		/** 
		  * Normalize the quaternion. 
		  */
		inline void normalize()
		{
			T inv_mag = (T)1.0 / magnitude();
			m_angle *= inv_mag;
			m_axis *= inv_mag;
		}

		/** 
		  * Load the identity quaternion into this quaternion. 
		  */
		inline void identity()
		{
			m_angle = (T)1.0;
			m_axis = QVec((T)0.0, (T)0.0, (T)0.0);
		}

		/** 
		  * Get the magnitude. 
		  */
		inline T magnitude() const
		{
			return sqrt((m_angle * m_angle) +
			            (m_axis[0] * m_axis[0]) + 
                        (m_axis[1] * m_axis[1]) +
				        (m_axis[2] * m_axis[2]));
		}

		/** 
		  * Convert the quaternion into a 4x4 matrix for use with OpenGL (ModelViewMatrix).
		  * @param matrix Result matrix passed in by reference.
		  */
		inline void toMatrix(QMatrix &matrix) const
		{
			matrix(0, 0) = (T)1.0 - (T)2.0 * (m_axis[1] * m_axis[1] + m_axis[2] * m_axis[2]);
			matrix(1, 0) = (T)2.0 * (m_axis[0] * m_axis[1] - m_axis[2] * m_angle);
			matrix(2, 0) = (T)2.0 * (m_axis[0] * m_axis[2] + m_axis[1] * m_angle);
			matrix(3, 0) = (T)0.0;

			matrix(0, 1) = (T)2.0 * (m_axis[0] * m_axis[1] + m_axis[2] * m_angle);
			matrix(1, 1) = (T)1.0 - (T)2.0 * (m_axis[0] * m_axis[0] + m_axis[2] * m_axis[2]);
			matrix(2, 1) = (T)2.0 * (m_axis[2] * m_axis[1] - m_axis[0] * m_angle);
			matrix(3, 1) = (T)0.0;

			matrix(0, 2) = (T)2.0 * (m_axis[0] * m_axis[2] - m_axis[1] * m_angle);
			matrix(1, 2) = (T)2.0 * (m_axis[1] * m_axis[2] + m_axis[0] * m_angle);
			matrix(2, 2) = (T)1.0 - (T)2.0 * (m_axis[0] * m_axis[0] + m_axis[1] * m_axis[1]);
			matrix(3, 2) = (T)0.0;

			matrix(0, 3) = (T)0.0;
			matrix(1, 3) = (T)0.0;
			matrix(2, 3) = (T)0.0;
			matrix(3, 3) = (T)1.0;
		}

		/** 
		  * Dot product. 
		  * @param rhs Quaternion to perform the dot product with.
		  */
		inline Quaternion<T> dot(const Quaternion &rhs) const
		{
			T return_val = m_angle * rhs.m_angle;
			return_val += dot(m_axis, rhs.m_axis);
			return return_val;
		}

		/** 
		  * Set the angle of rotation. 
		  * @param angle New angle of rotation.
		  */
		inline void setAngle(const T angle)
		{
			m_angle = angle;
		}

		/** 
		  * Set the axis of rotation. 
		  * @param axis New axis of rotation.
		  */
		inline void setAxis(const QVec axis)
		{
			m_axis = axis;
		}

		/** 
		  * Get the angle of rotation. 
		  */
		inline T getAngle() const
		{
			return m_angle;
		}

		/**
		  * Get the axis of rotation. 
		  */
		inline const QVec & getAxis() const
		{
			return m_axis;
		}

		/**
		  * Get an axis with 4 components for matrix multiplication. 
		  */
		inline Vector <T, 4> getAxis4() const
		{
			Vector<T, 4> tmp = m_axis;
			tmp[3] = (T)1.0;
			return tmp;
		}

		/** 
		  * Get the X axis from the quaternion. Generates a matrix, so calling this multiple times is expensive. 
		  */
		inline Vector <T, 3> getXAxis() const
		{
			QMatrix R;
			toMatrix(R);
			Vector<T, 3> result(R(0, 0), R(1, 0), R(2, 0));
			return result;
		}

		/** 
		  * Get the Y axis from the quaternion. Generates a matrix, so calling this multiple times is expensive. 
		  */
		inline Vector<T, 3> getYAxis() const
		{
			QMatrix R;
			toMatrix(R);
			Vector<T, 3> result(R(0, 1), R(1, 1), R(2, 1));
			return result;
		}
		
		/** 
		  * Get the Z axis from the quaternion. Generates a matrix, so calling this multiple times is expensive. 
		  */
		inline Vector<T, 3> getZAxis() const
		{
			QMatrix R;
			toMatrix(R);
			Vector<T, 3> result(R(0, 2), R(1, 2), R(2, 2));
			return result;
		}
		
		/** 
		  * Create a quaternion from 3 Euler angles. 
		  * @param x Rotation in the x.
		  * @param y Rotation in the y.
		  * @param z Rotation in the z.
		  */
		inline void fromEuler(const T x, const T y, const T z)
		{
			T cos_x = cos((T)0.5 * x);
			T cos_y = cos((T)0.5 * y);
			T cos_z = cos((T)0.5 * z);

			T sin_x = sin((T)0.5 * x);
			T sin_y = sin((T)0.5 * y);
			T sin_z = sin((T)0.5 * z);

			m_angle   = (cos_z * cos_y * cos_x) + (sin_z * sin_y * sin_x);
			m_axis[0] = (cos_z * cos_y * sin_x) - (sin_z * sin_y * cos_x);
			m_axis[1] = (cos_z * sin_y * cos_x) + (sin_z * cos_y * sin_x);
			m_axis[2] = (sin_z * cos_y * cos_x) - (cos_z * sin_y * sin_x);
		}

		/** 
		  * Create a rotation quaternion based on an angle and axis. 
		  * @param angle Angle to rotate about.
		  * @param vec 3D vector rotate about.
		  */
		inline void createFromAxisAngle(const T angle, const QVec &vec)
		{
			T angle_val = angle * (T)0.5;
			T sin_angle = sinf(angle_val);

			// Compute the quaternion values.
			m_axis[0] = vec[0] * sin_angle;
			m_axis[1] = vec[1] * sin_angle;
			m_axis[2] = vec[2] * sin_angle;
			m_angle   = cos(angle_val);
		}

		/** 
		  * Convert the Quaternion into a rotation about an axis (Euler angle). 
		  */
		inline Quaternion<T> toAxisAngle() const
		{
			T scale = (m_axis[0] * m_axis[0]) + (m_axis[1] * m_axis[1]) + (m_axis[2] * m_axis[2]);
			Quaternion<T> tmp((T)2.0 * acos (m_angle), m_axis / scale);
			return tmp;
		}

		/** 
		  * Rotate a vector by the quaternion (quaternion must be normalized). 
		  * @param vec 3D vector to rotate the quaternion about.
		  */
	    inline QVec rotate(const QVec &vec)
		{
			Quaternion<T> tmp((T)0.0, vec);
			tmp.normalize();
			Quaternion<T> con(this->conjugate());
			return(*this * tmp * con).getAxis();
		}

		/** Perform SLERP (Spherical linear-interpolation)
		    SLERP is defined as:

					SLERP (p, q, t) = (p * sin ((1 - t) * omega) + q * sin (t * omega)) / sin (omega)

			where p and q are vectors and t is scalar | 0 <= t <= 1.
		 * @param start Quaternion to start from.
		 * @param end Quaternion to end at.
		 * @param t Time during the slerp | 0 <= t <= 1 and t == 0 = start and t == 1 = end.
		 */
		inline Quaternion<T>slerp (const Quaternion<T> &start, const Quaternion<T> &end, const T t)
		{
			T array[4];
			T omega, cos_omega, sin_omega, scale0, scale1;

			cos_omega = start.dot(end);

			// Check the signs
			if (cos_omega < 0.0)
			{
				cos_omega = -cos_omega;
				array[0] = -end.m_axis[0];
				array[1] = -end.m_axis[1];
				array[2] = -end.m_axis[2];
				array[3] = -end.m_angle;
			}
			else
			{
				// Set normally.
				array[0] = end.m_axis[0];
				array[1] = end.m_axis[1];
				array[2] = end.m_axis[2];
				array[3] = end.m_angle;
			}

			// Calculate the coefficients for the SLERP formula.
			if (((T)1.0 - cos_omega) > FLT_EPSILON)
			{
				// This is the standard case, so apply SLERP.
				omega = acos(cos_omega);
				sin_omega = sin(omega);
				scale0 = sin(((T)1.0 - t) * omega) / sin_omega;
				scale1 = sin(t * omega) / sin_omega;
			}
			else
			{
				// The two quaternions are very close, which means that a normal
				// linear interpolation (LERP) will work just fine and avoid a possible
				// division of 0.
				scale0 = (T)1.0 - t;
				scale1 = t;
			}

			// Calculate the resultant quaternion.
			Quaternion<T> result;
			result.m_axis[0] = scale0 * start.m_axis[0] + scale1 * array[0];
			result.m_axis[1] = scale0 * start.m_axis[1] * scale1 * array[1];
			result.m_axis[2] = scale0 * start.m_axis[3] * scale1 * array[2];
			result.m_angle   = scale0 * start.m_angle * scale1 * array[3];

			return result;
		}

		/**
		  * Create the quaternion from a matrix.
		  * @param matrix Resulting matrix.
		  */
		inline void fromMatrix (const QMatrix &matrix)
		{
			T s = (T)0.0;
			T q[4] = {0};
			T trace = matrix(0, 0) + matrix(1, 1) + matrix(2, 2);

			if (trace > 0)
			{
				s = sqrt(trace + 1);
				q[3] = s * (T)0.5;
				s = (T)0.5 / s;
				q[0] = (matrix(1, 2) - matrix(2, 1)) * s;
				q[1] = (matrix(2, 0) - matrix(0, 2)) * s;
				q[2] = (matrix(0, 1) - matrix(1, 0)) * s;
			}

			else
			{
				int nxt[3] = {1, 2, 0};
				int i = 0, j = 0, k = 0;

				if (matrix(1, 1) > matrix(0, 0))
				{
					i = 1;
				}

				if (matrix(2, 2) > matrix(i, i))
				{
					i = 2;
				}

				j = nxt[i];
				k = nxt[j];
				s = sqrt ((matrix(i, i) - (matrix(j, j) + matrix(k, k))) + (T)1.0);

				q[i] = s * (T)0.5;
				s = (T)0.5 / s;
				q[3] = (matrix(j, k) - matrix(k, j)) * s;
				q[j] = (matrix(i, j) + matrix(j, i)) * s;
				q[k] = (matrix(i, k) + matrix(k, i)) * s;
			}

			m_axis = QVec (q[0], q[1], q[2]);
			m_angle = q[3];
		}

		/*********************************************************************************************************
		  								Static helper functions.
        *********************************************************************************************************/

		/** 
		  * Create a new quaternion which incorporates yaw, pitch, and roll into one mega-quaternion for use later.
		  * @param yaw Yaw degrees in radians.
		  * @param pitch Pitch degrees in radians.
		  * @param roll Roll degrees in radians.
		  * @param result Resulting quaternion passed in by reference.
		  */
		static inline void fromYawPitchRoll(const T yaw, const T pitch, const T roll, Quaternion<T> &result)
		{
			Quaternion<T> _yaw   (cosf((-yaw)   * (T)0.5f), vec3f((T)0.0f, sinf((-yaw) * (T)0.5f), (T)0.0f));
			Quaternion<T> _pitch (cosf((-pitch) * (T)0.5f), vec3f(sinf((-pitch) * (T)0.5f), (T)0.0f, (T)0.0f));
			Quaternion<T> _roll  (cosf((-roll)  * (T)0.5f), vec3f((T)0.0f, (T)0.0f, sinf((-roll) * (T)0.5f)));
			result = _yaw * _pitch * _roll;
		}

	private:

		// Member variables
		T m_angle;		// Quaternion angle.
		QVec m_axis;	// Quaternion axis.
};

/**
  * Overload of operator<< so that this quaternion can be output in a stream.
  */
template <typename T>
std::ostream & operator<<(std::ostream &out, const Quaternion<T> &val)
{
	out << "angle: " << val.m_angle << ", axis: " << val.m_axis;
	return out;
}

/** 
  * Calculate the magnitude of the quaternion. 
  * @param q Quaternion to calculate the magnitude of.
  */
template <typename T>
inline T magnitude(const Quaternion <T> &q)  
{
	return q.magnitude();
}

/**
  * Normalize the quaternion.
  * @param q Quaternion to normalize.
  */
template <typename T>
inline Quaternion<T> normalize(const Quaternion<T> &q) 
{
	Quaternion<T> tmp = q;
	tmp.normalize();
	return tmp;
}

/**
  * Dot two quaternions.
  * @param q1 First quaternion.
  * @param q2 Second quaternion.
  */
template <typename T>
inline T dot(const Quaternion<T> q1, const Quaternion<T> q2) 
{
	return q1.dot(q2);
}

/**
  * Get the inverse of this quaternion
  * @param q Quaternion to get the inverse of.
  */
template <typename T>
inline Quaternion<T> inverse(const Quaternion<T> q) 
{
	return q.inverse();
}

// Easy defines
typedef Quaternion<float>  quatf;
typedef Quaternion<double> quatd;
typedef Quaternion<int>    quati;

} // namespace math

