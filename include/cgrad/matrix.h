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

static inline bool cmat_is_scalar(cmat m) {
	return m.rows == 1 && m.cols == 1;
}

#define CMAT__DATA(m) (cmat_is_scalar(m) ? (m).scalar : (m).data)

static inline
cmat_tmp cmat_move(cmat* m) {
	cmat_tmp res = { .get = *m };
	*m = (cmat) { .status = CMAT_DELETED };
	return res;
}

static inline
cmat cmat_scalar(float val) {
	return (cmat) { .rows = 1, .cols = 1, .scalar = val };
}

static inline
cmat_tmp cmat_create(size_t rows, size_t cols) {
	if (rows == 1 && cols == 1)
		return (cmat_tmp) { .get = cmat_scalar(0.0f) };
	cmat res = { .rows = rows, .cols = cols };
	res.data = malloc(sizeof(float[rows][cols]));
	if (!res.data)
		return (cmat_tmp) { CMAT_ERR_OUT_OF_MEMORY };
	return cmat_move(&res);
}

// TODO
cmat cmat_wrap_array(size_t rows, size_t cols, float data[static rows][cols]);

static inline
cmat_tmp cmat_from_raw(size_t rows, size_t cols, const void *data) {
	cmat res = cmat_create(rows, cols).get;
	if (res.status == CMAT_VALID)
		memcpy(CMAT__DATA(res), data, sizeof(float[rows][cols]));
	return cmat_move(&res);
}

static inline
cmat_tmp cmat_from_array2d(size_t rows, size_t cols, float data[static rows][cols]) {
	return cmat_from_raw(rows, cols, data);
}

static inline
cmat_tmp cmat_from_array1d(size_t rows, size_t cols, float data[static rows * cols]) {
	return cmat_from_raw(rows, cols, data);
}

static inline
void cmat_del(cmat *m) {
	if (!cmat_is_scalar(*m))
		free(m->data);
	*m = (cmat) { .status = CMAT_DELETED };
}

#define CMAT_TO_VLA(m, vla) float (*vla)[m.cols] = CMAT__DATA(m)

static inline cmat cmat_get(cmat_tmp tmp) { return tmp.get; }
static inline cmat cmat_noop(cmat m) { return m; }

cmat_tmp cmat_add__internal(cmat a, cmat b, bool del_a, bool del_b);

#define CMAT__OP_UNPACK(op, a, b) \
	cmat_ ## op ## _internal( \
		_Generic((a), cmat: cmat_noop, cmat_tmp: cmat_get)(a), \
		_Generic((b), cmat: cmat_noop, cmat_tmp: cmat_get)(b), \
		_Generic((a), cmat: 0, cmat_tmp: 1),                   \
		_Generic((b), cmat: 0, cmat_tmp: 1))

#define cmat_add(a,b) CMAT__OP_UNPACK(add, a, b)

#if 0
//#define CMAT_IS_TMP(m) _Generic((m), cmat: 0, cmat_tmp: 1)
cmat_tmp cmat_add_mm(cmat a, cmat b);

static inline
cmat_tmp cmat_add_mt(cmat a, cmat_tmp b) {
	cmat_tmp res = cmat_add_mm(a, b.get);
	cmat_del(&b.get);
	return res;
}

static inline
cmat_tmp cmat_add_tm(cmat_tmp a, cmat b) {
	cmat_tmp res = cmat_add_mm(a.get, b);
	cmat_del(&a.get);
	return res;
}

static inline
cmat_tmp cmat_add_tt(cmat_tmp a, cmat_tmp b) {
	cmat_tmp res = cmat_add_mm(a.get, b.get);
	cmat_del(&a.get);
	cmat_del(&b.get);
	return res;
}
#define cmat_add(a,b) \
	(_Generic((a), cmat:     _Generic((b), cmat: cmat_add_mm, cmat_tmp: cmat_add_mt), \
	               cmat_tmp: _Generic((b), cmat: cmat_add_tm, cmat_tmp: cmat_add_tt))(a,b))
#endif


#ifdef CMAT_IMPLEMENTATION

#if 0
// instantiations of inline functions

bool cmat_is_scalar(cmat m);
cmat cmat_scalar(float val);
cmat_tmp cmat_create(size_t rows, size_t cols);
cmat_tmp cmat_from_array(size_t rows, size_t cols, float data[static rows][cols]);
void cmat_del(cmat* m);
cmat_tmp cmat_move(cmat* m);
cmat_tmp cmat_add_mt(cmat a, cmat_tmp b);
cmat_tmp cmat_add_tm(cmat_tmp a, cmat b);
cmat_tmp cmat_add_tt(cmat_tmp a, cmat_tmp b);
#endif

cmat_tmp cmat_add_internal(cmat a, cmat b, bool del_a, bool del_b) {
	size_t rows = a.rows > b.rows ? a.rows : b.rows;
	size_t cols = a.cols > b.cols ? a.cols : b.cols;

	cmat c = { 0 };

	c.status = CMAT_ERR_INVALID_OPERAND;
	if (a.status != CMAT_VALID || b.status != CMAT_VALID)
		goto done;

	c.status = CMAT_ERR_CANT_BROADCAST;
	if ((a.rows > 1 && b.rows > 1 && a.rows != b.rows) ||
	    (a.cols > 1 && b.cols > 1 && a.cols != b.cols))
		goto done;

	c = cmat_create(rows, cols).get;
	if (c.status != CMAT_VALID)
		goto done;

	size_t dra = (a.rows == rows);
	size_t drb = (b.rows == rows);
	size_t dca = (a.cols == cols);
	size_t dcb = (b.cols == cols);

	// limit scope of VLA types
	{
		CMAT_TO_VLA(a, adata);
		CMAT_TO_VLA(b, bdata);
		CMAT_TO_VLA(c, cdata);

		for (size_t r = 0, ra = 0, rb = 0; r < rows; r++, ra += dra, rb += drb) {
		for (size_t c = 0, ca = 0, cb = 0; c < cols; c++, ca += dca, cb += dcb) {
			cdata[r][c] = adata[ra][ca] + bdata[rb][cb];
		}}
	}

done:
	if (del_a) cmat_del(&a);
	if (del_b) cmat_del(&b);

	return cmat_move(&c);
}

#endif
