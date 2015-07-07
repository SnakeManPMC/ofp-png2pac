#include "stdafx.h"
#include "PAAFile.h"
#include "compress.h"

static char *tag_offset = "GGATSFFO";
static char *tag_avgcol = "GGATCGVA";
static char *tag_alpha = "GGATGALF";

PAAMipmap::PAAMipmap(const int mtype, const int mwidth, const int mheight, unsigned char *source) {
	width = mwidth;
	height = mheight;
/*
	if (mtype == 0x4444) {
		unsigned char *work_buffer = new unsigned char[width * height * 2];
		for (int i = 0; i < width * height; i++) {
			work_buffer[i * 2 + 0] = (source[i * 4 + 0] >> 4) | (source[i * 4 + 1] & 0xF0);
			work_buffer[i * 2 + 1] = (source[i * 4 + 2] >> 4) | (source[i * 4 + 3] & 0xF0);
		}
		data = new unsigned char[width * height * 3];
		size = compress(data, work_buffer, width * height * 2);
		delete[] work_buffer;
		return;
	}
*/
	if (mtype == 0x1555 || mtype == 0x8080 || mtype == 0x4444 || mtype == 0x0565) {
		data = new unsigned char[width * height * 3];
		size = compress(data, source, width * height * 2);
		return;
	}

	if (mtype == 0x8888) {
		data = new unsigned char[width * height * 5];
		size = compress(data, source, width * height * 4);
		return;
	}
	if (mtype == 0xFF01) {
		size = width * height / 2;
		data = new unsigned char[size];
		memcpy(data, source, size);
		return;
	}
	if (mtype == 0xFF03 || mtype == 0xFF05) {
		size = width * height;
		data = new unsigned char[size];
		memcpy(data, source, size);
		return;
	}

	// unknown type
	printf("unknown mipmap type %02x, aborting\n", mtype);
	exit(0);
}

void PAAFile::addMipmap(PAAMipmap *mipmap) {
	if (mipmap_num > 15) return;
	mipmaps[mipmap_num] = mipmap;
	mipmap_num++;
}

int PAAFile::writeFile(FILE *f) {
	int tmp_i;
	long offsets_pos;

	// file type
	fwrite(&type, 2, 1, f);

	// average color
	fwrite(tag_avgcol, 8, 1, f);
	tmp_i = 4;
	fwrite(&tmp_i, 4, 1, f);
	fwrite(avgcolor, 4, 1, f);

	if (flag) {
		fwrite(tag_alpha, 8, 1, f);
		tmp_i = 4;
		fwrite(&tmp_i, 4, 1, f);
		fwrite(&flag, 4, 1, f);
	}

	// offsets
	fwrite(tag_offset, 8, 1, f);	
	tmp_i = 16 * 4;
	fwrite(&tmp_i, 4, 1, f);

	offsets_pos = ftell(f);

	fwrite(offsets, 16 * 4, 1, f);

	// end-of-headers marker
	tmp_i = 0;
	fwrite(&tmp_i, 2, 1, f);

	// write mipmaps
	for (int i = 0; i < 16; i++) {
		if (mipmaps[i] != NULL) {
			offsets[i] = ftell(f);
			fwrite(&mipmaps[i]->width, 2, 1, f);
			fwrite(&mipmaps[i]->height, 2, 1, f);
			fwrite(&mipmaps[i]->size, 3, 1, f);
			fwrite(mipmaps[i]->data, mipmaps[i]->size, 1, f);
		}
	}

	// end-of-mipmaps marker
	tmp_i = 0;
	fwrite(&tmp_i, 4, 1, f);
	fwrite(&tmp_i, 2, 1, f);

	// write offsets again
	fseek(f, offsets_pos, SEEK_SET);
	fwrite(offsets, 16 * 4, 1, f);

	return 0;
}

