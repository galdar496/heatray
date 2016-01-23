//
//  Matrix.h
//  Heatray
//
//  Created by Cody White on 5/9/14.
//
//

#pragma once

#include "Constants.h"
#include "Vector.h"
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>

namespace math
{

 ///
 /// An RxC matrix stored in column-major format.
 ///
 
template <typename T, unsigned int R, unsigned int C>
struct Matrix
{
	T v[R * C]; ///< Ray matrix components in colum-major format.

	Matrix<T, R, C>()
	{
		for (unsigned int i = 0; i < R * C; ++i)
		{
			v[i] = static_cast <T>(0);
		}
	}

	template <typename L>
	Matrix<T, R, C>(const Matrix<L, R, C> &right)
	{
		*this = right;
	}


	bool operator==(const Matrix<T,R,C> &right) const
	{
		for (unsigned int i = 0; i < R * C; ++i)
		{
			if (v[i] != right.v[i])
			{
				return false;
			}
		}

		return true;
	}

    ///
    /// Returns a particular column of the matrix.
    ///
    /// @param c Colum index to return.
    ///
    /// @return The specified colum of the matrix.
    ///
	inline Vector<T, R> Col(const int c) const
	{
		Vector<T, R> result;
		for (unsigned int i = 0, j = c * R; i < R; ++i, ++j)
		{
			result.v[i] = v[j];
		}

		return result;
	}

	///
    /// Returns a particular row of the matrix.
    ///
    /// @param r Index of the row to return.
    ///
    /// @return The specified row of the matrix.
    ///
	inline Vector<T, C> Row(const int r) const
	{
		Vector<T, C> result;
		for (unsigned int i = 0, j = r; i < C; ++i, j += R)
		{
			result.v[i] = v[j];
		}

		return result;
	}

	///
    /// Access a particular matrix element by reference.
    ///
    /// @param r Row that contains the element.
    /// @param c Column that contains the element.
    ///
    /// @return Specified element of the matrix.
    ///
	inline T &operator()(const int r, const int c)
	{
		return v[r + c * C];
	}

    ///
    /// Access a particular matrix element by reference.
    ///
    /// @param r Row that contains the element.
    /// @param c Column that contains the element.
    ///
    /// @return Specified element of the matrix.
    inline const T operator ()(const int r, const int c) const
	{
		return v[r + c * C];
	}

	///
    /// Post multiplies this matrix by another matrix.
    ///
    /// @param right Matrix to multiply the contained matrix by.
    ///
	template<typename S, unsigned int N>
	inline Matrix<T, R, N> operator *(const Matrix<S, C, N> &right) const
	{
		Matrix<T, R, N> result;
		for (unsigned int r = 0; r < R; ++r)
		{
			for (unsigned int c = 0; c < N; ++c)
			{
				result (r,c) = dot(Row (r), right.col(c));
			}
		}

		return result;
	}

    ///
    /// Post multiplies this matrix by another matrix.
    ///
    /// @param right Matrix to multiply the contained matrix by.
    ///
    template<typename S, unsigned int N>
    inline void operator *=(const Matrix<S, C, N> &right)
    {
        for (unsigned int r = 0; r < R; ++r)
        {
            for (unsigned int c = 0; c < N; ++c)
            {
                v[r + c * C] = Row(r).Dot(right.Col(c));
            }
        }
    }

	///
    /// Post multiplies this matrix by a vector.
    ///
    /// @param right Matrix to post-multipy by.
    ///
	template<typename S>
	inline Vector<S, R> operator *(const Vector<S, C> &right) const
	{
		Vector<S, R> result;
		for (unsigned int r = 0; r < R; ++r)
		{
			result.v[r] = Row(r).Dot(right);
		}

		return result;
	}

	///
    /// Copy the contents of the target matrix.
    ///
    /// @param right Matrix to copy.
    ///
	template<typename S>
	inline void operator=(const Matrix<S, R, C> &right)
	{
		for (unsigned int i = 0; i < R * C; ++i)
		{
			v[i] = right.v[i];
		}
	}

	///
    /// Generate an identity matrix.
    ///
	static inline Matrix<T, R, C> Identity()
	{
		Matrix<T, R, C> result;
		for (int i = 0; i < R && i < C; ++i)
		{
			result(i, i) = 1;
		}

		return result;
	}
};

// Implementing this as a free function inside of this header removes the
// circular dependency between Matrix and Vector.
template<typename T, unsigned int N, unsigned int C>
inline Vector<T, C> operator*(const Vector<T,N>& left, const Matrix<T, N, C>& right)
{
	Vector<T, C> result;
	for (int i = 0; i < N; ++i)
	{
		result.v[i] = dot(left, right.col(i));
	}
	return result;
}

typedef Matrix<float,4,4> Mat4f;
typedef Matrix<double,4,4> Mat4d;

} // namespace math
