#ifndef DUMP_VERTEX_H
#define DUMP_VERTEX_H

#include <GL/gl.h>

#define DV_MAX_TRI	1024
#define DV_MAX_VTX	(3*DV_MAX_TRI)
#define DV_MAX_OBJ	16

#define DV_VTX	0x1
#define DV_CLR	0x2
#define	DV_TEX	0x4
#define DV_NRM	0x8

typedef struct {
	unsigned int idx;
	int facing;
} dv_meta_t;

typedef struct {
	GLuint tex_idx;
	GLfloat vtx[DV_MAX_VTX][3];
	GLfloat clr[DV_MAX_VTX][4];
	GLfloat tex[DV_MAX_VTX][2];
	GLfloat nrm[DV_MAX_VTX][3];
	GLuint flags[DV_MAX_VTX];
	GLuint tri[DV_MAX_TRI][3];
	GLuint vcnt;
	GLuint tcnt;
} dv_obj_t;

typedef struct {
	dv_obj_t obj[DV_MAX_OBJ];
	GLuint ocnt;
} dv_t;

extern dv_t*
dv_create();

extern void
dv_add_tfan(dv_t *dv, GLuint tex_idx, GLuint cnt, GLfloat *vtx, GLfloat *clr, GLfloat *tex, GLfloat *nrm);

extern void
dv_finish(dv_t *dv);

extern dv_t *dump_vertex_data;
extern dv_meta_t dump_vertex_meta;

#endif /* DUMP_VERTEX_H */
