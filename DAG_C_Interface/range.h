#ifndef _ACEMESH_BLOCKRANGE_H_
#define _ACEMESH_BLOCKRANGE_H_

#ifdef __ACEMESH_THREAD_GROUP 
#include <vector>
#include <cassert>
#include <initializer_list>
#include <stdarg.h>

/*
Interface requirement of RangeType:

For a type to be a RangeType, it should contain the following methods.

1. Copy constructor: RangeType(const RangeType& other);
   
   The copy constructor should also copy any information that you want to be preserved
   when the object of RangeType get splitted by an splitter.

2. void set_range_bounds(const std::vector<int> & range_bounds);

   Set the range bounds explicitly by an int vector.
   This is intended to be used in the splitter's split method.
   The splitted ranges are copy constructed from the base range(in order to 
   preserve additional information in RangeType, other than the range bounds, through the 
   splitting process), then the range bounds are explicitly set by this method.

3. int get_dim_low_end(size_t dim) const;

4. int get_dim_high_end(size_t dim) const;
	
5. size_t get_dim() const { return Dim; }
*/

namespace AceMesh_runtime {
const size_t MaxDim = 5;

class range_d
{
//	static const size_t MaxDim = 5;
	int dim_range[MaxDim][2];
	size_t dim;
public:
	range_d(size_t _dim, ...)
	{
		assert(_dim <= MaxDim);
		dim = _dim;

		va_list list;
		va_start(list, _dim);

		for(size_t i = 0; i < dim; ++i)
		{
			dim_range[i][0] = va_arg(list, int);
			dim_range[i][1] = va_arg(list, int);
		}

		va_end(list);
	}

//	explicit range_d(const std::vector<int> & range_bounds);
	range_d(const range_d& other)
	{
		this->dim = other.dim;

		for(size_t i = 0; i < dim; ++i)
		{
			dim_range[i][0] = other.dim_range[i][0];
			dim_range[i][1] = other.dim_range[i][1];
		}
	}


	void set_range_bounds(const std::vector<int> & range_bounds)
	{
		assert(range_bounds.size() % 2 == 0);
		assert(range_bounds.size() == dim * 2);
	
		for(size_t i = 0; i < dim; ++i)
		{
			dim_range[i][0] = range_bounds[i*2];
			dim_range[i][1] = range_bounds[i*2+1];
		}
	}

	inline int get_dim_low_end(size_t dim) const;
	inline int get_dim_high_end(size_t dim) const;
	
	inline size_t get_dim() const;

};

/*range_d::range_d(const range_d& other)
{
	this->dim = other.dim;

	for(size_t i = 0; i < dim; ++i)
	{
		dim_range[i][0] = other.dim_range[i][0];
		dim_range[i][1] = other.dim_range[i][1];
	}
}

void range_d::set_range_bounds(const std::vector<int> & range_bounds)
{
	assert(range_bounds.size() % 2 == 0);
	assert(range_bounds.size() == dim * 2);
	
	for(size_t i = 0; i < dim; ++i)
	{
		dim_range[i][0] = range_bounds[i*2];
		dim_range[i][1] = range_bounds[i*2+1];
	}
}*/

/*range_d::range_d(const std::vector<int> & range_bounds)
{
	assert(range_bounds.size() % 2 == 0);
	this->dim = range_bounds.size() / 2;
	
	for(size_t i = 0; i < dim; ++i)
	{
		dim_range[i][0] = range_bounds[i*2];
		dim_range[i][1] = range_bounds[i*2+1];
	}
}*/

inline int range_d::get_dim_low_end(size_t dim_index) const
{
	assert( dim_index >= 0 && dim_index < dim );
	return dim_range[dim_index][0];
}

inline int range_d::get_dim_high_end(size_t dim_index) const
{
	assert( dim_index >= 0 && dim_index < dim );
	return dim_range[dim_index][1];
}

inline size_t range_d::get_dim() const 
{ 
	return dim;
}


template<size_t Dim> class range
{
	int dim_range[Dim][2];
	
public:
	explicit range(const std::vector<int> & range_bounds);
	explicit range(std::initializer_list<int> list);

//	void set_range_bounds(const std::vector<int> & range_bounds);
//	range(const range & other);
//	range();

	inline int get_dim_low_end(size_t dim) const;
	inline int get_dim_high_end(size_t dim) const;
	
	inline size_t get_dim() const { return Dim; }

};



template<size_t Dim> range<Dim>::range(std::initializer_list<int> list)
{
	assert(list.size() == Dim * 2);

	int i = 0;
	for(auto iter = list.begin(); iter != list.end();)
	{
		dim_range[i][0] = *iter++;
		dim_range[i++][1] = *iter++;
	}

}


/*template<size_t Dim> range<Dim>::range(const range & other)
{
	for (size_t i = 0; i < Dim; ++i)
	{
		dim_range[i][0] = other.dim_range[i][0];
		dim_range[i][1] = other.dim_range[i][1];
	}
}*/


template<size_t Dim> range<Dim>::range(const std::vector<int> & range_bounds)
{
	assert(range_bounds.size() >= Dim * 2);
	
	for(size_t i = 0; i < Dim; ++i)
	{
		dim_range[i][0] = range_bounds[i*2];
		dim_range[i][1] = range_bounds[i*2+1];
	}
}

/*
template<size_t Dim> void range<Dim>::set_range_bounds(const std::vector<int> & range_bounds)
{
	assert(range_bounds.size() >= Dim * 2);
	
	for(size_t i = 0; i < range_bounds.size(); i+=2)
	{
		dim_range[i][0] = range_bounds[i];
		dim_range[i][1] = range_bounds[i+1];
	}
}
*/

template<size_t Dim> int range<Dim>::get_dim_low_end(size_t dim_index) const
{
	assert( dim_index >= 0 && dim_index < Dim );
	return dim_range[dim_index][0];
}

template<size_t Dim> int range<Dim>::get_dim_high_end(size_t dim_index) const
{
	assert( dim_index >= 0 && dim_index < Dim );
	return dim_range[dim_index][1];
}
}
#endif

#endif
