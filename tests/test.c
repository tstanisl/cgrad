#define CMAT_IMPLEMENTATION

#include <cgrad/matrix.h>
#include <stdio.h>

int main(void) {
	cmat m = cmat_from_array(2,3,(float[2][3]){{1,2,3},{4,5,6}}).get;
	cmat m2 = cmat_add(m, cmat_add(m, cmat_scalar(2))).get;

	CMAT_TO_VLA(m2, arr);
	for (size_t r = 0; r < m2.rows; ++r) {
		for (size_t c = 0; c < m2.cols; ++c) {
			printf("\t%f", arr[r][c]);
		}
		puts("");
	}

	cmat_del(&m);
	cmat_del(&m2);

	return 0;
}
