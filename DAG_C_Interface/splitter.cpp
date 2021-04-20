#include "splitter.h"

namespace AceMesh_runtime {
const size_t splitter::max_dim = 5;

splitter::splitter()
{

}

splitter::splitter(size_t dim, ...)
{
	assert(dim <= max_dim);
	std::vector<size_t> dim_chunks;
	va_list list;
	va_start(list, dim);
	for (size_t i = 0; i < dim; ++i)
	{
		dim_chunks.push_back(va_arg(list, size_t));
	}
	va_end(list);

	set_dim_chunks(dim_chunks);
}

splitter::splitter(std::initializer_list<int> list)
{
//	std::lock_guard<std::recursive_mutex> mutex_guard(m);

	std::vector<size_t> dim_chunks;
	for(auto iter = list.begin(); iter != list.end(); ++iter)
		dim_chunks.push_back(*iter);

	set_dim_chunks(dim_chunks);
}

const splitter& splitter::operator=(const splitter & other)
{
	chunks_on_dim.resize(other.dim);
	dim_range_index.resize(other.dim);
	dim_chunk_size.resize(other.dim);

	dim = other.dim;
	chunks_on_dim = other.chunks_on_dim;

	return *this;
}

void splitter::set_dim_chunks(const std::vector<size_t> & dim_chunks)
{
//	std::lock_guard<std::recursive_mutex> mutex_guard(m);
	dim = std::min(dim_chunks.size(), max_dim);
	chunks_on_dim.resize(dim);
	for(size_t i = 0; i < dim; ++i)
	{
		assert(dim_chunks[i] != (size_t)0);
		chunks_on_dim[i] = dim_chunks[i];
	}
}

void splitter::set_dim_chunks(size_t dim, ...)
{
	assert(dim <= max_dim);
	std::vector<size_t> dim_chunks;
	va_list list;
	va_start(list, dim);
	for (size_t i = 0; i < dim; ++i)
	{
		dim_chunks.push_back(va_arg(list, size_t));
	}
	va_end(list);

	set_dim_chunks(dim_chunks);
}

size_t splitter::get_total_chunks()
{
//	std::lock_guard<std::recursive_mutex> mutex_guard(m);
	int ret = 1;
	for (size_t i = 0; i < dim; ++i)
	{
		ret *= chunks_on_dim[i];
	}
	return ret;
}
}