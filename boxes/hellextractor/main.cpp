// Copyright 2024 Michael Fabian 'Xaymar' Dirks <info@xaymar.com>
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
// 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <cinttypes>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <vector>

template<class T, class D = std::default_delete<T>>
struct shared_ptr_with_deleter : public std::shared_ptr<T> {
	explicit shared_ptr_with_deleter(T* t = nullptr) : std::shared_ptr<T>(t, D()) {}

	// Reset function, as it also needs to properly set the deleter.
	void reset(T* t = nullptr)
	{
		std::shared_ptr<T>::reset(t, D());
	}
};

#ifdef WIN32
#include <Windows.h>

struct HANDLE_deleter {
	void operator()(HANDLE p) const
	{
		CloseHandle(p);
	}
};
using win32_handle_t = shared_ptr_with_deleter<void, HANDLE_deleter>;

#else
#pragma error("Not supported yet, do some work to make it work.")
#endif

namespace hd2 {
	// Models are incredibly tiny, less than 1/10th of a world unit.
	// So we scale them up to not lose precision during text export.
	constexpr float mesh_scale = 100.;

	union lodoffset_u {
		uint8_t raw[4];
		struct {
			uint32_t offset;
		} s;
	};

	union lodinfo_u { // 0x1B0h, 432 bytes
		uint8_t raw[0x1B0];
		struct {
			uint32_t __unk00[88];
			uint32_t vertex_count;
			// Not sure what this means exactly:
			// - 20 bytes:
			//   - [X, Y, Z], [U, V] (all float32)
			// - 28 bytes:
			//   - [X, Y, Z], [U, V], [U, V] (all float32)
			// - 36 bytes:
			//   - [X, Y, Z], [U, V], [U, V], [U, V] (all float32)
			//   - [X, Y, Z], [U, V, W], [nX, nY, nZ] (all float32)
			uint32_t vertex_stride;
			uint32_t __unk01[8];
			uint32_t index_count;
			uint32_t __unk02[5];
			uint32_t vertex_offset;
			uint32_t vertex_size; // vertex_count * vertex_stride
			uint32_t index_offset;
			uint32_t index_size; // index_count * 2.
		} s;
	};

	union lodtable_u {
		uint8_t raw[4];
		struct {
			uint32_t count;
		} s;
	};

	union meshoffset_u {
		uint8_t raw[4];
		struct {
			uint32_t offset;
		} s;
	};

	union meshinfo_u { // 0x94h, 148 bytes
		uint8_t raw[0x94];
		struct {
			uint32_t __unk00[8];
			// Seems to be some sort of bitfield?
			// 0000 0000 0010 (2)
			// - file had 1 LOD, 4 meshes
			// 0000 0000 1010 (10)
			// - file had 3 LODs
			// 0001 0000 0000 (256)
			// - file had 3 LODs
			// 0001 0000 0011 (259)
			// - file only has a single LOD
			// 0001 0000 1001 (265)
			// - file had 3 LODs
			// - file had 1 LOD, 4 meshes
			//
			// 000X 0000 X0XX
			//    |      | ^^- LOD Index? Maximum would be 3 then (0 to 3)
			//    |      ^- Set for non-zero LOD Index.
			//    ^- Set for non-highest LOD Index.
			uint32_t flags;
			uint32_t __unk01[24];
			// Offset relative to LOD vertex offset.
			uint32_t vtx_offset;
			uint32_t vtx_count;
			// Offset relative to LOD index offset.
			uint32_t idx_offset;
			uint32_t idx_count;
		} s;
	};

	union meshtable_u {
		uint8_t raw[4];
		struct {
			uint32_t count;
		} s;
		// Followed by a list of uint32_t offsets;
		// Followed by the actual data (raw address + offset)
		// followed by crc information.
		// Followed by 4 bytes zero.
	};

	union mesh_u {
		uint8_t raw[0x74];
		struct {
			uint32_t __unk00[22];
			uint32_t __unk_offset00;
			uint32_t lod_info_offset;
			uint32_t __unk_offset01;
			uint32_t mesh_info_offset;
			uint32_t __unk01[2];
			uint32_t __unk_offset02;
		} s;
	};

	struct index_t {
		uint16_t index;
	};

	struct vertex20_t {
		float x, y, z;
		float u, v;
	};

	struct vertex28_t {
		float x, y, z;
		float nx, ny;
		float u, v;
	};

	struct vertex36_t {
		float x, y, z;
		float nx, ny, nz;
		float u, v, w;
	};

	struct lodinfo_t {
		lodinfo_u const* raw;

		// Indices
		uint32_t index_count;
		uint32_t index_offset;
		uint32_t index_size;
		//std::vector<uint16_t> indices;

		// Vertices
		uint32_t vertex_count;
		uint32_t vertex_stride;
		uint32_t vertex_offset;
		uint32_t vertex_size;
		//std::vector<vertex_t> vertices;
	};

