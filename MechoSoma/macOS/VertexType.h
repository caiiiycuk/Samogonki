#pragma once

#include <array>

#include "Md3d.h"

namespace graphics
{

struct VertexType
{
	explicit VertexType(DWORD vertex_type)
	{
		if (vertex_type & D3DFVF_XYZRHW)
		{
			// float, float, float, float
			_offsets[0] = 4 * sizeof(float);
			_vertex_size += _offsets[0];
		}

		if (vertex_type & D3DFVF_XYZ)
		{
			// float, float, float
			_offsets[0] = 3 * sizeof(float);
			_vertex_size += _offsets[0];
		}

		if (vertex_type & D3DFVF_DIFFUSE)
		{
			// DWORD in ARGB order
			_offsets[1] = sizeof(DWORD);
			_vertex_size += _offsets[1];
		}

		if (vertex_type & D3DFVF_SPECULAR)
		{
			// DWORD in ARGB order
			_offsets[2] = sizeof(DWORD);
			_vertex_size += _offsets[2];
		}

		if (vertex_type & D3DFVF_TEX1)
		{
			// float, float
			_offsets[3] = 2 * sizeof(float);
			_vertex_size += _offsets[3];
		}
	}

	[[nodiscard]] size_t get_offset(DWORD vertex_type) const
	{
		if (vertex_type & D3DFVF_XYZRHW)
		{
			return _offsets[0];
		}

		if (vertex_type & D3DFVF_XYZ)
		{
			return _offsets[0];
		}

		if (vertex_type & D3DFVF_DIFFUSE)
		{
			return _offsets[1];
		}

		if (vertex_type & D3DFVF_SPECULAR)
		{
			return _offsets[2];
		}

		if (vertex_type & D3DFVF_TEX1)
		{
			return _offsets[3];
		}

		return 0;
	}

	[[nodiscard]] size_t get_vertex_size() const
	{
		return _vertex_size;
	}

private:
	size_t _vertex_size = 0;
	std::array<size_t, 4> _offsets{0};
};

}
