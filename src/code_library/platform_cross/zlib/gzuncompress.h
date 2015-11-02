//gzuncompress.cpp

#include <stdio.h>
//#include <memory.h>
//#include <malloc.h>

#include "zlib.h"

#define ALLOC(size) malloc(size)
#define TRYFREE(p) {if (p) free(p);}

static int gz_magic[2] = {0x1f, 0x8b}; /* gzip magic header */

/* gzip flag byte */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define RESERVED     0xE0 /* bits 5..7: reserved */

typedef struct gz_stream {
	z_stream stream;
	int      z_err;   /* error code for last stream operation */
	int      z_eof;   /* set if end of input file */
	uLong    crc;     /* crc32 of uncompressed data */
	int      transparent; /* 1 if input file is not a .gz file */
	long     startpos; /* start of compressed data in file (header skipped) */
} gz_stream;

int    gzu_get_byte     (gz_stream *s);
void   gzu_check_header (gz_stream *s);
int    gzu_destroy      (gz_stream *s);
uLong  gzu_getLong      (gz_stream *s);

int gzuncompress(Bytef *dest, uLongf *destLen,
				 const Bytef *source, uLong sourceLen)
{
	int err;
	int level = Z_DEFAULT_COMPRESSION; /* compression level */
	int strategy = Z_DEFAULT_STRATEGY; /* compression strategy */
	gz_stream *s;
	Bytef *start;
	Byte  *next_out; /* == stream.next_out but not forced far (for MSDOS) */

	s = (gz_stream *)ALLOC(sizeof(gz_stream));
	if (!s) return Z_NULL;

	s->stream.zalloc = (alloc_func)0;
	s->stream.zfree = (free_func)0;
	s->stream.opaque = (voidpf)0;
	s->stream.next_in = (Bytef*)source;
	s->stream.next_out = dest;
	s->stream.avail_in = sourceLen;
	s->stream.avail_out = (uInt)*destLen;
	s->z_err = Z_OK;
	s->z_eof = 0;
	s->crc = crc32(0L, Z_NULL, 0);
	s->transparent = 0;

	err = inflateInit2(&(s->stream), -MAX_WBITS);
	if (err != Z_OK) {
		return gzu_destroy(s);
	}

	gzu_check_header(s); /* skip the .gz header */
	s->startpos = (sourceLen - s->stream.avail_in);

	start = (Bytef*)dest; /* starting point for crc computation */

	if (s->z_err == Z_DATA_ERROR || s->z_err == Z_ERRNO) return gzu_destroy (s);
	if (s->z_err == Z_STREAM_END)
	{
		* ( (int *)destLen ) = 0;
		//(int)(*destLen) = 0;
		return gzu_destroy (s);
	}

	next_out = (Byte*)dest;
	s->stream.next_out = (Bytef*)dest;
	s->stream.avail_out = (uInt)*destLen;

	while (s->stream.avail_out != 0) {

		if (s->transparent) {
			s->z_err = Z_DATA_ERROR;
			return gzu_destroy (s);
		}
		if (s->stream.avail_in == 0 && !s->z_eof) {
			s->z_eof = 1;
		}
		s->z_err = inflate(&(s->stream), Z_NO_FLUSH);

		if (s->z_err == Z_STREAM_END) {
			/* Check CRC and original size */
			s->crc = crc32(s->crc, start, (uInt)(s->stream.next_out - start));
			start = s->stream.next_out;

			if (gzu_getLong(s) != s->crc) {
				s->z_err = Z_DATA_ERROR;
			} else {
				(void)gzu_getLong(s);
				/* The uncompressed length returned by above gzu_getLong() may
				* be different from s->stream.total_out) in case of
				* concatenated .gz files. Check for such files:
				*/
				gzu_check_header(s);
				if (s->z_err == Z_OK) {
					uLong total_in = s->stream.total_in;
					uLong total_out = s->stream.total_out;

					inflateReset(&(s->stream));
					s->stream.total_in = total_in;
					s->stream.total_out = total_out;
					s->crc = crc32(0L, Z_NULL, 0);
				}
			}
		}
		if (s->z_err != Z_OK || s->z_eof) break;
	}
	s->crc = crc32(s->crc, start, (uInt)(s->stream.next_out - start));

	* ( (int *)destLen ) = (int)(* ( (int *)destLen ) - s->stream.avail_out);

	return gzu_destroy(s);
}

int gzu_get_byte(gz_stream *s)
{
	if (s->z_eof) return EOF;
	if (s->stream.avail_in == 0) {
		s->z_eof = 1;
		return EOF;
	}
	s->stream.avail_in--;
	return *(s->stream.next_in)++;
}

void gzu_check_header(gz_stream *s)
{
	int method; /* method byte */
	int flags;  /* flags byte */
	uInt len;
	int c;

	/* Check the gzip magic header */
	for (len = 0; len < 2; len++) {
		c = gzu_get_byte(s);
		if (c != gz_magic[len]) {
			if (len != 0) s->stream.avail_in++, s->stream.next_in--;
			if (c != EOF) {
				s->stream.avail_in++, s->stream.next_in--;
				s->transparent = 1;
			}
			s->z_err = s->stream.avail_in != 0 ? Z_OK : Z_STREAM_END;
			return;
		}
	}
	method = gzu_get_byte(s);
	flags = gzu_get_byte(s);
	if (method != Z_DEFLATED || (flags & RESERVED) != 0) {
		s->z_err = Z_DATA_ERROR;
		return;
	}

	/* Discard time, xflags and OS code: */
	for (len = 0; len < 6; len++) (void)gzu_get_byte(s);

	if ((flags & EXTRA_FIELD) != 0) { /* skip the extra field */
		len  =  (uInt)gzu_get_byte(s);
		len += ((uInt)gzu_get_byte(s))<<8;
		/* len is garbage if EOF but the loop below will quit anyway */
		while (len-- != 0 && gzu_get_byte(s) != EOF) ;
	}
	if ((flags & ORIG_NAME) != 0) { /* skip the original file name */
		while ((c = gzu_get_byte(s)) != 0 && c != EOF) ;
	}
	if ((flags & COMMENT) != 0) {   /* skip the .gz file comment */
		while ((c = gzu_get_byte(s)) != 0 && c != EOF) ;
	}
	if ((flags & HEAD_CRC) != 0) {  /* skip the header crc */
		for (len = 0; len < 2; len++) (void)gzu_get_byte(s);
	}
	s->z_err = s->z_eof ? Z_DATA_ERROR : Z_OK;
}

int gzu_destroy (gz_stream *s)
{
	int err = Z_OK;

	if (!s) return Z_STREAM_ERROR;

	if (s->stream.state != NULL) {
		err = inflateEnd(&(s->stream));
	}

	if (s->z_err < 0) err = s->z_err;

	TRYFREE(s);
	return err;
}

uLong gzu_getLong (gz_stream *s)
{
	uLong x = (uLong)gzu_get_byte(s);
	int c;

	x += ((uLong)gzu_get_byte(s))<<8;
	x += ((uLong)gzu_get_byte(s))<<16;
	c = gzu_get_byte(s);
	if (c == EOF) s->z_err = Z_DATA_ERROR;
	x += ((uLong)c)<<24;
	return x;
}
