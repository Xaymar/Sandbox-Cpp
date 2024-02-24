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
#include <cstdio>
#include <cstring>
#include <filesystem>
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

struct FILE_deleter {
	void operator()(FILE* p) const
	{
		if (p)
			fclose(p);
	}
};
using file_t = shared_ptr_with_deleter<FILE, FILE_deleter>;

namespace hd2 {
	// Models are very tiny, so this can be used to scale them up.
	constexpr float mesh_scale = 1.;

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

	struct vertex16_t {
		float x, y, z;
		half  u, v;
	};

	struct vertex20_t {
		float    x, y, z;
		uint32_t __unk00;
		half     u, v;
	};

	struct vertex24_t {
		float    x, y, z;
		uint32_t __unk00;
		half     u0, v0;
		half     u1, v1;
	};

	struct vertex28_t {
		float    x, y, z;
		uint32_t __unk00;
		half     u0, v0;
		half     u1, v1; // Lightmap UVs?
		uint32_t __unk01; // Looks like UVs, but isn't.
	};

	struct vertex32_t {
		float    x, y, z;
		uint32_t __unk00;
		half     u0, v0;
		half     u1, v1;
		uint32_t __unk01[2];
	};

	struct vertex36_t {
		float    x, y, z;
		uint32_t __unk00;
		half     u0, v0;
		half     u1, v1;
		uint32_t __unk01[3];
	};

	struct vertex40_t {
		uint32_t __unk00; // Always 0xFFFFFFFF?
		float    x, y, z;
		uint32_t __unk01;
		half     u0, v0;
		half     u1, v1;
		uint32_t __unk02[3];
	};

	struct vertex60_t {
		float    x, y, z;
		uint32_t __unk00;
		half     u0, v0;
		half     u1, v1;
		half     u2, v2;
		half     nx, ny, nz;
		uint16_t __unk02;
		uint32_t __unk01[6];
	};
} // namespace hd2

