/*
******************************************************************************
*
*   Copyright (C) 2002, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*	Modified and Optimized for PDIC by TaN 2002.6.9
*
******************************************************************************
*   file name:  bocu1.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2002jan24
*   created by: Markus W. Scherer
*
*   This is a sample implementation of encoder and decoder functions for BOCU-1,
*   a MIME-compatible Binary Ordered Compression for Unicode.
*/

#include <stdio.h>
#include <string.h>

#ifndef __ANDROID__
typedef long int32_t;
#endif
typedef unsigned char byte;
#define	U_INLINE	inline
#define	U_CFUNC



/*
 * Standard ICU header.
 * - Includes inttypes.h or defines its types.
 * - Defines UChar for UTF-16 as an unsigned 16-bit type (wchar_t or uint16_t).
 * - Defines UTF* macros to handle reading and writing
 *   of in-process UTF-8/16 strings.
 */
//#include "unicode/utypes.h"

#include "bocu1.h"

static unsigned long NoCodeTranslate(unsigned long code)
	{ return code; }

/*
 * Byte value map for control codes,
 * from external byte values 0x00..0x20
 * to trail byte values 0..19 (0..0x13) as used in the difference calculation.
 * External byte values that are illegal as trail bytes are mapped to -1.
 */
static char
bocu1ByteToTrail[BOCU1_MIN]={
/*  0     1     2     3     4     5     6     7    */
	-1,   0x00, 0x01, 0x02, 0x03, 0x04, 0x05, -1,

/*  8     9     a     b     c     d     e     f    */
	-1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,

/*  10    11    12    13    14    15    16    17   */
	0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,

/*  18    19    1a    1b    1c    1d    1e    1f   */
	0x0e, 0x0f, -1,   -1,   0x10, 0x11, 0x12, 0x13,

/*  20   */
    -1
};

/*
 * Byte value map for control codes,
 * from trail byte values 0..19 (0..0x13) as used in the difference calculation
 * to external byte values 0x00..0x20.
 */
static char
bocu1TrailToByte[BOCU1_TRAIL_CONTROLS_COUNT]={
/*  0     1     2     3     4     5     6     7    */
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x10, 0x11,

/*  8     9     a     b     c     d     e     f    */
	0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,

/*  10    11    12    13   */
	0x1c, 0x1d, 0x1e, 0x1f
};


/* BOCU-1 implementation functions ------------------------------------------ */

/**
 * Compute the next "previous" value for differencing
 * from the current code point.
 *
 * @param c current code point, 0..0x10ffff
 * @return "previous code point" state value
 */
U_INLINE int32_t
bocu1Prev(int32_t c) {
	/* compute new prev */
	if(0x3040<=c && c<=0x309f) {
		/* Hiragana is not 128-aligned */
		return 0x3070;
	} else if(0x4e00<=c && c<=0x9fa5) {
		/* CJK Unihan */
		return 0x4e00-BOCU1_REACH_NEG_2;
	} else if(0xac00<=c && c<=0xd7a3) {
		/* Korean Hangul */
		return (0xd7a3+0xac00)/2;
	} else {
		/* mostly small scripts */
		return (c&~0x7f)+BOCU1_ASCII_PREV;
	}
}

