// This test aims to check for incompatibilities with earlier versions of Visual Studio 2017.
// Primary goal here is to check std::align, as this currently breaks another project incorrectly.
// 

#include <xmmintrin.h>
#include <vector>
#include <memory>

struct vec4 {
	union {
		struct { float x, y, z, w; };
		float ptr[4];
		__m128 m;
	};
};

struct vec3 : vec4 {};

struct matrix4 {
	struct vec4 x, y, z, t;
};

struct quat : vec4 {};

struct Part {
	// Uncomment any of the following lines to get an error.
	//vec3 position, scale, rotation;
	//quat qrotation;
	//matrix4 local, global;
	bool localdirty;
	bool dirty;
	bool isquat;
};

int main(int argc, char const* argv[]) {
	std::shared_ptr<Part> pt = std::make_shared<Part>();
	return 0;
}