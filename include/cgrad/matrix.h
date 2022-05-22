#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct cmat {
	int status;
	size_t rows;
	size_t cols;
	union {
		void *data;
		float scalar[1][1];
	};
} cmat;


enum cmat_status {
	CMAT_VALID = 0,
	CMAT_DELETED,
	CMAT_ERR_OUT_OF_MEMORY,
	CMAT_ERR_INVALID_OPERAND,
	CMAT_ERR_CANT_BROADCAST,
};


typedef union {
	int status;
	struct cmat get;
} cmat_tmp;

inline bool cmat_is_valid(cmat m) { return m.rows > 0 && m.cols > 0; }
inline bool cmat_is_scalar(cmat m) { return m.rows == 1 && m.cols == 1; }

#define CMAT__DATA(m) (cmat_is_scalar(m) ? (m).scalar : (m).data)

inline cmat_tmp cmat_move(cmat* m) {
	cmat_tmp res = { .get = *m };
	*m = (cmat) { .status = CMAT_DELETED };
	return res;
}

inline cmat cmat_scalar(float val) {
	return (cmat) { .rows = 1, .cols = 1, .scalar = val };
}

__attribute__((always_inline))
inline cmat_tmp cmat_create(size_t rows, size_t cols) {
	if (rows == 1 && cols == 1)
		return (cmat_tmp) { .get = cmat_scalar(0.0f) };
	cmat res = { .rows = rows, .cols = cols };
	res.data = malloc(sizeof(float[rows][cols]));
	if (!res.data)
		return (cmat_tmp) { CMAT_ERR_OUT_OF_MEMORY };
	return cmat_move(&res);
}

cmat cmat_wrap_array(size_t rows, size_t cols, float data[static rows][cols]);

inline cmat_tmp cmat_from_array(size_t rows, size_t cols, float data[static rows][cols]) {
	cmat res = cmat_create(rows, cols).get;
	if (res.status == CMAT_VALID)
		memcpy(CMAT__DATA(res), data, sizeof(float[rows][cols]));
	return cmat_move(&res);
}

inline void cmat_del(cmat *m) {
	if (!cmat_is_scalar(*m))
		free(m->data);
	*m = (cmat) { .status = CMAT_DELETED };
}

#define CMAT_TO_VLA(m, vla) float (*vla)[m.cols] = CMAT__DATA(m)

#define cmat_add cmat_add_mm

cmat_tmp cmat_add_mm(cmat a, cmat b);
cmat_tmp cmat_add_mt(cmat a, cmat_tmp b);
cmat_tmp cmat_add_tm(cmat_tmp a, cmat b);
cmat_tmp cmat_add_tt(cmat_tmp a, cmat_tmp b);

#ifdef CMAT_IMPLEMENTATION

// instantiations of inline functions

bool cmat_is_valid(cmat m);
bool cmat_is_scalar(cmat m);
cmat cmat_scalar(float val);
cmat_tmp cmat_create(size_t rows, size_t cols);
cmat_tmp cmat_from_array(size_t rows, size_t cols, float data[static rows][cols]);
void cmat_del(cmat* m);
cmat_tmp cmat_move(cmat* m);

cmat_tmp cmat_add_mm(cmat a, cmat b) {
	size_t rows = a.rows > b.rows ? a.rows : b.rows;
	size_t cols = a.cols > b.cols ? a.cols : b.cols;

	if (a.status != CMAT_VALID || b.status != CMAT_VALID)
		return (cmat_tmp) { CMAT_ERR_INVALID_OPERAND };

	if ((a.rows > 1 && b.rows > 1 && a.rows != b.rows) ||
	    (a.cols > 1 && b.cols > 1 && a.cols != b.cols))
		return (cmat_tmp) { CMAT_ERR_CANT_BROADCAST };

	cmat c = cmat_create(rows, cols).get;
	if (c.status != CMAT_VALID)
		return cmat_move(&c);

	size_t dra = (a.rows == rows);
	size_t drb = (b.rows == rows);
	size_t dca = (a.cols == cols);
	size_t dcb = (b.cols == cols);

	CMAT_TO_VLA(a, adata);
	CMAT_TO_VLA(b, bdata);
	CMAT_TO_VLA(c, cdata);

	for (size_t r = 0, ra = 0, rb = 0; r < rows; r++, ra += dra, rb += drb) {
	for (size_t c = 0, ca = 0, cb = 0; c < cols; c++, ca += dca, cb += dcb) {
		cdata[r][c] = adata[ra][ca] + bdata[rb][cb];
	}}

	return cmat_move(&c);
}

#endif
