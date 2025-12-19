#include "stdafx.h"
#include "tndefs.h"
#pragma hdrstop

#include "utf.h"

// c : UTF-32
int UTF8Len(int c)
{
	if ( c < 0x80 ){
		// 1 octet
		return 1;
	} else {
		if ( c < 0x800 ){
			return 2;
		} else {
			if ( c < 0x10000 ){
				return 3;
			} else {
				return 4;
			}
		}
	}
}
// dstlen : sizeof(*dst)
int UTF16toUTF8( const utf16_t *src, int srclen, char *dst, int dstlen )
{
	const utf16_t *end;
	if ( srclen == -1 ){
		end = (utf16_t*)-1;
	} else {
		end = src + srclen;
	}
	char *dst_end = (dstlen>=0?dst+dstlen-5:(char*)(-1));
	int size = 0;
	while (src<end){
		long c = *src++;
		if (c == 0)
			break;	// end of string
		if ( c >= 0xD800 && c <= 0xDBFF ){
			utf16_t wc = *src++;
			if ( wc >= 0xDC00 && wc <= 0xDFFF ){
				c = (((c-0xD800)<<10) | (wc - 0xDC00)) + 0x10000;
			} else {
				// illegal code
				continue;	// skip
			}
		}

		int w;

		if (dst>=dst_end){
			w = UTF8Len(c);
			if (dst+w>=dst_end+4){
				return -1;	// not enough dst area
			}
		}

		// change UTF-32 to UTF-8
		if ( c < 0x80 ){
			// 1 octet
			w = 1;
			*dst++ = (uint8_t)c;
		} else {
			if ( c < 0x800 ){
				w = 2;
				*dst++ = (uint8_t)((c>>6)+0xC0);
			} else {
				if ( c < 0x10000 ){
					w = 3;
					*dst++ = (uint8_t)((c>>12)+0xE0);
				} else {
					w = 4;
					*dst++ = (uint8_t)((c>>18)+0xF0);
					*dst++ = (uint8_t)(((c>>12)&0x3F)+0x80);
				}
				*dst++ = (uint8_t)(((c>>6)&0x3F)+0x80);
			}
			*dst++ = (uint8_t)((c&0x3F)+0x80);
		}
		size += w;
	}
	*dst = '\0';
	return size;
}
int UTF8toUTF16( const char *src, int srclen, utf16_t *dst, int dstlen )
{
	const char *end;
	if ( srclen == -1 ){
		end = (char*)-1;
	} else {
		end = src + srclen;
	}
	utf16_t *dst_end = (dstlen>=0?dst+dstlen-2:(utf16_t*)(-1));
	int size = 0;
	while (src<end){
		long c = *(uint8_t*)src++;
		if (c == 0)
			break;	// end of string
		if (c & 0x80){
			unsigned long diff;
			diff = (0xC0<<6);
			c = (c<<6) | (*(uint8_t*)src++ & 0x3F);
			if ( c >= (0xE0<<6) ){
				diff = (0xE0<<12);
				c = (c<<6) | (*(uint8_t*)src++ & 0x3F);
				if ( c >= (0xF0<<12) ){
					diff = (0xF0<<18);
					c = (c<<6) | (*(uint8_t*)src++ & 0x3F);
				}
			}
			c -= diff;
		}
		// change UTF-32 to UTF-16 //
		if (c <= 0xFFFF){
			if (dst>=dst_end){
				return -1;	// not enough area
			}
			*dst++ = (utf16_t)c;
			size++;
		} else {
			// surrogate
			if (dst>=dst_end-1){
				return -1;	// not enough area
			}
			*dst++ = (utf16_t)(((c - 0x10000)>>10) + 0xD800);
			*dst++ = (utf16_t)((c & 0x3FF) + 0xDC00);
			size+=2;
		}
	}
	*dst = '\0';
	return size;
}
unsigned long UTF8toUTF16( const char *src, const char **next )
{
	unsigned long c = *(uint8_t*)src;
	if (c == 0){
		*next = src;
		return 0;	// end of string
	}
	src++;
	if (c & 0x80){
		unsigned long diff;
		diff = (0xC0<<6);
		uint8_t cc = *(uint8_t*)src;
		if (!cc){
			*next = src;
			return 0;	// illegal sequence
		}
		src++;
		c = (c<<6) | (cc & 0x3F);
		if ( c >= (0xE0<<6) ){
			diff = (0xE0<<12);
			cc = *(uint8_t*)src;
			if (!cc){
				*next = src;
				return 0;	// illegal sequence
			}
			src++;
			c = (c<<6) | (cc & 0x3F);
			if ( c >= (0xF0<<12) ){
				diff = (0xF0<<18);
				cc = *(uint8_t*)src;
				if (!cc){
					*next = src;
					return 0;	// illegal sequence
				}
				src++;
				c = (c<<6) | (cc & 0x3F);
			}
		}
		c -= diff;
	}
	// change UTF-32 to UTF-16 //
	if (c >= 0x10000){
		// surrogate (not yet debugged)
		c = ((((c - 0x10000)>>10) + 0xD800)<<16) | (utf16_t)((c & 0x3FF) + 0xDC00);
	}
	*next = src;
	return c;
}

