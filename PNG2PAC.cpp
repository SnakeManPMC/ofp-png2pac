// PNG2PAC.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "compress.h"
#include "PAAFile.h"
#include <malloc.h>
#include <dxtlib.h>

#pragma pack(1)
typedef struct {
	char identsize;
	char colormaptype;
	char imagetype;

	short colormapstart;
	short colormaplength;
	char colormapbits;

	short xstart;
	short ystart;
	short width;
	short height;
	char bits;
	char descriptor;
} tga_header;

#pragma pack(8)

enum TextureType {
	txtUnknown,
	txtDXT1,
	txtDXT1a,
	txtDXT3,	
	txtDXT5,	
	txt4444,
	txt1555,
	txtIA88,
	txt8888,
	txt565,
	txtAutoDetect
};

void usage(char *programname) {
	printf("Converts TGA textures to Operation Flashpoint PAA format.\n\n");
	printf("Usage: %s -type source destination\n\n", programname);
	printf("  -type\tDestination file type, must be one of following:\n");
	printf("\tdxt1\tDXT1 compression\n");
	printf("\tdxt1a\tDXT1 compression with 1-bit alpha channel\n");
	printf("\t4444\tRGBA 4:4:4:4\n");
	printf("\t1555\tRGBA 5:5:5:1\n");
	printf("\tia88\tIA88, black&white texture with alpha channel\n");
	printf("  source\tSpecifies 32-bit TGA image file to be converted\n");
	printf("  destination\tSpecifies OFP texture file to be written\n");
	printf("\n");
	printf("Example: %s -5551 texture.tga texture.pac\n", programname);
	printf("\n");
	printf("If executed with only one parameter, TGA file specified by parameter is\n");
	printf("converted with destination type autodetection.\n");
	printf("\n");
	printf("%s (C) 2004 feersum.endjinn@luukku.com\n", programname);
	printf("Contains portions of NVIDIA DXT library, (C) 2003 NVIDIA Corporation\n");
}

CompressionOptions options;

PAAFile *callback_paafile;
int callback_width, callback_height;
HRESULT mipmapCallback(void *data, int level, DWORD size) {
	int width = callback_width >> level;
	int height = callback_height >> level;

	if (width > 2 && height > 2) {
		printf("mipmap %d, size %08x, %dx%d\n", level, size, width, height);

		PAAMipmap *mipmap = new PAAMipmap(callback_paafile->type, width, height, (unsigned char *)data);
		callback_paafile->addMipmap(mipmap);
	}

	return 0;
}

void flip_vertical(unsigned char *data, const int width, const int height) {
	unsigned char *temp = new unsigned char[width * height * 4];

	for (int y = 0; y < height; y++)
		memcpy(temp + (height - y - 1) * width * 4, data + y * width * 4, width * 4);

	memcpy(data, temp, width * height * 4);

	delete[] temp;
}

enum TextureType autodetect_type(const unsigned char *data, const int size) {
	int alpha = 0;
	int bwimg = 1;

	for (int i = 0; i < size; i++) {
		if (data[i * 4 + 3] == 0) {
			alpha = 1;
		}
		if (data[i * 4 + 3] > 0 && data[i * 4 + 3] < 0xff) {
			alpha = 2;
			break;
		}
	}

	for (int i = 0; i < size; i++) {
		if (data[i * 4 + 0] != data[i * 4 + 1] || data[i * 4 + 1] != data[i * 4 + 2]) {
			bwimg = 0;
			break;
		}
	}

	if (bwimg == 1) return txtIA88;
	if (alpha == 0) return txtDXT1;
	if (alpha == 1) return txtDXT1a;
	if (alpha == 2) return txt4444;

	return txtDXT1;
}

void calc_average_color(const unsigned char *data, const int size, unsigned char *dest) {
	int r, g, b, a;
	r = g = b = a = 0;
	for (int i = 0; i < size; i++) {
		r += data[i * 4 + 0];
		g += data[i * 4 + 1];
		b += data[i * 4 + 2];
		a += data[i * 4 + 3];
	}
	dest[0] = (unsigned char)(r / size);
	dest[1] = (unsigned char)(g / size);
	dest[2] = (unsigned char)(b / size);
	dest[3] = (unsigned char)(a / size);
}

int has_transparency(const unsigned char *data, const int size) {
	for (int i = 0; i < size; i++)
		if (data[i * 4 + 3] != 0xFF) return 1;
	return 0;
}

