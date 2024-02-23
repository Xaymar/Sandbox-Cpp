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

struct half {
	uint16_t value;

	half()
	{
		operator=(0.0f);
	}
	half(float v)
	{
		operator=(v);
	}
	half(uint16_t v)
	{
		operator=(v);
	}

	half& operator=(float v)
	{
		auto as_uint = [](const float x) { return *(uint32_t*)&x; };

		// IEEE-754 16-bit floating-point format (without infinity): 1-5-10, exp-15, +-131008.0, +-6.1035156E-5, +-5.9604645E-8, 3.311 digits
		const uint32_t b = as_uint(v) + 0x00001000; // round-to-nearest-even: add last bit after truncated mantissa
		const uint32_t e = (b & 0x7F800000) >> 23; // exponent
		const uint32_t m = b & 0x007FFFFF; // mantissa; in line below: 0x007FF000 = 0x00800000-0x00001000 = decimal indicator flag - initial rounding
		value            = (b & 0x80000000) >> 16 | (e > 112) * ((((e - 112) << 10) & 0x7C00) | m >> 13) | ((e < 113) & (e > 101)) * ((((0x007FF000 + m) >> (125 - e)) + 1) >> 1) | (e > 143) * 0x7FFF; // sign : normalized : denormalized : saturate
	}

	half& operator=(uint16_t v)
	{
		value = v;
	}

	operator float() const
	{
		auto as_float = [](const uint32_t x) { return *(float*)&x; };
		auto as_uint  = [](const float x) { return *(uint32_t*)&x; };

		// IEEE-754 16-bit floating-point format (without infinity): 1-5-10, exp-15, +-131008.0, +-6.1035156E-5, +-5.9604645E-8, 3.311 digits
		const uint32_t e = (value & 0x7C00) >> 10; // exponent
		const uint32_t m = (value & 0x03FF) << 13; // mantissa
		const uint32_t v = as_uint((float)m) >> 23; // evil log2 bit hack to count leading zeros in denormalized format
		return as_float((value & 0x8000) << 16 | (e != 0) * ((e + 112) << 23 | m) | ((e == 0) & (m != 0)) * ((v - 37) << 23 | ((m << (150 - v)) & 0x007FE000))); // sign : normalized : denormalized
	}