// length is in charactor unit
// return : next destination
// destination buffer should be the size of 120% of length
//  i.e. size >= length*2*1.2(byte)
unsigned char *DEF_NAME(bocu1Encode)( const bocu_t *src, const bocu_t *end, unsigned char*dst, FNPreCodeTranslate pretrans )
{
	int32_t pPrev;
	int32_t prev;
	if (!pretrans)
		pretrans = NoCodeTranslate;

	pPrev = BOCU1_ASCII_PREV;
	while (src<end){
		int32_t c = pretrans(*src++);
		if (c == 0)
			break;	// end of string
#ifdef BOCU_UTF16
		if ( c >= 0xD800 && c <= 0xDBFF ){
			wchar_t wc = *src++;
			if ( wc >= 0xDC00 && wc <= 0xDFFF ){
				c = (((c-0xD800)<<10) | (wc - 0xDC00)) + 0x10000;
			} else {
				// illegal code
				if (wc==0)
					//src--;	// illegal end of string
					break;		// DROP the illegal end of string.
				else
					continue;	// skip
			}
		}
#endif	// BOCU_UTF16
#ifdef BOCU_UTF8	//TODO: code妥当性のチェックはしていない
		if (c & 0x80){
			unsigned long diff;
			diff = (0xC2<<6);
			c = (c<<6) | (*src++ & 0x3F);
			if ( c >= (0xE0<<6) ){
				diff = (0xE0<<12);
				c = (c<<6) | (*src++ & 0x3F);
				if ( c >= (0xF0<<12) ){
					diff = (0xF0<<18);
					c = (c<<6) | (*src++ & 0x3F);
				}
			}
			c -= diff;
		}
#endif

		prev = pPrev;

		if (c<=0x20){
			if (c!=0x20){
				pPrev=BOCU1_ASCII_PREV;
			}
			*dst++ = (unsigned char)c;
			continue;
		}
		pPrev = bocu1Prev(c);
		c -= prev;
		{
			int32_t m, lead, count;

			if (c>=BOCU1_REACH_NEG_1) {
				/* mostly positive differences, and single-byte negative ones */
				if (c<=BOCU1_REACH_POS_1) {
					/* single byte */
					*dst++ = (unsigned char)(BOCU1_MIDDLE+c);
					continue;
				} else
				if (c<=BOCU1_REACH_POS_2) {
					/* two bytes */
					c-=BOCU1_REACH_POS_1+1;
					lead=BOCU1_START_POS_2;
					count=1;
				} else
				if(c<=BOCU1_REACH_POS_3) {
					/* three bytes */
					c-=BOCU1_REACH_POS_2+1;
					lead=BOCU1_START_POS_3;
					count=2;
				} else {
					/* four bytes */
					c-=BOCU1_REACH_POS_3+1;
					lead=BOCU1_START_POS_4;
					count=3;
				}
			} else {
				/* two- and four-byte negative differences */
				if(c>=BOCU1_REACH_NEG_2) {
					/* two bytes */
					c-=BOCU1_REACH_NEG_1;
					lead=BOCU1_START_NEG_2;
					count=1;
				} else if(c>=BOCU1_REACH_NEG_3) {
					/* three bytes */
					c-=BOCU1_REACH_NEG_2;
					lead=BOCU1_START_NEG_3;
					count=2;
				} else {
					/* four bytes */
					c-=BOCU1_REACH_NEG_3;
					lead=BOCU1_START_NEG_4;
					count=3;
				}
			}

			/* calculate trail bytes like digits in itoa() */
			unsigned char *dp = dst;
			dst += count+1;
			do {
				NEGDIVMOD(c, BOCU1_TRAIL_COUNT, m);
				dp[count] = (unsigned char)BOCU1_TRAIL_TO_BYTE(m);
			} while(--count>0);

			/* add lead byte */
			dp[0] = (unsigned char)(lead+c);
		}
	}
//	*dst++ = '\0';	// end mark
	return dst;
}
// length of encoded strings (I would like to find the optimized way..)
int DEF_NAME(bocu1EncodeLength)(const bocu_t *src, const bocu_t *end, unsigned char*dst )
{
	int len = bocu_len(src);
	if (end == (bocu_t*)-1){
		end = src + len;
	}
	unsigned char *temp = NULL;
	if (!dst){
		temp = new unsigned char[len*2+1];
		if (!temp)
			return 0;
		dst = temp;
	}
	unsigned char *cend = DEF_NAME(bocu1Encode)(src, end, dst);
	int clen = (int)(cend-dst);
	if (temp)
		delete[] temp;
	return clen;	// compressed length
}
// outmaxlength : in charactor in UTF-16
bocu_t *DEF_NAME(bocu1Decode)( const unsigned char **_src, const unsigned char *endp, int outmaxlength, bocu_t *outbuffer, int *outlen, FNPreCodeTranslate pretrans )
{
	int32_t c;
	Bocu1Rx Rx;
	const unsigned char *src = *_src;
	if (!outbuffer)
		outbuffer = new bocu_t[ outmaxlength + 1 ];
	bocu_t *dst = outbuffer;
	bocu_t *dstend = outbuffer + outmaxlength;
	if (!pretrans)
		pretrans = NoCodeTranslate;

	Rx.prev = BOCU1_ASCII_PREV;
	Rx.count = 0;
	while (src<endp){
		unsigned char b = *src++;
		int32_t prev = Rx.prev;

		if(Rx.count==0) {
			/* byte in lead position */
			if(b<=0x20){
				/*
				 * Direct-encoded C0 control code or space.
				 * Reset prev for C0 control codes but not for space.
				 */
				if(b!=0x20){
					if (b==0x00)
						break;	// EOC
					Rx.prev=BOCU1_ASCII_PREV;
				}
				*dst++ = b;
				if (dst>=dstend)
					break;
				continue;
			}

			/*
			 * b is a difference lead byte.
			 *
			 * Return a code point directly from a single-byte difference.
			 *
			 * For multi-byte difference lead bytes, set the decoder state
			 * with the partial difference value from the lead byte and
			 * with the number of trail bytes.
			 *
			 * For four-byte differences, the signedness also affects the
			 * first trail byte, which has special handling farther below.
			 */
			if(b>=BOCU1_START_NEG_2 && b<BOCU1_START_POS_2){
				/* single-byte difference */
				c=prev+((int32_t)b-BOCU1_MIDDLE);
				Rx.prev=bocu1Prev(c);
				*dst++ = (bocu_t)pretrans(c);
				if (dst>=dstend)
					break;
				continue;
			} else
			if(b==BOCU1_RESET){
				/* only reset the state, no code point */
				Rx.prev=BOCU1_ASCII_PREV;
				continue;	// nothing done
			} else {
				int32_t count;

				if(b>=BOCU1_START_NEG_2){
					/* positive difference */
					if(b<BOCU1_START_POS_3) {
						/* two bytes */
						c=((int32_t)b-BOCU1_START_POS_2)*BOCU1_TRAIL_COUNT+BOCU1_REACH_POS_1+1;
						count=1;
					} else if(b<BOCU1_START_POS_4) {
						/* three bytes */
						c=((int32_t)b-BOCU1_START_POS_3)*BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT+BOCU1_REACH_POS_2+1;
						count=2;
					} else {
						/* four bytes */
						c=BOCU1_REACH_POS_3+1;
						count=3;
					}
				} else {
					/* negative difference */
					if(b>=BOCU1_START_NEG_3) {
						/* two bytes */
						c=((int32_t)b-BOCU1_START_NEG_2)*BOCU1_TRAIL_COUNT+BOCU1_REACH_NEG_1;
						count=1;
					} else if(b>BOCU1_MIN) {
						/* three bytes */
						c=((int32_t)b-BOCU1_START_NEG_3)*BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT+BOCU1_REACH_NEG_2;
						count=2;
					} else {
						/* four bytes */
						c=-BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT+BOCU1_REACH_NEG_3;
						count=3;
					}
				}

				/* set the state for decoding the trail byte(s) */
				Rx.diff=c;
				Rx.count=count;
				continue;	// nothing to store
			}
		} else {
			/* trail byte in any position */
			int32_t t, count;

			if(b<=0x20){
				/* skip some C0 controls and make the trail byte range contiguous */
				t=bocu1ByteToTrail[b];
				if(t<0){
					/* illegal trail byte value */
					Rx.prev=BOCU1_ASCII_PREV;
					Rx.count=0;
					break;	// error
				}
#if BOCU1_MAX_TRAIL<0xff
			} else
			if(b>BOCU1_MAX_TRAIL) {
				break;	// error
#endif
			} else {
				t=(int32_t)b-BOCU1_TRAIL_BYTE_OFFSET;
			}

			/* add trail byte into difference and decrement count */
			c = Rx.diff;
			count = Rx.count;

			if(count==1){
				/* final trail byte, deliver a code point */
				c = Rx.prev+c+t;
				if ( 0<=c && c<=0x10ffff ){
					/* valid code point result */
					Rx.prev=bocu1Prev(c);
					Rx.count=0;
					c = pretrans(c);
#ifdef BOCU_UTF16
					// change UTF-32 to UTF-16 //
					if (c <= 0xFFFF){
						*dst++ = (wchar_t)c;
					} else {
						// surrogate
						*dst++ = (wchar_t)(((c - 0x10000)>>10) + 0xD800);
						*dst++ = (wchar_t)((c & 0x3FF) + 0xDC00);
					}
#endif	// BOCU_UTF16
#ifdef BOCU_UTF8
					// change UTF-32 to UTF-8
					if ( c < 0x80 ){
						// 1 octet
						*dst++ = (byte)c;
					} else {
						if ( c < 0x800 ){
							*dst++ = (byte)((c>>6)+0xC0);
						} else {
							if ( c < 0x10000 ){
								*dst++ = (byte)((c>>12)+0xE0);
							} else {
								*dst++ = (byte)((c>>18)+0xF0);
								*dst++ = (byte)(((c>>12)&0x3F)+0x80);
							}
							*dst++ = (byte)(((c>>6)&0x3F)+0x80);
						}
						*dst++ = (byte)((c&0x3F)+0x80);
					}
#endif	// BOCU_UTF8
					if (dst>=dstend)
						break;
					continue;
				} else {
					/* illegal code point result */
					Rx.prev=BOCU1_ASCII_PREV;
					Rx.count=0;
					break;	// error
				}
			}

			/* intermediate trail byte */
			if(count==2) {
				Rx.diff=c+t*BOCU1_TRAIL_COUNT;
			} else /* count==3 */ {
				Rx.diff=c+t*BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT;
			}
			Rx.count=count-1;
			continue;	// nothing to store
		}
	}
	*dst = '\0';
	if (outlen)
		*outlen = (int)(dst-outbuffer);
	*_src = src;
	return outbuffer;
}

