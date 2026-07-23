#ifndef MESH_CONTEXT_H
#define MESH_CONTEXT_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MESH_CONTEXT_EPS
#define MESH_CONTEXT_EPS 1.0e-30
#endif

typedef struct MeshContext {
	int nr;
	int nz;
	const double *rh;
	const double *zh;

	double z_gap;
	double z_reaction_fall;
	double z_reaction_first;
	double z_anode;
	double z_current_integral_max;
	double z_power_integral_max;
	double z_streak_max;
	double z_ey_ne_max;
	double z_trigger_priflag2;
	double z_limiter_initial;
	double z_limiter_after;

	double r_reaction_max;
	double r_plate_current_max;

	int j_gap;
	int j_reaction_fall;
	int j_reaction_first;
	int j_anode;
	int j_current_end;
	int j_power_end;
	int j_streak_end;
	int j_ey_ne_end;
	int j_limiter_initial;
	int j_limiter_after;
	int j_trigger_priflag2_nearest;

	int i_reaction_end;
	int i_plate_current_end;
} MeshContext;

static inline int mesh_first_geq(const double *x, int n, double value)
{
	int lo = 0;
	int hi = n;

	while (lo < hi) {
		int mid = lo + (hi - lo) / 2;
		if (x[mid] < value) lo = mid + 1;
		else hi = mid;
	}

	return lo;
}

static inline int mesh_nearest_index(const double *x, int n, double value)
{
	int k = mesh_first_geq(x, n, value);

	if (k <= 0) return 0;
	if (k >= n) return n - 1;

	return (fabs(x[k - 1] - value) <= fabs(x[k] - value)) ? (k - 1) : k;
}

static inline double mesh_sample_axis_linear(double **field, const double *z, int nz, double z0)
{
	int k = mesh_first_geq(z, nz, z0);

	if (k <= 0) return field[0][0];
	if (k >= nz) return field[0][nz - 1];

	double z_left = z[k - 1];
	double z_right = z[k];
	double den = z_right - z_left;

	if (fabs(den) < MESH_CONTEXT_EPS) return field[0][k];

	double w = (z0 - z_left) / den;
	return (1.0 - w) * field[0][k - 1] + w * field[0][k];
}

static inline int mesh_is_strictly_monotone(const double *x, int n)
{
	int k;

	if (n <= 0 || !isfinite(x[0])) return 0;

	for (k = 1; k < n; ++k) {
		if (!(x[k] > x[k - 1]) || !isfinite(x[k])) return 0;
	}

	return 1;
}

static inline void mesh_context_set_defaults(MeshContext *mc)
{
	memset(mc, 0, sizeof(*mc));
}

static inline int mesh_context_load_cfg(MeshContext *mc, const char *path)
{
	FILE *fp = fopen(path, "r");
	char key[128];
	char eq[8];
	double val;
	unsigned int seen = 0u;
	const unsigned int required = (1u << 13) - 1u;

	if (!fp) return -1;

	while (fscanf(fp, " %127[^= \t\n] %7[=] %le", key, eq, &val) == 3) {
		if (strcmp(key, "z_gap") == 0) {
			mc->z_gap = val;
			seen |= 1u << 0;
		} else if (strcmp(key, "z_reaction_fall") == 0) {
			mc->z_reaction_fall = val;
			seen |= 1u << 1;
		} else if (strcmp(key, "z_reaction_first") == 0) {
			mc->z_reaction_first = val;
			seen |= 1u << 2;
		} else if (strcmp(key, "z_anode") == 0) {
			mc->z_anode = val;
			seen |= 1u << 3;
		} else if (strcmp(key, "z_current_integral_max") == 0) {
			mc->z_current_integral_max = val;
			seen |= 1u << 4;
		} else if (strcmp(key, "z_power_integral_max") == 0) {
			mc->z_power_integral_max = val;
			seen |= 1u << 5;
		} else if (strcmp(key, "z_streak_max") == 0) {
			mc->z_streak_max = val;
			seen |= 1u << 6;
		} else if (strcmp(key, "z_ey_ne_max") == 0) {
			mc->z_ey_ne_max = val;
			seen |= 1u << 7;
		} else if (strcmp(key, "z_trigger_priflag2") == 0) {
			mc->z_trigger_priflag2 = val;
			seen |= 1u << 8;
		} else if (strcmp(key, "z_limiter_initial") == 0) {
			mc->z_limiter_initial = val;
			seen |= 1u << 9;
		} else if (strcmp(key, "z_limiter_after") == 0) {
			mc->z_limiter_after = val;
			seen |= 1u << 10;
		} else if (strcmp(key, "r_reaction_max") == 0) {
			mc->r_reaction_max = val;
			seen |= 1u << 11;
		} else if (strcmp(key, "r_plate_current_max") == 0) {
			mc->r_plate_current_max = val;
			seen |= 1u << 12;
		}

		int c;
		while ((c = fgetc(fp)) != EOF && c != '\n') { }
	}

	fclose(fp);

	return (seen == required) ? 0 : -2;
}

