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
#include <iomanip>
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

	class meshinfo_datatype {
		uint8_t const* _ptr;
		struct data {
			uint32_t __unk00[2];
			uint32_t __varies00[2];
			uint32_t __unk01[3];
			uint32_t __varies01[2];
			uint32_t __unk02[3];
			uint32_t __varies02[2];
			uint32_t __unk03[2];
			uint32_t __unk04;
			uint32_t __varies03[3];
			uint32_t __unk05[2];
			uint32_t __varies04[2];
			uint32_t __unk06[3];
			uint32_t __varies05[2];
			uint32_t __unk07[3];
			uint32_t __unk08[56];
			uint32_t vertex_count;
			uint32_t vertex_stride;
			uint32_t __unk09[8];
			uint32_t index_count;
			uint32_t __unk10[5];
			uint32_t vertex_offset;
			uint32_t vertex_size;
			uint32_t index_offset;
			uint32_t index_size;
			uint32_t __unk11[4];
		} const* _data;

		public:
		meshinfo_datatype(uint8_t const* ptr) : _ptr(ptr), _data(reinterpret_cast<decltype(_data)>(_ptr)) {}

		uint32_t vertices()
		{
			return _data->vertex_count;
		}

		uint32_t vertices_offset()
		{
			return _data->vertex_offset;
		}

		uint32_t vertices_size()
		{
			return _data->vertex_size;
		}

		uint32_t vertices_stride()
		{
			//return (_data->vertex_size / _data->vertex_count);
			return _data->vertex_stride;
		}

		uint32_t indices_offset()
		{
			return _data->index_offset;
		}

		uint32_t indices()
		{
			return _data->index_count;
		}

		uint32_t indices_size()
		{
			return _data->index_size;
		}

		uint32_t indices_stride()
		{
			return (_data->index_size / _data->index_count);
		}
	};

	class meshinfo_datatype_list {
		uint8_t const* _ptr;
		struct data {
			uint32_t count;
			//uint32_t offsets[count];
			//uint32_t crcs[count];
			//uint32_t __unk00;
		} const* _data;

		public:
		meshinfo_datatype_list(uint8_t const* ptr) : _ptr(ptr), _data(reinterpret_cast<decltype(_data)>(_ptr)) {}

		uint32_t count() const
		{
			return _data->count;
		}

		uint32_t const* offsets() const
		{
			return reinterpret_cast<uint32_t const*>(_ptr + sizeof(data));
		}

		uint32_t const* not_crcs() const
		{
			return reinterpret_cast<uint32_t const*>(_ptr + sizeof(data) + sizeof(uint32_t) * count());
		}

		meshinfo_datatype at(size_t idx) const
		{
			auto edx = count();
			if (idx >= edx) {
				std::out_of_range("idx >= edx");
			}

			return {reinterpret_cast<uint8_t const*>(_ptr + offsets()[idx])};
		}
	};

	class meshinfo_mesh_modeldata {
		uint8_t const* _ptr;
		struct {
			uint32_t vertices_offset;
			uint32_t vertices_count;
			uint32_t indices_offset;
			uint32_t indices_count;
		} const* _data;

		public:
		meshinfo_mesh_modeldata(uint8_t const* ptr) : _ptr(ptr), _data(reinterpret_cast<decltype(_data)>(_ptr)) {}

		uint32_t indices_offset() const
		{
			return _data->indices_offset;
		}

		uint32_t indices_count() const
		{
			return _data->indices_count;
		}

		uint32_t vertices_offset() const
		{
			return _data->vertices_offset;
		}

		uint32_t vertices_count() const
		{
			return _data->vertices_count;
		}
	};

	class meshinfo_mesh {
		uint8_t const* _ptr;
		struct data {
			uint32_t __unk00;
			uint32_t __varies00[7];
			uint32_t __unk01;
			uint32_t __identicalToNotCRC;
			uint32_t __varies01[2];
			uint32_t __unk02;
			uint32_t __varies02;
			uint32_t datatype_index;
			uint32_t __unk03[10];
			uint32_t material_count2;
			uint32_t __unk04[3];
			uint32_t material_count;
			uint32_t modeldata_offset;
			//uint32_t materials[material_count];
		} const* _data;

		public:
		meshinfo_mesh(uint8_t const* ptr) : _ptr(ptr), _data(reinterpret_cast<decltype(_data)>(_ptr)) {}

		uint32_t datatype_index() const
		{
			return _data->datatype_index;
		}

		uint32_t material_count() const
		{
			return _data->material_count;
		}

		uint32_t const* materials() const
		{
			return reinterpret_cast<uint32_t const*>(_ptr + offsetof(data, modeldata_offset) + sizeof(uint32_t));
		}

		uint32_t material_at(size_t idx) const
		{
			auto edx = material_count();
			if (idx >= edx) {
				std::out_of_range("idx >= edx");
			}

			return materials()[idx];
		}

		meshinfo_mesh_modeldata modeldata() const
		{
			return {_ptr + _data->modeldata_offset};
		}
	};

	class meshinfo_mesh_list {
		uint8_t const* _ptr;
		struct data {
			uint32_t count;
			//uint32_t offsets[count];
			//uint32_t crcs[count];
		} const* _data;

		public:
		meshinfo_mesh_list(uint8_t const* ptr) : _ptr(ptr), _data(reinterpret_cast<decltype(_data)>(_ptr)) {}

		uint32_t count() const
		{
			return _data->count;
		}

		uint32_t const* offsets() const
		{
			return reinterpret_cast<uint32_t const*>(_ptr + sizeof(data));
		}

		uint32_t const* not_crcs() const
		{
			return reinterpret_cast<uint32_t const*>(_ptr + sizeof(data) + sizeof(uint32_t) * count());
		}

		meshinfo_mesh at(size_t idx) const
		{
			auto edx = count();
			if (idx >= edx) {
				std::out_of_range("idx >= edx");
			}

			return {reinterpret_cast<uint8_t const*>(_ptr + sizeof(data) + offsets()[idx])};
		}
	};

	class meshinfo_material_list {
		uint8_t const* _ptr;
		struct data {
			uint32_t count;
			//uint64_t keys[count];
			//uint64_t values[count];
		} const* _data;

		public:
		meshinfo_material_list(uint8_t const* ptr) : _ptr(ptr), _data(reinterpret_cast<decltype(_data)>(_ptr)) {}

		uint32_t count()
		{
			return _data->count;
		}

		uint32_t const* keys()
		{
			return reinterpret_cast<uint32_t const*>(_ptr + sizeof(data));
		}

		uint64_t const* values()
		{
			return reinterpret_cast<uint64_t const*>(_ptr + sizeof(data) + sizeof(uint32_t) * count());
		}

		std::pair<uint32_t, uint64_t> at(size_t idx)
		{
			auto edx = count();
			if (idx >= edx) {
				std::out_of_range("idx >= edx");
			}

			return {keys()[idx], values()[idx]};
		}
	};

	class meshinfo {
		uint8_t const* _ptr;
		struct data {
			uint32_t __unk00[12];
			uint32_t __unk01;
			uint32_t __unk02;
			uint32_t __unk03;
			uint32_t __unk04;
			uint32_t __unk05[3];
			uint32_t __unk06;
			uint32_t __unk07;
			uint32_t __unk08;
			uint32_t __unk09;
			uint32_t datatype_offset;
			uint32_t __unk10;
			uint32_t mesh_offset;
			uint32_t __unk11[2];
			uint32_t material_offset;
		} const* _data;

		public:
		meshinfo(uint8_t const* mi) : _ptr(mi), _data(reinterpret_cast<decltype(_data)>(_ptr)) {}

		meshinfo_datatype_list datatypes()
		{
			return {_ptr + _data->datatype_offset};
		}

		meshinfo_mesh_list meshes()
		{
			return {_ptr + _data->mesh_offset};
		}

		meshinfo_material_list materials()
		{
			return {_ptr + _data->material_offset};
		}
	};

	struct index8_t {
		uint8_t index;
	};

	struct index16_t {
		uint16_t index;
	};

	struct index32_t {
		uint32_t index;
	};

	struct vertex20_t {
		float    x, y, z;
		uint32_t __unk00;
		half     u, v;
	};

	struct vertex24_t {
		uint32_t __unk00; // Always 0xFFFFFFFF?
		float    x, y, z;
		uint32_t __unk01;
		half     u, v;
	};

	struct vertex28_t {
		float    x, y, z;
		uint32_t __unk00;
		half     u, v;
		uint32_t __unk01[2];
	};

	struct vertex32_t {
		uint32_t __unk00; // Always 0xFFFFFFFF?
		float    x, y, z;
		uint32_t __unk01;
		half     u, v;
		uint32_t __unk02[2];
	};

	struct vertex36_t {
		float    x, y, z;
		uint32_t __unk00;
		half     u, v;
		uint32_t __unk01[4];
	};

	struct vertex40_t {
		uint32_t __unk00; // Always 0xFFFFFFFF?
		float    x, y, z;
		uint32_t __unk01;
		half     u, v;
		uint32_t __unk02[4];
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

	hd2::meshinfo mi{reinterpret_cast<uint8_t const*>(mesh_info_ptr)};
	{ // Enumerate Materials
		auto ms = mi.materials();
		printf("%" PRIu32 " Materials: \n", ms.count());
		for (size_t idx = 0; idx < ms.count(); idx++) {
			auto m = ms.at(idx);
			printf("  - [%zu] %04" PRIx32 " = %" PRIx64 "\n", idx, m.first, m.second);
		}
	}
	{ // Enumerate Data Types
		auto ds = mi.datatypes();
		printf("%" PRIu32 " Data Types: \n", ds.count());
		for (size_t idx = 0; idx < ds.count(); idx++) {
			auto d = ds.at(idx);
			printf("  - [%zu] \n", idx);
			printf("    - %" PRIu32 " Vertices\n", d.vertices());
			printf("      - From: %08" PRIx32 "\n", d.vertices_offset());
			printf("      - To:   %08" PRIx32 "\n", d.vertices_offset() + d.vertices_size());
			printf("      - Stride: %" PRId32 "\n", d.vertices_stride());
			printf("    - %" PRIu32 " Indices\n", d.indices());
			printf("      - From: %08" PRIx32 "\n", d.indices_offset());
			printf("      - To:   %08" PRIx32 "\n", d.indices_offset() + d.indices_size());
			printf("      - Stride: %" PRId32 "\n", d.indices_stride());
		}
	}
	{ // Enumerate Meshes
		auto ms = mi.meshes();
		printf("%" PRIu32 " Meshes: \n", ms.count());
		for (size_t idx = 0; idx < ms.count(); idx++) {
			auto m = ms.at(idx);
			auto d = mi.datatypes().at(m.datatype_index());
			printf("  - [%zu] Data Type: %" PRIu32 "\n", idx, m.datatype_index());
			printf("    - %" PRIu32 " Vertices\n", m.modeldata().vertices_count());
			printf("      - Offset: %08" PRIx32 "\n", m.modeldata().vertices_offset());
			printf("      - From:   %08" PRIx32 "\n", m.modeldata().vertices_offset() + d.vertices_offset());
			printf("      - To:   %08" PRIx32 "\n", m.modeldata().vertices_offset() + d.vertices_offset() + m.modeldata().vertices_count() * d.vertices_stride());
			printf("    - %" PRIu32 " Indices\n", m.modeldata().indices_count());
			printf("      - Offset: %08" PRIx32 "\n", m.modeldata().indices_offset());
			printf("      - From: %08" PRIx32 "\n", m.modeldata().indices_offset() + d.indices_offset());
			printf("      - To:   %08" PRIx32 "\n", m.modeldata().indices_offset() + d.indices_offset() + m.modeldata().indices_count() * d.indices_stride());
			printf("    - %" PRIu32 " Materials\n", m.material_count());
			for (size_t jdx = 0; jdx < m.material_count(); jdx++) {
				printf("      - [%zu] %" PRIx32 "\n", jdx, m.material_at(jdx));
			}
		}
	}

	/*

	{ // Parse Data Types
		auto fmtflags = std::cout.flags();
		std::cout << "Data Types at: 0x" << std::setfill('0') << std::setw(sizeof(size_t)) << std::right << std::hex << (reinterpret_cast<uint8_t const*>(mi.datatypes.raw) - mesh_info_ptr) << std::endl;
		std::cout.flags(fmtflags);
		mi.datatypes.raw = reinterpret_cast<decltype(mi.datatypes.raw)>(mesh_info_ptr + mi.raw->s.datatype_offset);

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

	{ // Parse Hash Tables
		auto fmtflags = std::cout.flags();
		std::cout << "Hash Tables at: 0x" << std::setfill('0') << std::setw(sizeof(size_t)) << std::right << std::hex << (reinterpret_cast<uint8_t const*>(mi.materials.raw) - mesh_info_ptr) << std::endl;
		std::cout.flags(fmtflags);
		mi.materials.raw   = reinterpret_cast<decltype(mi.materials.raw)>(mesh_info_ptr + mi.raw->s.hashtable_offset);
		mi.materials.count = mi.materials.raw->s.count;

		mi.materials.raw_datatype_hashes.resize(mi.materials.count);
		mi.materials.datatype_hashes.resize(mi.materials.count);
		mi.materials.material_hashes.resize(mi.materials.count);

		// Map raw information to memory.
		for (size_t i = 0; i < mi.materials.count; i++) {
			mi.materials.raw_datatype_hashes[i] = reinterpret_cast<hd2::hashentry_u const*>(mi.materials.raw + 1) + i;
		}

		// Parse raw information from memory.
		for (size_t i = 0; i < mi.materials.count; i++) {
			mi.materials.datatype_hashes[i] = mi.materials.raw_datatype_hashes[i]->s.value;
			mi.materials.material_hashes[i] =
		}
	}

	{ // Parse Meshes
		auto fmtflags = std::cout.flags();
		std::cout << "Meshes at: 0x" << std::setfill('0') << std::setw(sizeof(size_t)) << std::right << std::hex << (reinterpret_cast<uint8_t const*>(mi.meshes.raw) - mesh_info_ptr) << std::endl;
		std::cout.flags(fmtflags);
		mi.meshes.raw   = reinterpret_cast<decltype(mi.meshes.raw)>(mesh_info_ptr + mi.raw->s.meshinfo_info);
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

			// Find the matching data type entry.
			mesh.datatype_idx = -1;
			for (size_t j = 0; j < mi.materials.count; j++) {
				if (mesh.raw->s.datatype_hash == mi.materials.datatype_hashes[j]) {
					mesh.datatype_idx = j;
					break;
				}
			}
		}
	}

	for (size_t i = 0; i < mi.meshes.count; i++) { // Write test mesh.
		// Grab necessary information
		auto const& mesh     = mi.meshes.info[i];
		auto const& datatype = mi.datatypes.info[mesh.datatype_idx];

		// Generate name.
		std::string filename;
		{
			const char*       format = "mesh_%04" PRIu32 ".obj";
			std::vector<char> buf;
			size_t            bufsz = snprintf(nullptr, 0, format, i);
			buf.resize(bufsz + 1);
			snprintf(buf.data(), buf.size(), format, i);
			filename = {buf.data(), buf.data() + buf.size() - 1};
		}

		std::cout << filename << std::endl;
		std::cout << "  Type: " << datatype.vertex_stride << std::endl;
		std::cout << "  Indices: " << mesh.indices_count << "/" << datatype.index_count << std::endl;
		std::cout << "  Vertices: " << mesh.vertices_count << "/" << datatype.vertex_count << std::endl;

		// open file.
		std::ofstream output{filename};

		output << "o " << filename << std::endl;
		output << "g " << filename << std::endl;

		// Smooth shading
		output << "s 1" << std::endl;

		output.flush();

		uint8_t const* vtx_max_addr      = mesh_ptr + datatype.vertex_offset + datatype.vertex_size;
		uint8_t const* vtx_addr          = mesh_ptr + datatype.vertex_offset + mesh.vertices_offset * datatype.vertex_stride;
		uint8_t const* vtx_mesh_max_addr = mesh_ptr + datatype.vertex_offset + mesh.vertices_offset * datatype.vertex_stride + mesh.vertices_count * datatype.vertex_stride;

		if (vtx_mesh_max_addr > vtx_max_addr) {
			std::cerr << "Vertex list malformed, skipping." << std::endl;
			auto fmtflags = std::cerr.flags();
			std::cerr << "  0x";
			std::cerr << std::setfill('0') << std::setw(sizeof(size_t)) << std::right << std::hex << reinterpret_cast<std::intptr_t>(mesh_ptr);
			std::cerr.flags(fmtflags);
			std::cerr << std::endl;
			std::cerr << "  0x";
			std::cerr << std::setfill('0') << std::setw(sizeof(size_t)) << std::right << std::hex << reinterpret_cast<std::intptr_t>(vtx_addr);
			std::cerr.flags(fmtflags);
			std::cerr << std::endl;
			std::cerr << "  0x";
			std::cerr << std::setfill('0') << std::setw(sizeof(size_t)) << std::right << std::hex << reinterpret_cast<std::intptr_t>(vtx_max_addr);
			std::cerr.flags(fmtflags);
			std::cerr << std::endl;
			std::cerr << "  0x";
			std::cerr << std::setfill('0') << std::setw(sizeof(size_t)) << std::right << std::hex << reinterpret_cast<std::intptr_t>(vtx_mesh_max_addr);
			std::cerr.flags(fmtflags);
			std::cerr << std::endl;
			continue;
		}

		for (size_t j = 0; j < mesh.vertices_count; j++, vtx_addr += datatype.vertex_stride) {
			if (vtx_addr >= vtx_max_addr) {
				std::cerr << "Error: Vertex address out of bounds for data type group." << std::endl;
				auto fmtflags = std::cerr.flags();
				std::cerr << "  0x";
				std::cerr << std::setfill('0') << std::setw(sizeof(size_t)) << std::right << std::hex << reinterpret_cast<std::intptr_t>(mesh_ptr);
				std::cerr.flags(fmtflags);
				std::cerr << std::endl;
				std::cerr << "  0x";
				std::cerr << std::setfill('0') << std::setw(sizeof(size_t)) << std::right << std::hex << reinterpret_cast<std::intptr_t>(vtx_addr);
				std::cerr.flags(fmtflags);
				std::cerr << " < 0x";
				std::cerr << std::setfill('0') << std::setw(sizeof(size_t)) << std::right << std::hex << reinterpret_cast<std::intptr_t>(vtx_max_addr);
				std::cerr.flags(fmtflags);
				std::cerr << std::endl;
				std::cerr << "  " << j << " <= " << mesh.vertices_count << std::endl;
				std::cerr << "  " << (j + mesh.vertices_offset) << " <= " << datatype.vertex_count << std::endl;
				return 1;
			}

			switch (datatype.vertex_stride) {
			case sizeof(hd2::vertex20_t): {
				hd2::vertex20_t const* vtx = reinterpret_cast<decltype(vtx)>(vtx_addr);
				output << "# " << j << std::endl;
				output << "v " << (float)vtx->x * hd2::mesh_scale << " " << (float)vtx->y * hd2::mesh_scale << " " << (float)vtx->z * hd2::mesh_scale << std::endl;
				output << "vt " << (float)vtx->u << " " << (float)vtx->v << std::endl;

				break;
			}
			case sizeof(hd2::vertex24_t): {
				hd2::vertex24_t const* vtx = reinterpret_cast<decltype(vtx)>(vtx_addr);
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
			case sizeof(hd2::vertex32_t): {
				hd2::vertex32_t const* vtx = reinterpret_cast<decltype(vtx)>(vtx_addr);
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
			case sizeof(hd2::vertex40_t): {
				hd2::vertex40_t const* vtx = reinterpret_cast<decltype(vtx)>(vtx_addr);
				output << "# " << j << std::endl;
				output << "v " << (float)vtx->x * hd2::mesh_scale << " " << (float)vtx->y * hd2::mesh_scale << " " << (float)vtx->z * hd2::mesh_scale << std::endl;
				output << "vt " << (float)vtx->u << " " << (float)vtx->v << std::endl;

				break;
			}
			}
		}

		output.flush();

		uint8_t const* idx_max_addr      = mesh_ptr + datatype.index_offset + datatype.index_size;
		uint8_t const* idx_addr          = mesh_ptr + datatype.index_offset + mesh.indices_offset * sizeof(hd2::index_t);
		uint8_t const* idx_mesh_max_addr = mesh_ptr + datatype.index_offset + mesh.indices_offset * sizeof(hd2::index_t) + mesh.vertices_count * sizeof(hd2::index_t);

		if (idx_mesh_max_addr > idx_max_addr) {
			std::cerr << "Indices List malformed, skipping." << std::endl;
			auto fmtflags = std::cerr.flags();
			std::cerr << "  0x";
			std::cerr << std::setfill('0') << std::setw(sizeof(size_t)) << std::right << std::hex << reinterpret_cast<std::intptr_t>(mesh_ptr);
			std::cerr.flags(fmtflags);
			std::cerr << std::endl;
			std::cerr << "  0x";
			std::cerr << std::setfill('0') << std::setw(sizeof(size_t)) << std::right << std::hex << reinterpret_cast<std::intptr_t>(idx_addr);
			std::cerr.flags(fmtflags);
			std::cerr << std::endl;
			std::cerr << "  0x";
			std::cerr << std::setfill('0') << std::setw(sizeof(size_t)) << std::right << std::hex << reinterpret_cast<std::intptr_t>(idx_max_addr);
			std::cerr.flags(fmtflags);
			std::cerr << std::endl;
			std::cerr << "  0x";
			std::cerr << std::setfill('0') << std::setw(sizeof(size_t)) << std::right << std::hex << reinterpret_cast<std::intptr_t>(idx_mesh_max_addr);
			std::cerr.flags(fmtflags);
			std::cerr << std::endl;
			continue;
		}

		for (size_t j = 0; j < mesh.indices_count; j += 3, idx_addr += sizeof(hd2::index_t) * 3) {
			if (idx_addr >= idx_max_addr) {
				std::cerr << "Error: Index address out of bounds for data type group." << std::endl;
				auto fmtflags = std::cerr.flags();
				std::cerr << "  0x";
				std::cerr << std::setfill('0') << std::setw(sizeof(size_t)) << std::right << std::hex << reinterpret_cast<std::intptr_t>(mesh_ptr);
				std::cerr.flags(fmtflags);
				std::cerr << std::endl;
				std::cerr << "  0x";
				std::cerr << std::setfill('0') << std::setw(sizeof(size_t)) << std::right << std::hex << reinterpret_cast<std::intptr_t>(idx_addr);
				std::cerr.flags(fmtflags);
				std::cerr << std::endl;
				std::cerr << "  0x";
				std::cerr << std::setfill('0') << std::setw(sizeof(size_t)) << std::right << std::hex << reinterpret_cast<std::intptr_t>(idx_max_addr);
				std::cerr.flags(fmtflags);
				std::cerr << std::endl;
				std::cerr << "  0x";
				std::cerr << std::setfill('0') << std::setw(sizeof(size_t)) << std::right << std::hex << reinterpret_cast<std::intptr_t>(idx_mesh_max_addr);
				std::cerr.flags(fmtflags);
				std::cerr << std::endl;
				std::cerr << "  " << j << " <= " << mesh.indices_count << std::endl;
				std::cerr << "  " << (j + mesh.indices_offset) << " <= " << datatype.index_count << std::endl;
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
	}*/

	std::cout << std::endl;

	return 0;
}