void make_mipmap(unsigned char *data, const int width, const int height) {
	unsigned char *temp = new unsigned char[(width / 2)* (height / 2) * 4];

	for (int i = 0; i < height / 2; i++)
		for (int j = 0; j < width / 2; j++) {
			int r, g, b, a;
			r = g = b = a = 0;

			r += data[((i * 2 + 0) * width * 4) + (j * 2 + 0) * 4 + 0];
			r += data[((i * 2 + 1) * width * 4) + (j * 2 + 0) * 4 + 0];
			r += data[((i * 2 + 0) * width * 4) + (j * 2 + 1) * 4 + 0];
			r += data[((i * 2 + 1) * width * 4) + (j * 2 + 1) * 4 + 0];

			g += data[((i * 2 + 0) * width * 4) + (j * 2 + 0) * 4 + 1];
			g += data[((i * 2 + 1) * width * 4) + (j * 2 + 0) * 4 + 1];
			g += data[((i * 2 + 0) * width * 4) + (j * 2 + 1) * 4 + 1];
			g += data[((i * 2 + 1) * width * 4) + (j * 2 + 1) * 4 + 1];

			b += data[((i * 2 + 0) * width * 4) + (j * 2 + 0) * 4 + 2];
			b += data[((i * 2 + 1) * width * 4) + (j * 2 + 0) * 4 + 2];
			b += data[((i * 2 + 0) * width * 4) + (j * 2 + 1) * 4 + 2];
			b += data[((i * 2 + 1) * width * 4) + (j * 2 + 1) * 4 + 2];

			a += data[((i * 2 + 0) * width * 4) + (j * 2 + 0) * 4 + 3];
			a += data[((i * 2 + 1) * width * 4) + (j * 2 + 0) * 4 + 3];
			a += data[((i * 2 + 0) * width * 4) + (j * 2 + 1) * 4 + 3];
			a += data[((i * 2 + 1) * width * 4) + (j * 2 + 1) * 4 + 3];

			temp[(i * width / 2 * 4) + j * 4 + 0] = r >> 2;
			temp[(i * width / 2 * 4) + j * 4 + 1] = g >> 2;		
			temp[(i * width / 2 * 4) + j * 4 + 2] = b >> 2;
			temp[(i * width / 2 * 4) + j * 4 + 3] = a >> 2;
		}

		memcpy(data, temp, (width / 2) * (height / 2) * 4);
}

