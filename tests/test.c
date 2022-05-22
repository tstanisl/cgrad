#define CMAT_IMPLEMENTATION

#include <cgrad/matrix.h>
#include <stdio.h>

void print(cmat m) {
	CMAT_TO_VLA(m, arr);
	for (size_t r = 0; r < m.rows; ++r) {
		for (size_t c = 0; c < m.cols; ++c) {
			printf("\t%f", arr[r][c]);
		}
		puts("");
	}
}

int main(void) {
	cmat m = cmat_from_raw(2,3,(float[]){1,2,3,4,5,6}).get;
	cmat m2 = cmat_add(m, cmat_add(m, cmat_scalar(2))).get;

	print(m2);

	cmat_del(&m);
	cmat_del(&m2);

	return 0;
}