int UTF32toUTF8( const utf32_t *src, int srclen, char *dst, int dstlen )
{
	const utf32_t *end;
	if ( srclen == -1 ){
		end = (utf32_t*)-1;
	} else {
		end = src + srclen;
	}
	char *dst_end = (dstlen>=0?dst+dstlen-5:(char*)(-1));
	int size = 0;
	while (src<end){
		utf32_t c = *src++;
		if (c == 0)
			break;	// end of string

		int w;

		if (dst>=dst_end){
			w = UTF8Len(c);
			if (dst+w>=dst_end+4){
				return -1;	// not enough dst area
			}
		}

		// change UTF-32 to UTF-8
		if ( c < 0x80 ){
			// 1 octet
			w = 1;
			*dst++ = (uint8_t)c;
		} else {
			if ( c < 0x800 ){
				w = 2;
				*dst++ = (uint8_t)((c>>6)+0xC0);
			} else {
				if ( c < 0x10000 ){
					w = 3;
					*dst++ = (uint8_t)((c>>12)+0xE0);
				} else {
					w = 4;
					*dst++ = (uint8_t)((c>>18)+0xF0);
					*dst++ = (uint8_t)(((c>>12)&0x3F)+0x80);
				}
				*dst++ = (uint8_t)(((c>>6)&0x3F)+0x80);
			}
			*dst++ = (uint8_t)((c&0x3F)+0x80);
		}
		size += w;
	}
	*dst = '\0';
	return size;
}

int UTF8toUTF32( const char *src, int srclen, utf32_t *dst, int dstlen )
{
	const char *end;
	if ( srclen == -1 ){
		end = (char*)-1;
	} else {
		end = src + srclen;
	}
	utf32_t *dst_end = (dstlen>=0?dst+dstlen-2:(utf32_t*)(-1));
	int size = 0;
	while (src<end){
		utf32_t c = *(uint8_t*)src++;
		if (c == 0)
			break;	// end of string
		if (c & 0x80){
			unsigned long diff;
			diff = (0xC0<<6);
			c = (c<<6) | (*(uint8_t*)src++ & 0x3F);
			if ( c >= (0xE0<<6) ){
				diff = (0xE0<<12);
				c = (c<<6) | (*(uint8_t*)src++ & 0x3F);
				if ( c >= (0xF0<<12) ){
					diff = (0xF0<<18);
					c = (c<<6) | (*(uint8_t*)src++ & 0x3F);
				}
			}
			c -= diff;
		}
		if (dst>=dst_end){
			return -1;	// not enough area
		}
		*dst++ = (utf32_t)c;
		size++;
	}
	*dst = '\0';
	return size;
}

