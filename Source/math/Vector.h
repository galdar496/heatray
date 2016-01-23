//
//  Vector.h
//  Heatray
//
//  Created by Cody White on 5/9/14.
//
//

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

namespace math
{

///
/// Represents a mathematical N-dimensional vector.
///

/// T = type, N = dimensions.
template<typename T, unsigned int N>
struct Vector
{
	T v[N]; ///< Raw vector.

	inline Vector<T,N>()
	{
		for (unsigned int i = 0; i < N; ++i)
		{
			v[i] = 0;
		}
	}

	inline Vector<T,N>(const T x)
	{
		for (unsigned int i = 0; i < N; ++i)
		{
			v[i] = x;
		}
	}

	template <typename S, unsigned int M>
	inline Vector<T,N>(const Vector<S,M>& target)
	{
		for (unsigned int i = 0; i < N && i < M; ++i)
		{
			v[i] = target[i];
		}
		for (unsigned int i = M; i < N; ++i)
		{
			v[i] = 0;
		}
	}

	template <typename S>
	inline Vector<T,N>(const S& target)
	{
		for (unsigned int i = 0; i < N; ++i)
		{
			v[i] = target[i];
		}
	}
    
    ///
    /// Initialize the first three components, set all others to zero.
    ///
    /// @param x First component value.
    /// @param y Second component value.
    /// @param z Third component value.
    ///
    inline Vector<T,N>(const T x, const T y, const T z)
    {
        assert(N >= 3);
        
        v[0] = x;
        v[1] = y;
        v[2] = z;
        
        for (unsigned int ii = 3; ii < N; ++ii)
        {
            v[ii] = (T)0;
        }
    }
    
    /// Operator overloads begin //////////////////////////////

	inline Vector<T,N> operator+(const Vector<T,N>& right) const
	{
		Vector<T,N> result;
		for (unsigned int i = 0; i < N; ++i)
		{
			result.v[i] = v[i] + right.v[i];
		}
		return result;
	}

	inline void operator+=(const Vector<T,N>& right)
	{
		for (unsigned int i = 0; i < N; ++i)
		{
			v[i] += right.v[i];
		}
	}

	inline Vector<T,N> operator-(const Vector<T,N>& right) const
	{
		Vector<T,N> result;
		for (unsigned int i = 0; i < N; ++i)
		{
			result.v[i] = v[i] - right.v[i];
		}
		return result;
	}

	inline void operator-=(const Vector<T,N>& right)
	{
		for (unsigned int i = 0; i < N; ++i)
		{
			v[i] -= right.v[i];
		}
	}

	inline Vector<T,N> operator*(const T& right) const
	{
		Vector<T,N> result;
		for (unsigned int i = 0; i < N; ++i)
		{
			result.v[i] = v[i] * right;
		}
		return result;
	}

	inline Vector <T, N> operator* (const Vector <T, N> &right) const
	{
		Vector <T, N> result;
		for (unsigned int i = 0; i < N; ++i)
		{
			result.v[i] = v[i] * right[i];
		}

		return result;
	}

	inline void operator*=(const T& right)
	{
		for (unsigned int i = 0; i < N; ++i)
		{
			v[i] *= right;
		}
	}

	inline void operator*= (const Vector <T, N> &right)
	{
		for (unsigned int i = 0; i < N; ++i)
		{
			v[i] *= right[i];
		}
	}

	inline Vector<T,N> operator/(const T& right) const
	{
		Vector<T,N> result;
		for (unsigned int i = 0; i < N; ++i)
		{
			result.v[i] = v[i] / right;
		}
		return result;
	}

	inline Vector <T, N> operator/ (const Vector <T, N> &right) const
	{
		Vector <T, N> result;
		for (unsigned int i = 0; i < N; ++i)
		{
			result.v[i] = v[i] / right.v[i];
		}

		return result;
	}

	inline Vector<T,N> operator/=(const T& right)
	{
		T tmp = 1 / right;
		for (unsigned int i = 0; i < N; ++i)
		{
			v[i] *= tmp;
		}
		return *this;
	}

	inline void operator=(const Vector<T,N>& right)
	{
		for (unsigned int i = 0; i < N; ++i)
		{
			v[i] = right.v[i];
		}
	}

	template <typename S, unsigned int M>
	inline Vector<T,N>& operator=(const Vector<S,M>& target)
	{
		for (unsigned int i = 0; i < N && i < M; ++i)
		{
			v[i] = target[i];
		}

		for (unsigned int i = M; i < N; ++i)
		{
			v[i] = 0;
		}

		return *this;
	}

	template<unsigned int M>
	inline void operator=(const Vector<T,M>& right)
	{
		for (unsigned int i = 0; (i < N) & (i < M); ++i)
		{
			v[i] = right.v[i];
		}
	}

	inline bool operator==(const Vector<T,N>& right) const
	{
		for (unsigned int i = 0; i < N; ++i)
		{
			if (v[i] != right.v[i])
			{
				return false;
			}
		}
		return true;
	}

	inline bool operator!= (const Vector <T, N> &right) const
	{
		for (unsigned int i = 0; i < N; ++i)
		{
			if (v[i] != right.v[i])
			{
				return true;
			}
		}

		return false;
	}
    
    inline T& operator[](int i)
    {
        return v[i];
    }
    
    inline T operator[](int i) const
    {
        return v[i];
    }
    
    /// Operator overloads end //////////////////////////////

	///
    /// Set all of the components to zero.
    ///
	static inline Vector<T,N> Zero()
	{
		Vector<T,N> result;
		for (unsigned int i = 0; i < N; ++i)
		{
			result.v[i] = (T)0;
		}
		return result;
	}

    ///
    /// Calculate the magnitude (length) of the vector.
    ///
    T Length() const
    {
        T length = Dot(*this);
        return sqrtf(length);
    }
    
    ///
    /// Calculate the squared magnitude (length) of the vector.
    ///
    T Length2() const
    {
        T length = Dot(*this);
        return length;
    }
    
    ///
    /// Normalize this vector.
    ///
    void Normalize()
    {
        *this /= Length();
    }
    
    ///
    /// Dot product.
    ///
    T Dot(const Vector<T, N> &other) const
    {
        T result = (T)0;
        for (unsigned int i = 0; i < N; ++i)
        {
            result += (v[i] * other.v[i]);
        }
        return result;
    }
    
    ///
    /// Cross product (only defined for 3 component-vectors).
    ///
    Vector<T, 3> Cross(const Vector<T, N> &other) const
    {
        Vector<T,3>	result;
        result[0] = v[1] * other.v[2] - other.v[1] * v[2];
        result[1] = v[2] * other.v[0] - other.v[2] * v[0];
        result[2] = v[0] * other.v[1] - other.v[0] * v[1];
        return result;
    }
};

typedef Vector<int,2> Vec2i;
typedef Vector<int,3> Vec3i;
typedef Vector<int,4> Vec4i;

typedef Vector<float,2> Vec2f;
typedef Vector<float,3> Vec3f;
typedef Vector<float,4> Vec4f;

typedef Vector<double,2> Vec2d;
typedef Vector<double,3> Vec3d;
typedef Vector<double,4> Vec4d;

typedef Vector<unsigned int,2> Vec2u;
typedef Vector<unsigned int,3> Vec3u;
typedef Vector<unsigned int,4> Vec4u;

typedef Vector<unsigned short,2> Vec2us;
typedef Vector<unsigned short,3> Vec3us;
typedef Vector<unsigned short,4> Vec4us;

} // namespace math



