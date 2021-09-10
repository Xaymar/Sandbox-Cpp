#include <stdio.h>
#include <tmmintrin.h>

int main() {
	__m128i a, mask;

	a.m128i_i8[0] = 255;
	a.m128i_i8[1] = 138;
	a.m128i_i8[2] = 88;
	a.m128i_i8[3] = 42;
	a.m128i_i8[4] = 93;
	a.m128i_i8[5] = 232;
	a.m128i_i8[6] = 99;
	a.m128i_i8[7] = 78;

	a.m128i_i8[8] = -1;
	a.m128i_i8[9] = -2;
	a.m128i_i8[10] = -4;
	a.m128i_i8[11] = -8;
	a.m128i_i8[12] = -16;
	a.m128i_i8[13] = -32;
	a.m128i_i8[14] = -64;
	a.m128i_i8[15] = -128;

	mask.m128i_u8[0] = 0x0;
	mask.m128i_u8[1] = 0x01;
	mask.m128i_u8[2] = 0x02;
	mask.m128i_u8[3] = 0x03;
	mask.m128i_u8[4] = 0x04;
	mask.m128i_u8[5] = 0x05;
	mask.m128i_u8[6] = 0x06;
	mask.m128i_u8[7] = 0x07;

	mask.m128i_u8[8] = 0x00;
	mask.m128i_u8[9] = 0x11;
	mask.m128i_u8[10] = 0x22;
	mask.m128i_u8[11] = 0x33;
	mask.m128i_u8[12] = 0x44;
	mask.m128i_u8[13] = 0x55;
	mask.m128i_u8[14] = 0x66;
	mask.m128i_u8[15] = 0x77;

	__m128i res = _mm_shuffle_epi8(a, mask);

	printf_s("Result res:\t%2d\t%2d\t%2d\t%2d\n\t\t%2d\t%2d\t%2d\t%2d\n",
		res.m128i_i8[0], res.m128i_i8[1], res.m128i_i8[2],
		res.m128i_i8[3], res.m128i_i8[4], res.m128i_i8[5],
		res.m128i_i8[6], res.m128i_i8[7]);
	printf_s("\t\t%2d\t%2d\t%2d\t%2d\n\t\t%2d\t%2d\t%2d\t%2d\n",
		res.m128i_i8[8], res.m128i_i8[9], res.m128i_i8[10],
		res.m128i_i8[11], res.m128i_i8[12], res.m128i_i8[13],
		res.m128i_i8[14], res.m128i_i8[15]);

	return 0;
}