utf32_t UTF8toUTF32( const char *src, const char **next )
{
	utf32_t c = *(uint8_t*)src;
	if (c == 0){
		*next = src;
		return 0;	// end of string
	}
	src++;
	if (c & 0x80){
		utf32_t diff;
		diff = (0xC0<<6);
		uint8_t cc = *(uint8_t*)src;
		if (!cc){
			*next = src;
			return 0;	// illegal sequence
		}
		src++;
		c = (c<<6) | (cc & 0x3F);
		if ( c >= (0xE0<<6) ){
			diff = (0xE0<<12);
			cc = *(uint8_t*)src;
			if (!cc){
				*next = src;
				return 0;	// illegal sequence
			}
			src++;
			c = (c<<6) | (cc & 0x3F);
			if ( c >= (0xF0<<12) ){
				diff = (0xF0<<18);
				cc = *(uint8_t*)src;
				if (!cc){
					*next = src;
					return 0;	// illegal sequence
				}
				src++;
				c = (c<<6) | (cc & 0x3F);
			}
		}
		c -= diff;
	}
	*next = src;
	return c;
}

const char *utf8CharNext(const char *s)
{
	unsigned long c = *(uint8_t*)s;
	if (*s == 0){
		return s;	// end of string
	}
	s++;
	if (c & 0x80){
		uint8_t cc = *(uint8_t*)s;
		if (!cc){
			return s;	// illegal sequence
		}
		s++;
		c = (c<<6) | (cc & 0x3F);
		if ( c >= (0xE0<<6) ){
			cc = *(uint8_t*)s;
			if (!cc){
				return s;	// illegal sequence
			}
			s++;
			c = (c<<6) | (cc & 0x3F);
			if ( c >= (0xF0<<12) ){
				cc = *(uint8_t*)s;
				if (!cc){
					return s;	// illegal sequence
				}
				s++;
//				c = (c<<6) | (cc & 0x3F);
			}
		}
	}
	return s;
}

int UTF32toUTF16( const utf32_t *src, int srclen, utf16_t *dst, int dstlen )
{
	const utf32_t *end;
	if ( srclen == -1 ){
		end = (utf32_t*)-1;
	} else {
		end = src + srclen;
	}
	utf16_t *dst_end = (dstlen>=0?dst+dstlen-5:(utf16_t*)(-1));
	int size = 0;
	while (src<end){
		utf32_t c = *src++;
		if (c == 0)
			break;	// end of string

		// change UTF-32 to UTF-16 //
		if (c <= 0xFFFF){
			if (dst>=dst_end){
				return -1;	// not enough area
			}
			*dst++ = (utf16_t)c;
			size++;
		} else {
			// surrogate
			if (dst>=dst_end-1){
				return -1;	// not enough area
			}
			*dst++ = (utf16_t)(((c - 0x10000)>>10) + 0xD800);
			*dst++ = (utf16_t)((c & 0x3FF) + 0xDC00);
			size+=2;
		}
	}
	*dst = '\0';
	return size;
}

int UTF16toUTF32( const utf16_t *src, int srclen, utf32_t *dst, int dstlen )
{
	const utf16_t *end;
	if ( srclen == -1 ){
		end = (utf16_t*)-1;
	} else {
		end = src + srclen;
	}
	utf32_t *dst_end = (dstlen>=0?dst+dstlen:(utf32_t*)(-1));
	int size = 0;
	while (src<end){
		utf16_t c = *src++;
		if (c == 0)
			break;	// end of string
		if ( c >= 0xD800 && c <= 0xDBFF ){
			utf16_t wc = *src++;
			if ( wc >= 0xDC00 && wc <= 0xDFFF ){
				c = (((c-0xD800)<<10) | (wc - 0xDC00)) + 0x10000;
			} else {
				// illegal code
				continue;	// skip
			}
		}

		if (dst>=dst_end-1){
			return -1;	// not enough area
		}
		*dst++ = c;
		size++;
	}
	*dst = '\0';
	return size;
}
