#ifndef __PAAFILE_H
#define __PAAFILE_H

class PAAMipmap {
public:
	int width;
	int height;
	int size;
	unsigned char *data;

	PAAMipmap(const int type, const int width, const int height, unsigned char *source);
};
class PAAFile {
public:
	int type;
	int flag;
	int offsets[16];
	unsigned char avgcolor[4];
	int mipmap_num;

	PAAMipmap *mipmaps[16];

	PAAFile() {
		mipmap_num = 0;
		type = 0x4444;
		flag = 0;
		for (int i = 0; i < 16; i++) {
			offsets[i] = 0;
			mipmaps[i] = 0;
		}
		for (int i = 0; i < 4; i++)
			avgcolor[i] = 0xFF;
	};
	int writeFile(FILE *f);
	void addMipmap(PAAMipmap *mipmap);
};

#endif