	struct lodtable_t {
		lodtable_u const*               raw;
		std::vector<lodoffset_u const*> raw_offset;
		std::vector<lodinfo_u const*>   raw_info;

		uint32_t               count;
		std::vector<lodinfo_t> info;
	};

	struct meshinfo_t {
		meshinfo_u const* raw;

		// Offset is relative to matching LOD
		uint32_t indices_offset;
		uint32_t indices_count;

		// Offset is relative to matching LOD
		uint32_t vertices_offset;
		uint32_t vertices_count;
	};

	struct meshtable_t {
		meshtable_u const*               raw;
		std::vector<meshoffset_u const*> raw_offset;
		std::vector<uint32_t const*>     raw_crc;
		std::vector<meshinfo_u const*>   raw_info;

		uint32_t                count;
		std::vector<uint32_t>   crc;
		std::vector<meshinfo_t> info;
	};

	struct mesh_t {
		mesh_u const* raw;

		lodtable_t lods;

		meshtable_t meshes;
	};

} // namespace hd2

int main(int argc, const char** argv)
{
	if (argc < 3) {
		return 1;
	}

#ifdef WIN32
	win32_handle_t mesh_info_file(CreateFileA(argv[1], GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL));
	if (mesh_info_file.get() == INVALID_HANDLE_VALUE) {
		return 1;
	}

	win32_handle_t mesh_info_fmap(CreateFileMapping(mesh_info_file.get(), NULL, PAGE_READONLY, 0, 0, NULL));
	if (mesh_info_fmap.get() == INVALID_HANDLE_VALUE) {
		return 1;
	}

	uint8_t const* mesh_info_ptr = reinterpret_cast<uint8_t const*>(MapViewOfFile(mesh_info_fmap.get(), FILE_MAP_READ, 0, 0, 0));
	if (!mesh_info_ptr) {
		return 1;
	}

	win32_handle_t mesh_file(CreateFileA(argv[2], GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL));
	if (mesh_file.get() == INVALID_HANDLE_VALUE) {
		return 1;
	}

	win32_handle_t mesh_fmap(CreateFileMapping(mesh_file.get(), NULL, PAGE_READONLY, 0, 0, NULL));
	if (mesh_fmap.get() == INVALID_HANDLE_VALUE) {
		return 1;
	}

	uint8_t const* mesh_ptr = reinterpret_cast<uint8_t const*>(MapViewOfFile(mesh_fmap.get(), FILE_MAP_READ, 0, 0, 0));
	if (!mesh_ptr) {
		return 1;
	}
#else
#pragma error("Not supported yet, do some work to make it work.")
#endif

	hd2::mesh_t mi;
	mi.raw = reinterpret_cast<decltype(mi.raw)>(mesh_info_ptr);

	{ // Parse LODs
		std::cout << "LODs at:" << std::hex << mi.raw->s.lod_info_offset << std::endl;
		mi.lods.raw = reinterpret_cast<hd2::lodtable_u const*>(mesh_info_ptr + mi.raw->s.lod_info_offset);

		mi.lods.count = mi.lods.raw->s.count;
		mi.lods.info.resize(mi.lods.count);

		mi.lods.raw_offset.resize(mi.lods.count);
		mi.lods.raw_info.resize(mi.lods.count);

		// Map raw information to memory.
		for (size_t idx = 0; idx < mi.lods.count; idx++) {
			mi.lods.raw_offset[idx] = reinterpret_cast<hd2::lodoffset_u const*>(reinterpret_cast<uint8_t const*>(mi.lods.raw) + sizeof(hd2::lodtable_u)) + idx;
			mi.lods.raw_info[idx]   = reinterpret_cast<hd2::lodinfo_u const*>(reinterpret_cast<uint8_t const*>(mi.lods.raw) + mi.lods.raw_offset[idx]->s.offset);
		}

		// Parse raw information from memory.
		for (size_t idx = 0; idx < mi.lods.count; idx++) {
			auto& li = mi.lods.info[idx];
			auto  ri = mi.lods.raw_info[idx];
			li.raw   = ri;

			li.index_count  = ri->s.index_count;
			li.index_offset = ri->s.index_offset;
			li.index_size   = ri->s.index_size;

			li.vertex_count  = ri->s.vertex_count;
			li.vertex_stride = ri->s.vertex_stride;
			li.vertex_offset = ri->s.vertex_offset;
			li.vertex_size   = ri->s.vertex_size;
		}
	}

	{ // Parse Meshes
		std::cout << "Meshes at:" << std::hex << mi.raw->s.mesh_info_offset << std::endl;
		mi.meshes.raw   = reinterpret_cast<hd2::meshtable_u const*>(mesh_info_ptr + mi.raw->s.mesh_info_offset);
		mi.meshes.count = mi.meshes.raw->s.count;

		mi.meshes.info.resize(mi.meshes.count);
		mi.meshes.crc.resize(mi.meshes.count);

		mi.meshes.raw_offset.resize(mi.meshes.count);
		mi.meshes.raw_info.resize(mi.meshes.count);
		mi.meshes.raw_crc.resize(mi.meshes.count);

		// Map raw information to memory.
		for (size_t idx = 0; idx < mi.meshes.count; idx++) {
			mi.meshes.raw_offset[idx] = reinterpret_cast<hd2::meshoffset_u const*>(reinterpret_cast<uint8_t const*>(mi.meshes.raw) + sizeof(hd2::meshtable_u)) + idx;
			mi.meshes.raw_crc[idx]    = reinterpret_cast<uint32_t const*>(reinterpret_cast<uint8_t const*>(mi.meshes.raw) + sizeof(hd2::meshtable_u) + sizeof(hd2::meshoffset_u) * mi.meshes.count) + idx;
			mi.meshes.raw_info[idx]   = reinterpret_cast<hd2::meshinfo_u const*>(reinterpret_cast<uint8_t const*>(mi.meshes.raw) + sizeof(hd2::meshtable_u) + mi.meshes.raw_offset[idx]->s.offset);
		}

		// Parse raw information from memory.
		for (size_t idx = 0; idx < mi.meshes.count; idx++) {
			auto& crc  = mi.meshes.crc[idx];
			auto  rcrc = mi.meshes.raw_crc[idx];
			crc        = *rcrc;

			auto& mesh  = mi.meshes.info[idx];
			auto  rmesh = mi.meshes.raw_info[idx];
			mesh.raw    = rmesh;

			mesh.indices_offset = rmesh->s.idx_offset;
			mesh.indices_count  = rmesh->s.idx_count;

			mesh.vertices_offset = rmesh->s.vtx_offset;
			mesh.vertices_count  = rmesh->s.vtx_count;
		}
	}

	{ // Write test mesh.
		std::ofstream output{"./test.obj"};

		// For now limit to the first mesh and lod, until we figure out how the rest works.
		auto const& lod  = mi.lods.info[0];
		auto const& mesh = mi.meshes.info[0];

		uint8_t const* vtx_max_addr = mesh_ptr + lod.vertex_offset + lod.vertex_size;
		uint8_t const* vtx_addr     = mesh_ptr + lod.vertex_offset + mesh.vertices_offset;
		for (size_t i = 0; i < mesh.vertices_count; i++, vtx_addr += lod.vertex_stride) {
			switch (lod.vertex_stride) {
			case sizeof(hd2::vertex20_t): {
				hd2::vertex20_t const* vtx = reinterpret_cast<decltype(vtx)>(vtx_addr);
				output << "# " << i << std::endl;
				output << "v " << vtx->x * hd2::mesh_scale << " " << vtx->y * hd2::mesh_scale << " " << vtx->z * hd2::mesh_scale << std::endl;
				output << "vt " << vtx->u << " " << vtx->v << std::endl;
				break;
			}
			case sizeof(hd2::vertex28_t): {
				hd2::vertex28_t const* vtx = reinterpret_cast<decltype(vtx)>(vtx_addr);
				output << "# " << i << std::endl;
				output << "v " << vtx->x * hd2::mesh_scale << " " << vtx->y * hd2::mesh_scale << " " << vtx->z * hd2::mesh_scale << std::endl;
				output << "vn " << vtx->nx << " " << vtx->ny << " " << FLT_EPSILON << std::endl;
				output << "vt " << vtx->u << " " << vtx->v << std::endl;

				break;
			}
			case sizeof(hd2::vertex36_t): {
				hd2::vertex36_t const* vtx = reinterpret_cast<decltype(vtx)>(vtx_addr);
				output << "# " << i << std::endl;
				output << "v " << vtx->x * hd2::mesh_scale << " " << vtx->y * hd2::mesh_scale << " " << vtx->z * hd2::mesh_scale << std::endl;
				output << "vn " << vtx->nx << " " << vtx->ny << " " << vtx->nz << std::endl;
				output << "vt " << vtx->u << " " << vtx->v << " " << vtx->w << std::endl;

				break;
			}
			}
		}

		uint8_t const* idx_max_addr = mesh_ptr + lod.index_offset + lod.index_size;
		uint8_t const* idx_addr     = mesh_ptr + lod.index_offset + mesh.indices_offset;
		for (size_t i = 0; i < mesh.indices_count; i += 3, idx_addr += sizeof(hd2::index_t) * 3) {
			hd2::index_t const* idx0     = reinterpret_cast<decltype(idx0)>(idx_addr);
			hd2::index_t const* idx1     = reinterpret_cast<decltype(idx1)>(idx_addr + 2);
			hd2::index_t const* idx2     = reinterpret_cast<decltype(idx2)>(idx_addr + 4);

			output << "# " << i << std::endl;
			output << "f " << (1 + idx0->index) << " " << (1 + idx1->index) << " " << (1 + idx2->index) << std::endl;
		} // This breaks past the lod.index_size "boundary", but the export is correct.

		output.close();
	}

	std::cout << std::endl;

	return 0;
}
