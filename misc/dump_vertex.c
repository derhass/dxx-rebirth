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

static void
dv_add_tri(dv_obj_t *o, GLfloat *vtxA, GLfloat *vtxB, GLfloat *vtxC,  GLfloat *clrA, GLfloat *clrB, GLfloat *clrC, GLfloat *texA, GLfloat *texB, GLfloat *texC, GLfloat *nrmA, GLfloat *nrmB, GLfloat *nrmC)
{
	GLuint flags = 0;
	GLuint i;

	if (o->vcnt + 3 > DV_MAX_VTX) {
		return;
	}
	if (o->tcnt + 1 > DV_MAX_TRI) {
		return;
	}

	if (vtxA && vtxB && vtxC) {
		flags |= DV_VTX;
		for (i=0; i<3; i++) {
			o->vtx[o->vcnt+0][i] = vtxA[i];
			o->vtx[o->vcnt+1][i] = vtxB[i];
			o->vtx[o->vcnt+2][i] = vtxC[i];
		}
	}
	if (clrA && clrB && clrC) {
		flags |= DV_CLR;
		for (i=0; i<4; i++) {
			o->clr[o->vcnt+0][i] = clrA[i];
			o->clr[o->vcnt+1][i] = clrB[i];
			o->clr[o->vcnt+2][i] = clrC[i];
		}
	}
	if (texA && texB && texC) {
		flags |= DV_TEX;
		for (i=0; i<2; i++) {
			GLfloat f=(i==1)?-1.f:1.f;
			o->tex[o->vcnt+0][i] = f*texA[i];
			o->tex[o->vcnt+1][i] = f*texB[i];
			o->tex[o->vcnt+2][i] = f*texC[i];
		}
	}
	if (nrmA && nrmB && nrmC) {
		flags |= DV_NRM;
		for (i=0; i<3; i++) {
			o->nrm[o->vcnt+0][i] = nrmA[i];
			o->nrm[o->vcnt+1][i] = nrmB[i];
			o->nrm[o->vcnt+2][i] = nrmC[i];
		}
	}
	if (!flags) {
		return;
	}

	if (!(flags&DV_NRM)) {
		GLfloat a[3],b[3],n[4];
		flags |= DV_NRM;
		for (i=0; i<3; i++) {
			/*
			a[i] =  o->vtx[o->vcnt+2 - dump_vertex_meta.facing][i] - o->vtx[o->vcnt+0][i];
			b[i] =  o->vtx[o->vcnt+1 + dump_vertex_meta.facing][i] - o->vtx[o->vcnt+0][i];
			*/
			a[i] =  o->vtx[o->vcnt+1][i] - o->vtx[o->vcnt+0][i];
			b[i] =  o->vtx[o->vcnt+2][i] - o->vtx[o->vcnt+0][i];
		}
		n[0] = a[1]*b[2] - a[2]*b[1];
		n[1] = a[2]*b[0] - a[0]*b[2];
		n[2] = a[0]*b[1] - a[1]*b[0];
		n[3] = sqrt(n[0] * n[0] + n[1]*n[1] + n[2]*n[2]);
		for (i=0; i<3; i++) {
			n[i] = n[i] / n[3];
		}
		for (i=0; i<3; i++) {
			o->nrm[o->vcnt+i][0] = n[0];
			o->nrm[o->vcnt+i][1] = n[1];
			o->nrm[o->vcnt+i][2] = n[2];
		}
	}
	for (i=0; i<3; i++) {
		o->flags[o->vcnt+i] = flags;
	}

	o->tri[o->tcnt][0] = o->vcnt;
	o->tri[o->tcnt][1] = o->vcnt + 1;
	o->tri[o->tcnt][2] = o->vcnt + 2;
	o->tcnt += 1;
	o->vcnt += 3;
}

extern void
dv_add_tfan(dv_t *dv, GLuint tex_idx, GLuint cnt, GLfloat *vtx, GLfloat *clr, GLfloat *tex, GLfloat *nrm)
{
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

	for (i=0; i<cnt -2; i++) {
		GLuint j=i+2 - dump_vertex_meta.facing;
		GLuint k=i+1 + dump_vertex_meta.facing;
		dv_add_tri(o, vtx, vtx+3*j, vtx+3*k, clr, clr+4*j, clr+4*k, tex, tex+2*j, tex+2*k, nrm, nrm+3*j, nrm+3*k);
	}
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