	operator uint16_t() const
	{
		return value;
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
	constexpr float mesh_scale = 10.;

	union datatypeoffset_u {
		uint8_t raw[4];
		struct {
			uint32_t offset;
		} s;
	};

	union datatypeinfo_u { // 0x1B0h, 432 bytes
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

	union datatypetable_u {
		uint8_t raw[4];
		struct {
			uint32_t count;
		} s;
	};

	union hashentry_u {
		uint8_t raw[4];
		struct {
			uint32_t value;
		} s;
	};

	union hashtable_u {
		uint8_t raw[4];
		struct {
			uint32_t count;
		} s;
		// Followed by uint32[count] hashes.
		// Followed by 2x uint32[count] unknown static values.
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
			uint32_t __unk02;
			uint32_t datatypeinfo_t[22];
			uint32_t datatype_hash;
			uint32_t __unk03;
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
			uint32_t eof_offset; // this +8 is the end of the file.
			uint32_t mesh_info_offset;
			uint32_t __unk01[2];
			uint32_t hash_table_offset;
		} s;
	};

	struct index_t {
		uint16_t index;
	};

	struct vertex20_t {
		float    x, y, z;
		uint32_t __unk00;
		half     u, v;
	};

	struct vertex28_t {
		float    x, y, z;
		uint32_t __unk00;
		half     u, v;
		uint32_t __unk01[2];
	};

	struct vertex36_t {
		float    x, y, z;
		uint32_t __unk00;
		half     u, v;
		uint32_t __unk01[4];
	};

	struct datatypeinfo_t {
		datatypeinfo_u const* raw;

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

	struct datatypetable_t {
		datatypetable_u const*               raw;
		std::vector<datatypeoffset_u const*> raw_offset;
		std::vector<datatypeinfo_u const*>   raw_info;

		uint32_t                    count;
		std::vector<datatypeinfo_t> info;
	};

	struct meshinfo_t {
		meshinfo_u const* raw;

		uint32_t lod_idx;

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

	struct hashtable_t {
		hashtable_u const*              raw;
		std::vector<hashentry_u const*> raw_datatype_hashes;

		uint32_t              count;
		std::vector<uint32_t> datatype_hashes;
		//std::vector<uint32_t>    __unk00; // Always the same?
		//std::vector<uint32_t>    __unk01; // Always the same?
	};

	struct mesh_t {
		mesh_u const* raw;

		datatypetable_t datatypes;

		meshtable_t meshes;

		hashtable_t hashes;
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
		mi.datatypes.raw = reinterpret_cast<decltype(mi.datatypes.raw)>(mesh_info_ptr + mi.raw->s.lod_info_offset);

		mi.datatypes.count = mi.datatypes.raw->s.count;
		mi.datatypes.info.resize(mi.datatypes.count);

		mi.datatypes.raw_offset.resize(mi.datatypes.count);
		mi.datatypes.raw_info.resize(mi.datatypes.count);

		// Map raw information to memory.
		for (size_t i = 0; i < mi.datatypes.count; i++) {
			mi.datatypes.raw_offset[i] = reinterpret_cast<hd2::datatypeoffset_u const*>(mi.datatypes.raw + 1) + i;
			mi.datatypes.raw_info[i]   = reinterpret_cast<hd2::datatypeinfo_u const*>(reinterpret_cast<uint8_t const*>(mi.datatypes.raw) + mi.datatypes.raw_offset[i]->s.offset);
		}

		// Parse raw information from memory.
		for (size_t i = 0; i < mi.datatypes.count; i++) {
			auto& li = mi.datatypes.info[i];
			auto  ri = mi.datatypes.raw_info[i];
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

	{ // Parse LOD Hash Tables
		std::cout << "Hash Tables at:" << std::hex << mi.raw->s.hash_table_offset << std::endl;
		mi.hashes.raw   = reinterpret_cast<decltype(mi.hashes.raw)>(mesh_info_ptr + mi.raw->s.hash_table_offset);
		mi.hashes.count = mi.hashes.raw->s.count;

		mi.hashes.raw_datatype_hashes.resize(mi.hashes.count);
		mi.hashes.datatype_hashes.resize(mi.hashes.count);

		// Map raw information to memory.
		for (size_t i = 0; i < mi.hashes.count; i++) {
			mi.hashes.raw_datatype_hashes[i] = reinterpret_cast<hd2::hashentry_u const*>(mi.hashes.raw + 1) + i;
		}

		// Parse raw information from memory.
		for (size_t i = 0; i < mi.hashes.count; i++) {
			mi.hashes.datatype_hashes[i] = mi.hashes.raw_datatype_hashes[i]->s.value;
		}
	}

	{ // Parse Meshes
		std::cout << "Meshes at:" << std::hex << mi.raw->s.mesh_info_offset << std::endl;
		mi.meshes.raw   = reinterpret_cast<decltype(mi.meshes.raw)>(mesh_info_ptr + mi.raw->s.mesh_info_offset);
		mi.meshes.count = mi.meshes.raw->s.count;

		mi.meshes.info.resize(mi.meshes.count);
		mi.meshes.crc.resize(mi.meshes.count);

		mi.meshes.raw_offset.resize(mi.meshes.count);
		mi.meshes.raw_info.resize(mi.meshes.count);
		mi.meshes.raw_crc.resize(mi.meshes.count);

		// Map raw information to memory.
		for (size_t i = 0; i < mi.meshes.count; i++) {
			mi.meshes.raw_offset[i] = reinterpret_cast<hd2::meshoffset_u const*>(mi.meshes.raw + 1) + i;
			mi.meshes.raw_crc[i]    = reinterpret_cast<uint32_t const*>(reinterpret_cast<uint8_t const*>(mi.meshes.raw + 1) + sizeof(hd2::meshoffset_u) * mi.meshes.count) + i;
			mi.meshes.raw_info[i]   = reinterpret_cast<hd2::meshinfo_u const*>(reinterpret_cast<uint8_t const*>(mi.meshes.raw) + sizeof(hd2::meshtable_u) + mi.meshes.raw_offset[i]->s.offset);
		}

		// Parse raw information from memory.
		for (size_t i = 0; i < mi.meshes.count; i++) {
			auto& crc  = mi.meshes.crc[i];
			auto  rcrc = mi.meshes.raw_crc[i];
			crc        = *rcrc;

			auto& mesh  = mi.meshes.info[i];
			auto  rmesh = mi.meshes.raw_info[i];
			mesh.raw    = rmesh;

			mesh.indices_offset = rmesh->s.idx_offset;
			mesh.indices_count  = rmesh->s.idx_count;

			mesh.vertices_offset = rmesh->s.vtx_offset;
			mesh.vertices_count  = rmesh->s.vtx_count;

			// Find the matching LOD entry.
			mesh.lod_idx = -1;
			for (size_t j = 0; j < mi.hashes.count; j++) {
				if (mesh.raw->s.datatype_hash == mi.hashes.datatype_hashes[j]) {
					mesh.lod_idx = j;
					break;
				}
			}
		}
	}

	for (size_t i = 0; i < mi.meshes.count; i++) { // Write test mesh.
		// Grab necessary information
		auto const& mesh = mi.meshes.info[i];
		auto const& lod  = mi.datatypes.info[mesh.lod_idx];

		// Generate name.
		std::string filename;
		{
			const char*       format = "mesh%04" PRIu32 "_lod%02" PRIu32 ".obj";
			std::vector<char> buf;
			size_t            bufsz = snprintf(nullptr, 0, format, i, mesh.lod_idx);
			buf.resize(bufsz + 1);
			snprintf(buf.data(), buf.size(), format, i, mesh.lod_idx);
			filename = {buf.data(), buf.data() + buf.size() - 1};
		}

		// open file.
		std::ofstream output{filename};

		output << "o " << filename << std::endl;
		output << "g " << filename << std::endl;

		// Smooth shading
		output << "s 1" << std::endl;

		output.flush();

		uint8_t const* vtx_max_addr = mesh_ptr + lod.vertex_offset + lod.vertex_size;
		uint8_t const* vtx_addr     = mesh_ptr + lod.vertex_offset + mesh.vertices_offset * lod.vertex_stride;
		for (size_t j = 0; j < mesh.vertices_count; j++, vtx_addr += lod.vertex_stride) {
			if (vtx_addr >= vtx_max_addr) {
				std::cerr << "Parsing failed. Output model corrupted." << std::endl;
				return 1;
			}

			switch (lod.vertex_stride) {
			case sizeof(hd2::vertex20_t): {
				hd2::vertex20_t const* vtx = reinterpret_cast<decltype(vtx)>(vtx_addr);
				output << "# " << j << std::endl;
				output << "v " << (float)vtx->x * hd2::mesh_scale << " " << (float)vtx->y * hd2::mesh_scale << " " << (float)vtx->z * hd2::mesh_scale << std::endl;
				output << "vt " << (float)vtx->u << " " << (float)vtx->v << std::endl;

				break;
			}
			case sizeof(hd2::vertex28_t): {
				hd2::vertex28_t const* vtx = reinterpret_cast<decltype(vtx)>(vtx_addr);
				output << "# " << j << std::endl;
				output << "v " << (float)vtx->x * hd2::mesh_scale << " " << (float)vtx->y * hd2::mesh_scale << " " << (float)vtx->z * hd2::mesh_scale << std::endl;
				output << "vt " << (float)vtx->u << " " << (float)vtx->v << std::endl;

				break;
			}
			case sizeof(hd2::vertex36_t): {
				hd2::vertex36_t const* vtx = reinterpret_cast<decltype(vtx)>(vtx_addr);
				output << "# " << j << std::endl;
				output << "v " << (float)vtx->x * hd2::mesh_scale << " " << (float)vtx->y * hd2::mesh_scale << " " << (float)vtx->z * hd2::mesh_scale << std::endl;
				output << "vt " << (float)vtx->u << " " << (float)vtx->v << std::endl;

				break;
			}
			}
		}

		output.flush();

		uint8_t const* idx_max_addr = mesh_ptr + lod.index_offset + lod.index_size;
		uint8_t const* idx_addr     = mesh_ptr + lod.index_offset + mesh.indices_offset * sizeof(hd2::index_t);
		for (size_t j = 0; j < mesh.indices_count; j += 3, idx_addr += sizeof(hd2::index_t) * 3) {
			if (idx_addr >= idx_max_addr) {
				std::cerr << "Parsing failed. Output model corrupted." << std::endl;
				return 1;
			}

			hd2::index_t const* idx0 = reinterpret_cast<decltype(idx0)>(idx_addr);
			hd2::index_t const* idx1 = reinterpret_cast<decltype(idx1)>(idx_addr + sizeof(hd2::index_t));
			hd2::index_t const* idx2 = reinterpret_cast<decltype(idx2)>(idx_addr + sizeof(hd2::index_t) * 2);

			output << "# " << j << std::endl;
			output << "f ";
			output << (1 + idx0->index) << "/" << (1 + idx0->index) << "/" << (1 + idx0->index) << " ";
			output << (1 + idx1->index) << "/" << (1 + idx1->index) << "/" << (1 + idx1->index) << " ";
			output << (1 + idx2->index) << "/" << (1 + idx2->index) << "/" << (1 + idx2->index) << std::endl;
			//output << "f " << (1 + idx0->index) << " " << (1 + idx1->index) << " " << (1 + idx2->index) << std::endl;
		}

		output.flush();

		output.close();
	}

	std::cout << std::endl;

	return 0;
}