static inline int mesh_context_finalize(MeshContext *mc, int nr, int nz, const double *rh, const double *zh)
{
	if (nr <= 0 || nz <= 0 || !rh || !zh) return -1;

	mc->nr = nr;
	mc->nz = nz;
	mc->rh = rh;
	mc->zh = zh;

	if (!mesh_is_strictly_monotone(rh, nr)) return -2;
	if (!mesh_is_strictly_monotone(zh, nz)) return -3;

	mc->j_gap = mesh_first_geq(zh, nz, mc->z_gap);
	mc->j_reaction_fall = mesh_first_geq(zh, nz, mc->z_reaction_fall);
	mc->j_reaction_first = mesh_first_geq(zh, nz, mc->z_reaction_first);
	mc->j_anode = mesh_first_geq(zh, nz, mc->z_anode);
	mc->j_current_end = mesh_first_geq(zh, nz, mc->z_current_integral_max);
	mc->j_power_end = mesh_first_geq(zh, nz, mc->z_power_integral_max);
	mc->j_streak_end = mesh_first_geq(zh, nz, mc->z_streak_max);
	mc->j_ey_ne_end = mesh_first_geq(zh, nz, mc->z_ey_ne_max);
	mc->j_limiter_initial = mesh_first_geq(zh, nz, mc->z_limiter_initial);
	mc->j_limiter_after = mesh_first_geq(zh, nz, mc->z_limiter_after);
	mc->j_trigger_priflag2_nearest = mesh_nearest_index(zh, nz, mc->z_trigger_priflag2);

	mc->i_reaction_end = mesh_first_geq(rh, nr, mc->r_reaction_max);
	mc->i_plate_current_end = mesh_first_geq(rh, nr, mc->r_plate_current_max);

	if (mc->j_gap < 0 || mc->j_gap > nz) return -4;
	if (mc->j_current_end < 0 || mc->j_current_end > nz) return -5;
	if (mc->j_power_end < 0 || mc->j_power_end > nz) return -6;
	if (mc->j_streak_end < 0 || mc->j_streak_end > nz) return -7;
	if (mc->j_ey_ne_end < 0 || mc->j_ey_ne_end > nz) return -8;
	if (mc->i_reaction_end < 0 || mc->i_reaction_end > nr) return -9;
	if (mc->i_plate_current_end < 0 || mc->i_plate_current_end > nr) return -10;

	return 0;
}

static inline int mesh_reaction_sink_bin(const MeshContext *mc, int i, int j)
{
	double r = mc->rh[i];
	double z = mc->zh[j];

	if (r >= mc->r_reaction_max) return -1;
	if (z >= mc->z_gap) return -1;

	if (z < mc->z_reaction_fall) return 0;
	if (z < mc->z_reaction_first) return 1;
	if (z < mc->z_anode) return 2;
	return 3;
}

static inline void mesh_context_print(const MeshContext *mc, FILE *fp)
{
	if (!fp) fp = stdout;

	fprintf(fp, "MeshContext:\n");
	fprintf(fp, "  z_gap                  %.17e -> j_gap=%d\n", mc->z_gap, mc->j_gap);
	fprintf(fp, "  z_reaction_fall        %.17e -> j=%d\n", mc->z_reaction_fall, mc->j_reaction_fall);
	fprintf(fp, "  z_reaction_first       %.17e -> j=%d\n", mc->z_reaction_first, mc->j_reaction_first);
	fprintf(fp, "  z_anode                %.17e -> j=%d\n", mc->z_anode, mc->j_anode);
	fprintf(fp, "  z_current_integral_max %.17e -> j_end=%d\n", mc->z_current_integral_max, mc->j_current_end);
	fprintf(fp, "  z_power_integral_max   %.17e -> j_end=%d\n", mc->z_power_integral_max, mc->j_power_end);
	fprintf(fp, "  z_streak_max           %.17e -> j_end=%d\n", mc->z_streak_max, mc->j_streak_end);
	fprintf(fp, "  z_ey_ne_max            %.17e -> j_end=%d\n", mc->z_ey_ne_max, mc->j_ey_ne_end);
	fprintf(fp, "  z_trigger_priflag2     %.17e -> j_nearest=%d\n", mc->z_trigger_priflag2, mc->j_trigger_priflag2_nearest);
	fprintf(fp, "  z_limiter_initial      %.17e -> j=%d\n", mc->z_limiter_initial, mc->j_limiter_initial);
	fprintf(fp, "  z_limiter_after        %.17e -> j=%d\n", mc->z_limiter_after, mc->j_limiter_after);
	fprintf(fp, "  r_reaction_max         %.17e -> i_end=%d\n", mc->r_reaction_max, mc->i_reaction_end);
	fprintf(fp, "  r_plate_current_max    %.17e -> i_end=%d\n", mc->r_plate_current_max, mc->i_plate_current_end);
}

#endif