int _tmain(int argc, _TCHAR* argv[])
{
	tga_header tgah;
	enum TextureType type_param = txtUnknown;
	char input_filename[_MAX_PATH + 1];
	char output_filename[_MAX_PATH + 1];

	time_t time_start, time_stop;

	time(&time_start);

	if (argc < 4 && argc != 2) {
		usage(argv[0]);
		return 0;
	}

	memset(input_filename, 0, _MAX_PATH + 1);
	memset(output_filename, 0, _MAX_PATH + 1);

	if (argc == 2) {
		char s1[_MAX_DRIVE];
		char s2[_MAX_DIR];
		char s3[_MAX_FNAME];

		strncpy(input_filename, argv[1], _MAX_PATH);

		_splitpath(input_filename, s1, s2, s3, NULL);
		_makepath(output_filename, s1, s2, s3, "paa");
		
		type_param = txtAutoDetect;
	} else {
		strncpy(input_filename, argv[2], _MAX_PATH);
		strncpy(output_filename, argv[3], _MAX_PATH);
	}

	if (stricmp("-dxt1", argv[1]) == 0) type_param = txtDXT1;
	if (stricmp("-dxt1a", argv[1]) == 0) type_param = txtDXT1a;
	if (stricmp("-4444", argv[1]) == 0) type_param = txt4444;
	if (stricmp("-1555", argv[1]) == 0) type_param = txt1555;
	if (stricmp("-ia88", argv[1]) == 0) type_param = txtIA88;
	if (stricmp("-8888", argv[1]) == 0) type_param = txt8888;
	if (stricmp("-565", argv[1]) == 0) type_param = txt565;
	if (stricmp("-dxt3", argv[1]) == 0) type_param = txtDXT3;
	if (stricmp("-dxt5", argv[1]) == 0) type_param = txtDXT5;

	if (type_param == txtUnknown) {
		usage(argv[0]);
		return 0;
	}

	printf("Converting %s -> %s\n", input_filename, output_filename);

	FILE *in = fopen(input_filename, "rb");
	if (in == NULL) {
		perror("Cannot open input file");
		return(1);
	}

	int inb = (int)fread(&tgah, 18, 1, in);

	if (tgah.bits != 32) {
		printf("ERROR: not 32bpp TGA file\n");
		return(2);
	}
	if (tgah.colormaptype != 0) {
		printf("ERROR: TGA file contains colormap, should have none\n");
		return(3);
	}
	if (tgah.imagetype != 2 && tgah.imagetype != 10) {
		printf("ERROR: TGA imagetype not truecolor or RLE compressed truecolor, was %d\n", tgah.imagetype);
		return(4);
	}

	printf("image size = %dx%dx%d, type = %d, desc = 0x%02x\n", 
			tgah.width, tgah.height, tgah.bits, tgah.imagetype, tgah.descriptor);

	printf("top-down image: %s\n", (tgah.descriptor & 0x20 ? "no" : "yes"));

	fseek(in, tgah.identsize, SEEK_CUR);

	unsigned char *imagebuf;
	if ((imagebuf = (unsigned char *)malloc(tgah.width * tgah.height * 4)) == NULL) {
		printf("could not alloc memory\n");
		return(0);
	}
	
	// raw image data
	if (tgah.imagetype == 2) {
		fread(imagebuf, 1, tgah.width * tgah.height * 4, in);
	}
	// compressed TGA image
	if (tgah.imagetype == 10) {
		int read_pixels = 0;
		unsigned int *buf_ptr = (unsigned int *)imagebuf;

		while (read_pixels < tgah.width * tgah.height) {
			unsigned char header = 0;
			int color, length;

			fread(&header, 1, 1, in);
			length = (header & 0x7f) + 1;

			// repeat
			if (header & 0x80) {
				fread(&color, 4, 1, in);
				for (int i = 0; i < length; i++) {
					*buf_ptr++ = color;
					read_pixels++;
				}
			} else {
				fread(buf_ptr, 4 * length, 1, in);
				buf_ptr += length;
				read_pixels += length;
			}
		}
		printf("uncompressed %d bytes\n", read_pixels);
	}
	fclose(in);

	if ((tgah.descriptor & 0x20) == 0) flip_vertical(imagebuf, tgah.width, tgah.height);

	FILE *out = fopen(output_filename, "wb");
	if (out == NULL) {
			perror("Cannot create output file");
			return(5);
	}

	PAAFile testi;

	callback_paafile = &testi;
	callback_width = tgah.width;
	callback_height = tgah.height;

	calc_average_color(imagebuf, tgah.width * tgah.height, testi.avgcolor);
	testi.flag = has_transparency(imagebuf, tgah.width * tgah.height);

	// use BOX filtering with mipmaps to avoid alpha bleeding
	options.MIPFilterType = kMIPFilterBox;

	if (type_param == txtAutoDetect) type_param = autodetect_type(imagebuf, tgah.width * tgah.height);

	if (type_param == txtDXT1) {
		printf("texture type: DXT1\n");
		testi.type = 0xFF01;
		options.TextureFormat = kDXT1;
		nvDXTcompress(imagebuf, tgah.width, tgah.height, tgah.width * 4, &options, 4, (MIPcallback)mipmapCallback, NULL);
	}
	if (type_param == txtDXT1a) {
		printf("texture type: DXT1 with 1-bit alpha\n");
		testi.type = 0xFF01;
		options.TextureFormat = kDXT1a;
		nvDXTcompress(imagebuf, tgah.width, tgah.height, tgah.width * 4, &options, 4, (MIPcallback)mipmapCallback, NULL);
	}

	if (type_param == txt1555) {
		printf("texture type: 1555\n");
		testi.type = 0x1555;
		options.TextureFormat = k1555;
		nvDXTcompress(imagebuf, tgah.width, tgah.height, tgah.width * 4, &options, 4, (MIPcallback)mipmapCallback, NULL);
	}
	if (type_param == txtIA88) {
		printf("texture type: IA80\n");
		testi.type = 0x8080;
		options.TextureFormat = kA8L8;
		nvDXTcompress(imagebuf, tgah.width, tgah.height, tgah.width * 4, &options, 4, (MIPcallback)mipmapCallback, NULL);
	}

	if (type_param == txt4444) {
		printf("texture type: 4444\n");
		testi.type = 0x4444;
		options.TextureFormat = k4444;
		nvDXTcompress(imagebuf, tgah.width, tgah.height, tgah.width * 4, &options, 4, (MIPcallback)mipmapCallback, NULL);
	
		/* int width = tgah.width;
		int height = tgah.height;
		while (width > 1 && height > 1) {
			PAAMipmap *mipmap = new PAAMipmap(0x4444, width, height, (unsigned char *)imagebuf);
			testi.addMipmap(mipmap);
			printf("mipmap %d, size %08x, %dx%d\n", testi.mipmap_num, mipmap->size, mipmap->width, mipmap->height);
			make_mipmap(imagebuf, width, height);
			width >>= 1;
			height >>= 1;
		} */
	}
	if (type_param == txt8888) {
		testi.type = 0x8888;
		options.TextureFormat = k8888;
		nvDXTcompress(imagebuf, tgah.width, tgah.height, tgah.width * 4, &options, 4, (MIPcallback)mipmapCallback, NULL);
	}
	if (type_param == txt565) {
		testi.type = 0x0565;
		options.TextureFormat = k565;
		nvDXTcompress(imagebuf, tgah.width, tgah.height, tgah.width * 4, &options, 4, (MIPcallback)mipmapCallback, NULL);
	}
	if (type_param == txtDXT3) {
		testi.type = 0xFF03;
		options.TextureFormat = kDXT3;
		nvDXTcompress(imagebuf, tgah.width, tgah.height, tgah.width * 4, &options, 4, (MIPcallback)mipmapCallback, NULL);
	}
	if (type_param == txtDXT5) {
		testi.type = 0xFF05;
		options.TextureFormat = kDXT5;
		nvDXTcompress(imagebuf, tgah.width, tgah.height, tgah.width * 4, &options, 4, (MIPcallback)mipmapCallback, NULL);
	}

	printf("writing %d mipmaps\n", testi.mipmap_num);
	
	testi.writeFile(out);

	fclose(out);

	time(&time_stop);
	printf("Processing time: %d seconds\n", time_stop - time_start);

	return 0;

}

void WriteDTXnFile(DWORD count, void *buf) {
}

void __cdecl ReadDTXnFile(DWORD count, void *buf) {
}