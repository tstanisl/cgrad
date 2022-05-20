#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct cmat {
	size_t rows;
	size_t cols;
	union {
		void *data;
		float scalar[1][1];
	};
} cmat;

inline bool cmat_is_valid(cmat m) { return m.rows + m.cols > 0; }
inline bool cmat_is_scalar(cmat m) { return m.rows == 1 && m.cols == 1; }
inline cmat cmat_scalar(float val) { return (cmat) { .rows = 1, .cols = 1, .scalar = val }; }

cmat cmat_create(size_t rows, size_t cols);
cmat cmat_create_fill(size_t rows, size_t cols, float val);

#define CMAT_TO_VLA(m, vla) float (*vla)[m.cols] = cmat_is_scalar(m) ? m.scalar : m.data

cmat cmat_add(cmat a, cmat b);

#ifdef CMAT_IMPLEMENTATION

bool cmat_is_valid(cmat);
bool cmat_is_scalar(cmat m);

cmat cmat_add(cmat a, cmat b) {
	size_t rows = a.rows > b.rows ? a.rows : b.rows;
	size_t cols = a.cols > b.cols ? a.cols : b.cols;
}

#endif