int main(int argc, const char** argv)
{
	if (argc < 2) {
		return 1;
	}

	auto path          = std::filesystem::path{argv[1]};
	auto meshinfo_path = path.replace_extension(".meshinfo");
	auto mesh_path     = path.replace_extension(".mesh");
	auto output_path   = path.replace_extension();

#ifdef WIN32
	win32_handle_t mesh_info_file(CreateFileA(meshinfo_path.u8string().c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL));
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

	win32_handle_t mesh_file(CreateFileA(mesh_path.u8string().c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL));
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

	hd2::meshinfo meshinfo{reinterpret_cast<uint8_t const*>(mesh_info_ptr)};
	auto          materials = meshinfo.materials();
	auto          datatypes = meshinfo.datatypes();
	auto          meshes    = meshinfo.meshes();

	{ // Enumerate Materials
		printf("%" PRIu32 " Materials: \n", materials.count());
		for (size_t idx = 0; idx < materials.count(); idx++) {
			auto material = materials.at(idx);
			printf("- [%zu] %08" PRIx32 " = %" PRIx64 "\n", idx, material.first, material.second);
		}
	}

	{ // Enumerate Data Types
		printf("%" PRIu32 " Data Types: \n", datatypes.count());
		for (size_t idx = 0; idx < datatypes.count(); idx++) {
			auto datatype = datatypes.at(idx);
			printf("- [%zu] \n", idx);
			printf("  - %" PRIu32 " Vertices, ", datatype.vertices());
			printf("From: %08" PRIx32 ", ", datatype.vertices_offset());
			printf("To: %08" PRIx32 ", ", datatype.vertices_offset() + datatype.vertices_size());
			printf("Stride: %" PRId32 "\n", datatype.vertices_stride());
			printf("  - %" PRIu32 " Indices, ", datatype.indices());
			printf("From: %08" PRIx32 ", ", datatype.indices_offset());
			printf("To: %08" PRIx32 ", ", datatype.indices_offset() + datatype.indices_size());
			printf("Stride: %" PRId32 "\n", datatype.indices_stride());
		}
	}

	{ // Enumerate Meshes
		printf("%" PRIu32 " Meshes: \n", meshes.count());
		for (size_t idx = 0; idx < meshes.count(); idx++) {
			auto mesh     = meshes.at(idx);
			auto datatype = datatypes.at(mesh.datatype_index());
			printf("- [%zu] Data Type: %" PRIu32 "\n", idx, mesh.datatype_index());

			printf("  - %" PRIu32 " Vertices, Stride: %" PRIu32 ", ", mesh.modeldata().vertices_count(), datatype.vertices_stride());
			printf("Offset: %08" PRIx32 ", ", mesh.modeldata().vertices_offset());
			printf("From: %08" PRIx32 ", ", mesh.modeldata().vertices_offset() + datatype.vertices_offset());
			printf("To: %08" PRIx32 "\n", mesh.modeldata().vertices_offset() + datatype.vertices_offset() + mesh.modeldata().vertices_count() * datatype.vertices_stride());

			printf("  - %" PRIu32 " Indices, Stride: %" PRIu32 ", ", mesh.modeldata().indices_count(), datatype.indices_stride());
			printf("Offset: %08" PRIx32 ", ", mesh.modeldata().indices_offset());
			printf("From: %08" PRIx32 ", ", mesh.modeldata().indices_offset() + datatype.indices_offset());
			printf("To: %08" PRIx32 "\n", mesh.modeldata().indices_offset() + datatype.indices_offset() + mesh.modeldata().indices_count() * datatype.indices_stride());

			printf("  - %" PRIu32 " Materials: ", mesh.material_count());
			for (size_t jdx = 0; jdx < mesh.material_count(); jdx++) {
				printf("%" PRIx32 ", ", mesh.material_at(jdx));
			}
			printf("\n");
		}
	}
	printf("\n\n\n\n");

	{ // Export meshes.
		std::filesystem::create_directories(output_path);

		printf("Exporting Meshes...\n");
		for (size_t idx = 0; idx < meshes.count(); idx++) {
			printf("- [%zu] Exporting...\n", idx);
			auto mesh     = meshes.at(idx);
			auto datatype = datatypes.at(mesh.datatype_index());

			// Generate name.
			std::string filename;
			{
				const char*       format = "%s/%08" PRIu32 ".obj";
				std::vector<char> buf;
				size_t            bufsz = snprintf(nullptr, 0, format, output_path.u8string().c_str(), idx);
				buf.resize(bufsz + 1);
				snprintf(buf.data(), buf.size(), format, output_path.u8string().c_str(), idx);
				filename = {buf.data(), buf.data() + buf.size() - 1};
			}

			// Open output file.
			auto   filehandle = fopen(filename.c_str(), "w+b");
			file_t file{filehandle};

			// Write overall data.
			fprintf(file.get(), "o %s\n", filename.c_str());
			fprintf(file.get(), "g %s\n", filename.c_str());
			fprintf(file.get(), "s 0\n");
			fflush(file.get());

			{ // Write all vertices.
				std::ptrdiff_t ptr         = datatype.vertices_offset() + mesh.modeldata().vertices_offset() * datatype.vertices_stride();
				std::ptrdiff_t max_ptr     = ptr + mesh.modeldata().vertices_count() * datatype.vertices_stride();
				std::ptrdiff_t abs_max_ptr = datatype.vertices_offset() + datatype.vertices_size();

				bool is_error_ptr = false;
				if (ptr > abs_max_ptr) {
					printf("  ptr > abs_max_ptr\n");
					is_error_ptr = true;
				} else if (max_ptr > abs_max_ptr) {
					printf("  max_ptr > abs_max_ptr\n");
					is_error_ptr = true;
				}
				if (is_error_ptr) {
					printf("  ptr =         %08zx", ptr);
					printf("  max_ptr =     %08zx", max_ptr);
					printf("  abs_max_ptr = %08zx", abs_max_ptr);
					continue;
				}

				for (size_t vtx = 0; vtx < mesh.modeldata().vertices_count(); vtx++) {
					auto vtx_ptr = reinterpret_cast<uint8_t const*>(mesh_ptr + ptr + datatype.vertices_stride() * vtx);
					fprintf(file.get(), "# %zu\n", vtx);

					switch (datatype.vertices_stride()) {
					case sizeof(hd2::vertex16_t): {
						hd2::vertex20_t const* vtx = reinterpret_cast<decltype(vtx)>(vtx_ptr);
						fprintf(file.get(), "v  %#16.8g %#16.8g %#16.8g\n", (float)vtx->x * hd2::mesh_scale, (float)vtx->y * hd2::mesh_scale, (float)vtx->z * hd2::mesh_scale);
						fprintf(file.get(), "vt %#16.8g %#16.8g\n", (float)vtx->u, 1. - (float)vtx->v);
						break;
					}
					case sizeof(hd2::vertex20_t): {
						hd2::vertex20_t const* vtx = reinterpret_cast<decltype(vtx)>(vtx_ptr);
						fprintf(file.get(), "v  %#16.8g %#16.8g %#16.8g\n", (float)vtx->x * hd2::mesh_scale, (float)vtx->y * hd2::mesh_scale, (float)vtx->z * hd2::mesh_scale);
						fprintf(file.get(), "# ?? %08" PRIx32 "\n", vtx->__unk00);
						fprintf(file.get(), "vt %#16.8g %#16.8g\n", (float)vtx->u, 1. - (float)vtx->v);
						break;
					}
					case sizeof(hd2::vertex24_t): {
						hd2::vertex24_t const* vtx = reinterpret_cast<decltype(vtx)>(vtx_ptr);
						fprintf(file.get(), "# ?? %08" PRIx32 "\n", vtx->__unk00);
						fprintf(file.get(), "v  %#16.8g %#16.8g %#16.8g\n", (float)vtx->x * hd2::mesh_scale, (float)vtx->y * hd2::mesh_scale, (float)vtx->z * hd2::mesh_scale);
						fprintf(file.get(), "vt %#16.8g %#16.8g\n", (float)vtx->u0, 1. - (float)vtx->v0);
						fprintf(file.get(), "# vt %#16.8g %#16.8g\n", (float)vtx->u1, 1. - (float)vtx->v1);
						break;
					}
					case sizeof(hd2::vertex28_t): {
						hd2::vertex28_t const* vtx = reinterpret_cast<decltype(vtx)>(vtx_ptr);
						fprintf(file.get(), "v  %#16.8g %#16.8g %#16.8g\n", (float)vtx->x * hd2::mesh_scale, (float)vtx->y * hd2::mesh_scale, (float)vtx->z * hd2::mesh_scale);
						fprintf(file.get(), "# ?? %08" PRIx32 "\n", vtx->__unk00);
						fprintf(file.get(), "vt %#16.8g %#16.8g\n", (float)vtx->u0, 1. - (float)vtx->v0);
						fprintf(file.get(), "# vt %#16.8g %#16.8g\n", (float)vtx->u1, 1. - (float)vtx->v1);
						fprintf(file.get(), "# ?? %08" PRIx32 "\n", vtx->__unk01);
						break;
					}
					case sizeof(hd2::vertex32_t): {
						hd2::vertex32_t const* vtx = reinterpret_cast<decltype(vtx)>(vtx_ptr);
						fprintf(file.get(), "v  %#16.8g %#16.8g %#16.8g\n", (float)vtx->x * hd2::mesh_scale, (float)vtx->y * hd2::mesh_scale, (float)vtx->z * hd2::mesh_scale);
						fprintf(file.get(), "# ?? %08" PRIx32 "\n", vtx->__unk00);
						fprintf(file.get(), "vt %#16.8g %#16.8g\n", (float)vtx->u0, 1. - (float)vtx->v0);
						fprintf(file.get(), "# vt %#16.8g %#16.8g\n", (float)vtx->u1, 1. - (float)vtx->v1);
						fprintf(file.get(), "# ?? %08" PRIx32 "\n", vtx->__unk01[0]);
						fprintf(file.get(), "# ?? %08" PRIx32 "\n", vtx->__unk01[1]);
						break;
					}
					case sizeof(hd2::vertex36_t): {
						hd2::vertex36_t const* vtx = reinterpret_cast<decltype(vtx)>(vtx_ptr);
						fprintf(file.get(), "v  %#16.8g %#16.8g %#16.8g\n", (float)vtx->x * hd2::mesh_scale, (float)vtx->y * hd2::mesh_scale, (float)vtx->z * hd2::mesh_scale);
						fprintf(file.get(), "# ?? %08" PRIx32 "\n", vtx->__unk00);
						fprintf(file.get(), "vt %#16.8g %#16.8g\n", (float)vtx->u0, 1. - (float)vtx->v0);
						fprintf(file.get(), "# vt %#16.8g %#16.8g\n", (float)vtx->u1, 1. - (float)vtx->v1);
						fprintf(file.get(), "# ?? %08" PRIx32 "\n", vtx->__unk01[0]);
						fprintf(file.get(), "# ?? %08" PRIx32 "\n", vtx->__unk01[1]);
						fprintf(file.get(), "# ?? %08" PRIx32 "\n", vtx->__unk01[2]);
						break;
					}
					case sizeof(hd2::vertex40_t): {
						hd2::vertex40_t const* vtx = reinterpret_cast<decltype(vtx)>(vtx_ptr);
						fprintf(file.get(), "# ?? %08" PRIx32 "\n", vtx->__unk00);
						fprintf(file.get(), "v  %#16.8g %#16.8g %#16.8g\n", (float)vtx->x * hd2::mesh_scale, (float)vtx->y * hd2::mesh_scale, (float)vtx->z * hd2::mesh_scale);
						fprintf(file.get(), "# ?? %08" PRIx32 "\n", vtx->__unk01);
						fprintf(file.get(), "vt %#16.8g %#16.8g\n", (float)vtx->u0, 1. - (float)vtx->v0);
						fprintf(file.get(), "# vt %#16.8g %#16.8g\n", (float)vtx->u1, 1. - (float)vtx->v1);
						fprintf(file.get(), "# ?? %08" PRIx32 "\n", vtx->__unk02[0]);
						fprintf(file.get(), "# ?? %08" PRIx32 "\n", vtx->__unk02[1]);
						fprintf(file.get(), "# ?? %08" PRIx32 "\n", vtx->__unk02[2]);
						break;
					}
					case sizeof(hd2::vertex60_t): {
						hd2::vertex60_t const* vtx = reinterpret_cast<decltype(vtx)>(vtx_ptr);
						fprintf(file.get(), "v  %#16.8g %#16.8g %#16.8g\n", (float)vtx->x * hd2::mesh_scale, (float)vtx->y * hd2::mesh_scale, (float)vtx->z * hd2::mesh_scale);
						fprintf(file.get(), "# ?? %08" PRIx32 "\n", vtx->__unk00);
						fprintf(file.get(), "vt %#16.8g %#16.8g\n", (float)vtx->u0, 1. - (float)vtx->v0);
						fprintf(file.get(), "# vt %#16.8g %#16.8g\n", (float)vtx->u1, 1. - (float)vtx->v1);
						fprintf(file.get(), "# vt %#16.8g %#16.8g\n", (float)vtx->u2, 1. - (float)vtx->v2);
						fprintf(file.get(), "vn %#16.8g %#16.8g %#16.8g # %#16.8g\n", (float)vtx->nx, (float)vtx->ny, (float)vtx->nz, (float)vtx->nx + (float)vtx->ny + (float)vtx->nz);
						fprintf(file.get(), "# ?? %08" PRIx32 "\n", vtx->__unk01[0]);
						fprintf(file.get(), "# ?? %08" PRIx32 "\n", vtx->__unk01[1]);
						fprintf(file.get(), "# ?? %08" PRIx32 "\n", vtx->__unk01[2]);
						fprintf(file.get(), "# ?? %08" PRIx32 "\n", vtx->__unk01[3]);
						fprintf(file.get(), "# ?? %08" PRIx32 "\n", vtx->__unk01[4]);
						fprintf(file.get(), "# ?? %08" PRIx32 "\n", vtx->__unk01[5]);
						break;
					}
					}
				}
			}
			fflush(file.get());

			{ // Write all faces.
				std::ptrdiff_t ptr         = datatype.indices_offset() + mesh.modeldata().indices_offset() * datatype.indices_stride();
				std::ptrdiff_t max_ptr     = ptr + mesh.modeldata().indices_count() * datatype.indices_stride();
				std::ptrdiff_t abs_max_ptr = datatype.indices_offset() + datatype.indices_size();

				bool is_error_ptr = false;
				if (ptr > abs_max_ptr) {
					printf("  ptr > abs_max_ptr\n");
					is_error_ptr = true;
				} else if (max_ptr > abs_max_ptr) {
					printf("  max_ptr > abs_max_ptr\n");
					is_error_ptr = true;
				}
				if (is_error_ptr) {
					printf("  ptr =         %08zx", ptr);
					printf("  max_ptr =     %08zx", max_ptr);
					printf("  abs_max_ptr = %08zx", abs_max_ptr);
					continue;
				}

				for (size_t idx = 0; idx < mesh.modeldata().indices_count(); idx += 3) {
					auto idx_ptr = reinterpret_cast<uint8_t const*>(mesh_ptr + ptr + datatype.indices_stride() * idx);
					fprintf(file.get(), "# %zu\n", idx);

					switch (datatype.indices_stride()) {
					case 1: {
						auto v0 = *reinterpret_cast<uint8_t const*>(idx_ptr + datatype.indices_stride() * 0) + 1;
						auto v1 = *reinterpret_cast<uint8_t const*>(idx_ptr + datatype.indices_stride() * 1) + 1;
						auto v2 = *reinterpret_cast<uint8_t const*>(idx_ptr + datatype.indices_stride() * 2) + 1;
						fprintf(file.get(), "f %" PRIu8 "/%" PRIu8 "/%" PRIu8 " %" PRIu8 "/%" PRIu8 "/%" PRIu8 " %" PRIu8 "/%" PRIu8 "/%" PRIu8 "\n", v0, v0, v0, v1, v1, v1, v2, v2, v2);
						break;
					}
					case 2: {
						auto v0 = *reinterpret_cast<uint16_t const*>(idx_ptr + datatype.indices_stride() * 0) + 1;
						auto v1 = *reinterpret_cast<uint16_t const*>(idx_ptr + datatype.indices_stride() * 1) + 1;
						auto v2 = *reinterpret_cast<uint16_t const*>(idx_ptr + datatype.indices_stride() * 2) + 1;
						fprintf(file.get(), "f %" PRIu16 "/%" PRIu16 "/%" PRIu16 " %" PRIu16 "/%" PRIu16 "/%" PRIu16 " %" PRIu16 "/%" PRIu16 "/%" PRIu16 "\n", v0, v0, v0, v1, v1, v1, v2, v2, v2);
						break;
					}
					case 4: {
						auto v0 = *reinterpret_cast<uint32_t const*>(idx_ptr + datatype.indices_stride() * 0) + 1;
						auto v1 = *reinterpret_cast<uint32_t const*>(idx_ptr + datatype.indices_stride() * 1) + 1;
						auto v2 = *reinterpret_cast<uint32_t const*>(idx_ptr + datatype.indices_stride() * 2) + 1;
						fprintf(file.get(), "f %" PRIu32 "/%" PRIu32 "/%" PRIu32 " %" PRIu32 "/%" PRIu32 "/%" PRIu32 " %" PRIu32 "/%" PRIu32 "/%" PRIu32 "\n", v0, v0, v0, v1, v1, v1, v2, v2, v2);
						break;
					}
					case 8: {
						auto v0 = *reinterpret_cast<uint64_t const*>(idx_ptr + datatype.indices_stride() * 0) + 1;
						auto v1 = *reinterpret_cast<uint64_t const*>(idx_ptr + datatype.indices_stride() * 1) + 1;
						auto v2 = *reinterpret_cast<uint64_t const*>(idx_ptr + datatype.indices_stride() * 2) + 1;
						fprintf(file.get(), "f %" PRIu64 "/%" PRIu64 "/%" PRIu64 " %" PRIu64 "/%" PRIu64 "/%" PRIu64 " %" PRIu64 "/%" PRIu64 "/%" PRIu64 "\n", v0, v0, v0, v1, v1, v1, v2, v2, v2);
						break;
					}
					}
				}
			}
			fflush(file.get());

			file.reset();
		}
	}

	std::cout << std::endl;

	return 0;
}
