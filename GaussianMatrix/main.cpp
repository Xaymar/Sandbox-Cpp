#include <cinttypes>
#include <string>
#include <iostream>

// Constants
#define PI		3.1415926535897932384626433832795
#define PI2		6.283185307179586476925286766559
#define PI2_SQROOT	2.506628274631000502415765284811		

inline double_t Gaussian1D(double_t x, double_t o) {
	double_t c = (x / o);
	double_t b = exp(-0.5 * c * c);
	double_t a = (1.0 / (o * PI2_SQROOT));
	return a * b;
}

int main(int argc, char* argv[]) {
	double matrix[2];
	matrix[0] = Gaussian1D(1.0, 1.0);
	matrix[1] = Gaussian1D(0.0, 1.0);

	double fullmatrix[3][3];
	fullmatrix[0][0] = fullmatrix[0][2] = fullmatrix[2][0] = fullmatrix[2][2] = matrix[0] * matrix[0];
	fullmatrix[1][0] = fullmatrix[1][2] = fullmatrix[0][1] = fullmatrix[2][1] = matrix[1] * matrix[0];
	fullmatrix[1][1] = matrix[1];
	double adjmatrix[3][3];

	double avg = 0.0;
	for (size_t x = 0; x <= 2; x++) {
		for (size_t y = 0; y <= 2; y++) {
			avg += fullmatrix[x][y];
		}
	}
	double rv = 1.0 / avg;
	for (size_t x = 0; x <= 2; x++) {
		for (size_t y = 0; y <= 2; y++) {
			adjmatrix[x][y] = fullmatrix[x][y] * rv;
		}
	}

	std::cout << "Full Matrix" << '\n';
	for (size_t y = 0; y <= 2; y++) {
		std::cout << fullmatrix[0][y] << "; " << fullmatrix[1][y] << "; " << fullmatrix[2][y] << '\n';
	}

	std::cout << "Total: " << avg << '\n';
	std::cout << "Inverse: " << rv << '\n';

	std::cout << "Adjusted Matrix" << '\n';
	for (size_t y = 0; y <= 2; y++) {
		std::cout << adjmatrix[0][y] << "; " << adjmatrix[1][y] << "; " << adjmatrix[2][y] << '\n';
	}

	std::cout << std::endl;

	std::cin.get();


	return 0;
}