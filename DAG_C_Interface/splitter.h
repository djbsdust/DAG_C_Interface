#ifndef _ACEMESH_SPLITTER_H_
#define _ACEMESH_SPLITTER_H_

#include <vector>
#include <cassert>
#include <initializer_list>
#include <iostream>
#include <stdarg.h>
//#include <mutex>

namespace AceMesh_runtime {
class splitter
{
	std::vector<size_t> chunks_on_dim;

	std::vector<size_t> dim_range_index;
	std::vector<size_t> dim_chunk_size;

	static const size_t max_dim;
	size_t dim;


public:

	splitter();
	splitter(std::initializer_list<int> list);
	splitter(size_t dim, ...);

	explicit splitter(size_t group_size); // auto factorize, yet to impletment.

	void set_dim_chunks(const std::vector<size_t> & dim_chunks);
	void set_dim_chunks(size_t dim, ...);

	size_t get_total_chunks();

	template<typename RangeType>void split(RangeType base_range, std::vector<RangeType> & out_vec);

	const splitter& operator=(const splitter & other);
};


template<typename RangeType> void splitter::split(RangeType base_range, std::vector<RangeType> & out_vec)
{

	size_t range_dim = base_range.get_dim();
	assert(range_dim == this->dim); 
    // Has to be equal! If less, the number of resulted chunks would not match the number of the sub-threads.

	for(size_t i = 0; i < range_dim; ++i)
	{
		dim_chunk_size[i] = (base_range.get_dim_high_end(i) - base_range.get_dim_low_end(i) + 
            chunks_on_dim[i] - 1) / chunks_on_dim[i];
		dim_range_index[i] = 0;
	}
	
	do
	{
		std::vector<int> splitted_range_bounds;
		for(size_t i = 0; i < range_dim; ++i)
		{
			splitted_range_bounds.push_back(std::min(base_range.get_dim_high_end(i), 
                base_range.get_dim_low_end(i) + (int)(dim_range_index[i] * dim_chunk_size[i])));
			splitted_range_bounds.push_back(std::min(base_range.get_dim_high_end(i), 
                base_range.get_dim_low_end(i) + (int)((dim_range_index[i] + 1) * dim_chunk_size[i])));
		}

		// also copy other informations that need to be preserved between base range and splitted range.
		RangeType splitted_range(base_range);
		splitted_range.set_range_bounds(splitted_range_bounds);

		out_vec.push_back(splitted_range);


		dim_range_index[0] += 1;
		
		for(size_t i = 0; i < range_dim - 1; ++i)
		{
			if(dim_range_index[i] == chunks_on_dim[i])
			{
				dim_range_index[i] = 0;
				dim_range_index[i+1] += 1;
			}
			else
			{
				break;

			}
		}
		
	} while(dim_range_index[range_dim - 1] < chunks_on_dim[range_dim - 1]);

//	std::cout << "get_total_chunks() : " << get_total_chunks() << std::endl;	
	assert(out_vec.size() == get_total_chunks());

	return;
}

//extern splitter cur_splitter;
}
#endif
