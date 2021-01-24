#include "dump_vertex.h"

#include <stdlib.h>
#include <stdio.h>

dv_meta_t dump_vertex_meta = { 0 };
dv_t *dump_vertex_data = NULL;

static void
dv_obj_init(dv_obj_t *o)
{
	o->tex_idx = 0;
	o->tcnt = 0;
	o->vcnt = 0;
}

static void
dv_obj_dump_as_obj(dv_obj_t *o, GLuint i)
{
	char buf[256];
	FILE *f;
	GLuint v,t;

	snprintf(buf, sizeof(buf), "/tmp/dxx_dv_%u_%u.obj", dump_vertex_meta.idx, i);
	f=fopen(buf, "wt");
	if (!f) {
		return;
	}

	for (v=0; v<o->vcnt; v++) {
		fprintf(f,"v %f %f %f\n", o->vtx[v][0], o->vtx[v][1], o->vtx[v][2]);
	}
	for (v=0; v<o->vcnt; v++) {
		fprintf(f,"vt %f %f\n", o->tex[v][0], o->tex[v][1]);
	}
	for (t=0; t<o->tcnt; t++) {
		if (o->flags[o->tri[t][0]] & DV_TEX) {
			fprintf(f,"f %u/%u %u/%u %u/%u\n", 
					o->tri[t][0]+1, o->tri[t][0]+1, o->tri[t][1]+1,
					o->tri[t][1]+1, o->tri[t][2]+1, o->tri[t][2]+1);
		} else {
			fprintf(f,"f %u %u %u\n", o->tri[t][0]+1, o->tri[t][1]+1, o->tri[t][2]+1);
		}

	}

	fclose(f);

}

static void
dv_init(dv_t *dv)
{
	dv->ocnt = 0;
}

extern dv_t*
dv_create()
{
	dump_vertex_meta.idx++;
	dv_t *dv = malloc(sizeof(dv_t));
	if (dv) {
		dv_init(dv);
	}
	return dv;
}

static dv_obj_t *
dv_find_obj(dv_t *dv, GLuint tex)
{
	GLuint i;
	for (i=0; i<dv->ocnt; i++) {
		if (dv->obj[i].tex_idx == tex) {
			return &dv->obj[i];
		}
	}
	return NULL;
}

static dv_obj_t *
dv_find_or_create_obj(dv_t *dv, GLuint tex)
{
	dv_obj_t *o = dv_find_obj(dv, tex);
	if (!o) {
		if (dv->ocnt < DV_MAX_OBJ) {
			o=&dv->obj[dv->ocnt++];
			dv_obj_init(o);
			o->tex_idx = tex;
		}
	}
	return o;
}

extern void
dv_add_tfan(dv_t *dv, GLuint tex_idx, GLuint cnt, GLfloat *vtx, GLfloat *clr, GLfloat *tex, GLfloat *nrm)
{
	GLuint flags = 0;
	GLuint i;
	dv_obj_t *o;

	if (!dv || cnt < 3) {
		return;
	}

	o = dv_find_or_create_obj(dv, tex_idx);
	if (!o) {
		return;
	}

	if (o->vcnt + cnt > DV_MAX_VTX) {
		return;
	}
	if (o->tcnt + cnt - 2 > DV_MAX_TRI) {
		return;
	}

	if (vtx) {
		flags |= DV_VTX;
		for (i=0; i<cnt; i++) {
			o->vtx[o->vcnt+i][0] = vtx[3*i+0];
			o->vtx[o->vcnt+i][1] = vtx[3*i+1];
			o->vtx[o->vcnt+i][2] = vtx[3*i+2];
		}
	}
	if (clr) {
		flags |= DV_CLR;
		for (i=0; i<cnt; i++) {
			o->clr[o->vcnt+i][0] = clr[4*i+0];
			o->clr[o->vcnt+i][1] = clr[4*i+1];
			o->clr[o->vcnt+i][2] = clr[4*i+2];
			o->clr[o->vcnt+i][3] = clr[4*i+3];
		}
	}
	if (tex) {
		flags |= DV_TEX;
		for (i=0; i<cnt; i++) {
			o->tex[o->vcnt+i][0] = tex[2*i+0];
			o->tex[o->vcnt+i][1] = tex[2*i+1];
		}
	}
	if (!flags) {
		return;
	}
	for (i=0; i<cnt; i++) {
		o->flags[o->vcnt+i] = flags;
	}
	for (i=0; i<cnt -2; i++) {
		o->tri[o->tcnt + i][0] = o->vcnt;
		o->tri[o->tcnt + i][1] = o->vcnt + i + 1;
		o->tri[o->tcnt + i][2] = o->vcnt + i + 2;
	}
	o->tcnt += cnt - 2;
	o->vcnt += cnt;
}

extern void
dv_finish(dv_t *dv)
{
	GLuint i;

	if (!dv) {
		return;
	}

	for (i=0; i<dv->ocnt; i++) {
		dv_obj_dump_as_obj(&dv->obj[i], i);
	}

	free(dv);
}

