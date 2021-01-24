#include "dump_vertex.h"
#include "lupng.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

dv_meta_t dump_vertex_meta = { 0,0 };
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
	char buf[512];
	char sbase[256];
	char base[256];
	char stexname[512];
	char texname[512];
	FILE *f;
	GLuint v,t;

	stexname[0]=0;
	texname[0]=0;
	snprintf(sbase, sizeof(sbase), "dxx_dv_%u_%u", dump_vertex_meta.idx, i);
	snprintf(base, sizeof(base), "/tmp/%s", sbase);

	/* GEOMETRY: .obj */
	snprintf(buf, sizeof(buf), "%s.obj", base);
	f=fopen(buf, "wt");
	if (!f) {
		return;
	}

	snprintf(buf, sizeof(buf), "%s.mtl", sbase);
	fprintf(f,"mtllib %s\n",buf);
	for (v=0; v<o->vcnt; v++) {
		fprintf(f,"v %f %f %f\n", o->vtx[v][0], o->vtx[v][1], o->vtx[v][2]);
	}
	for (v=0; v<o->vcnt; v++) {
		fprintf(f,"vt %f %f\n", o->tex[v][0], o->tex[v][1]);
	}
	for (v=0; v<o->vcnt; v++) {
		fprintf(f,"vn %f %f %f\n", o->nrm[v][0], o->nrm[v][1], o->nrm[v][2]);
	}
	fprintf(f,"usemtl %s\n",sbase);
	for (t=0; t<o->tcnt; t++) {
		if ( (o->flags[o->tri[t][0]] & (DV_TEX | DV_NRM)) == (DV_TEX | DV_NRM))  {
			fprintf(f,"f %u/%u/%u %u/%u/%u %u/%u/%u\n", 
					o->tri[t][0]+1, o->tri[t][0]+1, o->tri[t][0]+1,
					o->tri[t][1]+1, o->tri[t][1]+1, o->tri[t][1]+1,
					o->tri[t][2]+1, o->tri[t][2]+1, o->tri[t][2]+1);
		} else if (o->flags[o->tri[t][0]] & DV_TEX) {
			fprintf(f,"f %u/%u %u/%u %u/%u\n", 
					o->tri[t][0]+1, o->tri[t][0]+1, o->tri[t][1]+1,
					o->tri[t][1]+1, o->tri[t][2]+1, o->tri[t][2]+1);
		} else {
			fprintf(f,"f %u %u %u\n", o->tri[t][0]+1, o->tri[t][1]+1, o->tri[t][2]+1);
		}

	}

	fclose(f);

	if (o->tex_idx) {
		/* TEXTURE */
		GLint w=0,h=0;

		glBindTexture(GL_TEXTURE_2D, o->tex_idx);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
		if (w > 0 && h > 0) {
			LuImage *img = luImageCreate(w,h,4,8,NULL,NULL);
			if (img) {
				glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, img->data);
				snprintf(stexname, sizeof(stexname), "%s_tex%u_%dx%d.png", sbase,o->tex_idx,w,h);
				snprintf(texname, sizeof(texname), "%s_tex%u_%dx%d.png", base,o->tex_idx,w,h);
				luPngWriteFile(texname, img);
				luImageRelease(img, NULL);
			}
		}
	}

	/* MTL */
	snprintf(buf, sizeof(buf), "%s.mtl", base);
	f=fopen(buf, "wt");
	if (!f) {
		return;
	}

	fprintf(f,"newmtl %s\n",sbase);
	fprintf(f,"\tKa 1.0 1.0 1.0\n");
	fprintf(f,"\tKd 1.0 1.0 1.0\n");
	fprintf(f,"\tKs 0.0 0.0 0.0\n");
	if (texname[0]) {
		fprintf(f,"\n\tmap_Ka %s\n",stexname);
		fprintf(f,"\n\tmap_Kd %s\n",stexname);
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

	/*
	if (dump_vertex_meta.facing > 0) {
		dump_vertex_meta.facing=1;
	} else {
		dump_vertex_meta.facing=0;
	}
	*/
	dump_vertex_meta.facing = 0;

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
			o->tex[o->vcnt+i][1] = -tex[2*i+1];
		}
	}
	if (nrm) {
		flags |= DV_NRM;
		for (i=0; i<cnt; i++) {
			o->nrm[o->vcnt+i][0] = nrm[3*i+0];
			o->nrm[o->vcnt+i][1] = nrm[3*i+1];
			o->nrm[o->vcnt+i][2] = nrm[3*i+2];
		}
	}
	if (!flags) {
		return;
	}
	if (!(flags&DV_NRM)) {
		GLfloat a[3],b[3],n[4];
		flags |= DV_NRM;
		for (i=0; i<3; i++) {
			a[i] =  o->vtx[o->vcnt+2 - dump_vertex_meta.facing][i] - o->vtx[o->vcnt+0][i];
			b[i] =  o->vtx[o->vcnt+1 + dump_vertex_meta.facing][i] - o->vtx[o->vcnt+0][i];
		}
		n[0] = a[1]*b[2] - a[2]*b[1];
		n[1] = a[2]*b[0] - a[0]*b[2];
		n[2] = a[0]*b[1] - a[1]*b[0];
		n[3] = sqrt(n[0] * n[0] + n[1]*n[1] + n[2]*n[2]);
		for (i=0; i<3; i++) {
			n[i] = n[i] / n[3];
		}
		for (i=0; i<cnt; i++) {
			o->nrm[o->vcnt+i][0] = n[0];
			o->nrm[o->vcnt+i][1] = n[1];
			o->nrm[o->vcnt+i][2] = n[2];
		}
	}
	for (i=0; i<cnt; i++) {
		o->flags[o->vcnt+i] = flags;
	}
	for (i=0; i<cnt -2; i++) {
		o->tri[o->tcnt + i][0] = o->vcnt;
		o->tri[o->tcnt + i][1] = o->vcnt + i + 2 - dump_vertex_meta.facing;
		o->tri[o->tcnt + i][2] = o->vcnt + i + 1 + dump_vertex_meta.facing;
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

