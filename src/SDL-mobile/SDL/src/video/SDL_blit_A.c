/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2012 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "SDL_config.h"

#include "SDL_video.h"
#include "SDL_blit.h"

/* Functions to perform alpha blended blitting */

/* N->1 blending with per-surface alpha */
static void
BlitNto1SurfaceAlpha(SDL_BlitInfo * info)
{
    int width = info->dst_w;
    int height = info->dst_h;
    Uint8 *src = info->src;
    int srcskip = info->src_skip;
    Uint8 *dst = info->dst;
    int dstskip = info->dst_skip;
    Uint8 *palmap = info->table;
    SDL_PixelFormat *srcfmt = info->src_fmt;
    SDL_PixelFormat *dstfmt = info->dst_fmt;
    int srcbpp = srcfmt->BytesPerPixel;

    const unsigned A = info->a;

    while (height--) {
	    /* *INDENT-OFF* */
	    DUFFS_LOOP4(
	    {
		Uint32 Pixel;
		unsigned sR;
		unsigned sG;
		unsigned sB;
		unsigned dR;
		unsigned dG;
		unsigned dB;
		DISEMBLE_RGB(src, srcbpp, srcfmt, Pixel, sR, sG, sB);
		dR = dstfmt->palette->colors[*dst].r;
		dG = dstfmt->palette->colors[*dst].g;
		dB = dstfmt->palette->colors[*dst].b;
		ALPHA_BLEND(sR, sG, sB, A, dR, dG, dB);
		dR &= 0xff;
		dG &= 0xff;
		dB &= 0xff;
		/* Pack RGB into 8bit pixel */
		if ( palmap == NULL ) {
		    *dst =((dR>>5)<<(3+2))|
			  ((dG>>5)<<(2))|
			  ((dB>>6)<<(0));
		} else {
		    *dst = palmap[((dR>>5)<<(3+2))|
				  ((dG>>5)<<(2))  |
				  ((dB>>6)<<(0))];
		}
		dst++;
		src += srcbpp;
	    },
	    width);
	    /* *INDENT-ON* */
        src += srcskip;
        dst += dstskip;
    }
}

/* N->1 blending with pixel alpha */
static void
BlitNto1PixelAlpha(SDL_BlitInfo * info)
{
    int width = info->dst_w;
    int height = info->dst_h;
    Uint8 *src = info->src;
    int srcskip = info->src_skip;
    Uint8 *dst = info->dst;
    int dstskip = info->dst_skip;
    Uint8 *palmap = info->table;
    SDL_PixelFormat *srcfmt = info->src_fmt;
    SDL_PixelFormat *dstfmt = info->dst_fmt;
    int srcbpp = srcfmt->BytesPerPixel;

    while (height--) {
	    /* *INDENT-OFF* */
	    DUFFS_LOOP4(
	    {
		Uint32 Pixel;
		unsigned sR;
		unsigned sG;
		unsigned sB;
		unsigned sA;
		unsigned dR;
		unsigned dG;
		unsigned dB;
		DISEMBLE_RGBA(src,srcbpp,srcfmt,Pixel,sR,sG,sB,sA);
		dR = dstfmt->palette->colors[*dst].r;
		dG = dstfmt->palette->colors[*dst].g;
		dB = dstfmt->palette->colors[*dst].b;
		ALPHA_BLEND(sR, sG, sB, sA, dR, dG, dB);
		dR &= 0xff;
		dG &= 0xff;
		dB &= 0xff;
		/* Pack RGB into 8bit pixel */
		if ( palmap == NULL ) {
		    *dst =((dR>>5)<<(3+2))|
			  ((dG>>5)<<(2))|
			  ((dB>>6)<<(0));
		} else {
		    *dst = palmap[((dR>>5)<<(3+2))|
				  ((dG>>5)<<(2))  |
				  ((dB>>6)<<(0))  ];
		}
		dst++;
		src += srcbpp;
	    },
	    width);
	    /* *INDENT-ON* */
        src += srcskip;
        dst += dstskip;
    }
}

/* colorkeyed N->1 blending with per-surface alpha */
static void
BlitNto1SurfaceAlphaKey(SDL_BlitInfo * info)
{
    int width = info->dst_w;
    int height = info->dst_h;
    Uint8 *src = info->src;
    int srcskip = info->src_skip;
    Uint8 *dst = info->dst;
    int dstskip = info->dst_skip;
    Uint8 *palmap = info->table;
    SDL_PixelFormat *srcfmt = info->src_fmt;
    SDL_PixelFormat *dstfmt = info->dst_fmt;
    int srcbpp = srcfmt->BytesPerPixel;
    Uint32 ckey = info->colorkey;

    const int A = info->a;

    while (height--) {
	    /* *INDENT-OFF* */
	    DUFFS_LOOP(
	    {
		Uint32 Pixel;
		unsigned sR;
		unsigned sG;
		unsigned sB;
		unsigned dR;
		unsigned dG;
		unsigned dB;
		DISEMBLE_RGB(src, srcbpp, srcfmt, Pixel, sR, sG, sB);
		if ( Pixel != ckey ) {
		    dR = dstfmt->palette->colors[*dst].r;
		    dG = dstfmt->palette->colors[*dst].g;
		    dB = dstfmt->palette->colors[*dst].b;
		    ALPHA_BLEND(sR, sG, sB, A, dR, dG, dB);
		    dR &= 0xff;
		    dG &= 0xff;
		    dB &= 0xff;
		    /* Pack RGB into 8bit pixel */
		    if ( palmap == NULL ) {
			*dst =((dR>>5)<<(3+2))|
			      ((dG>>5)<<(2)) |
			      ((dB>>6)<<(0));
		    } else {
			*dst = palmap[((dR>>5)<<(3+2))|
				      ((dG>>5)<<(2))  |
				      ((dB>>6)<<(0))  ];
		    }
		}
		dst++;
		src += srcbpp;
	    },
	    width);
	    /* *INDENT-ON* */
        src += srcskip;
        dst += dstskip;
    }
}

#ifdef __MMX__

/* fast RGB888->(A)RGB888 blending with surface alpha=128 special case */
static void
BlitRGBtoRGBSurfaceAlpha128MMX(SDL_BlitInfo * info)
{
    int width = info->dst_w;
    int height = info->dst_h;
    Uint32 *srcp = (Uint32 *) info->src;
    int srcskip = info->src_skip >> 2;
    Uint32 *dstp = (Uint32 *) info->dst;
    int dstskip = info->dst_skip >> 2;
    Uint32 dalpha = info->dst_fmt->Amask;

    __m64 src1, src2, dst1, dst2, lmask, hmask, dsta;

    hmask = _mm_set_pi32(0x00fefefe, 0x00fefefe);       /* alpha128 mask -> hmask */
    lmask = _mm_set_pi32(0x00010101, 0x00010101);       /* !alpha128 mask -> lmask */
    dsta = _mm_set_pi32(dalpha, dalpha);        /* dst alpha mask -> dsta */

    while (height--) {
        int n = width;
        if (n & 1) {
            Uint32 s = *srcp++;
            Uint32 d = *dstp;
            *dstp++ = ((((s & 0x00fefefe) + (d & 0x00fefefe)) >> 1)
                       + (s & d & 0x00010101)) | dalpha;
            n--;
        }

        for (n >>= 1; n > 0; --n) {
            dst1 = *(__m64 *) dstp;     /* 2 x dst -> dst1(ARGBARGB) */
            dst2 = dst1;        /* 2 x dst -> dst2(ARGBARGB) */

            src1 = *(__m64 *) srcp;     /* 2 x src -> src1(ARGBARGB) */
            src2 = src1;        /* 2 x src -> src2(ARGBARGB) */

            dst2 = _mm_and_si64(dst2, hmask);   /* dst & mask -> dst2 */
            src2 = _mm_and_si64(src2, hmask);   /* src & mask -> src2 */
            src2 = _mm_add_pi32(src2, dst2);    /* dst2 + src2 -> src2 */
            src2 = _mm_srli_pi32(src2, 1);      /* src2 >> 1 -> src2 */

            dst1 = _mm_and_si64(dst1, src1);    /* src & dst -> dst1 */
            dst1 = _mm_and_si64(dst1, lmask);   /* dst1 & !mask -> dst1 */
            dst1 = _mm_add_pi32(dst1, src2);    /* src2 + dst1 -> dst1 */
            dst1 = _mm_or_si64(dst1, dsta);     /* dsta(full alpha) | dst1 -> dst1 */

            *(__m64 *) dstp = dst1;     /* dst1 -> 2 x dst pixels */
            dstp += 2;
            srcp += 2;
        }

        srcp += srcskip;
        dstp += dstskip;
    }
    _mm_empty();
}

/* fast RGB888->(A)RGB888 blending with surface alpha */
static void
BlitRGBtoRGBSurfaceAlphaMMX(SDL_BlitInfo * info)
{
    SDL_PixelFormat *df = info->dst_fmt;
    Uint32 chanmask = df->Rmask | df->Gmask | df->Bmask;
    unsigned alpha = info->a;

    if (alpha == 128 && (df->Rmask | df->Gmask | df->Bmask) == 0x00FFFFFF) {
        /* only call a128 version when R,G,B occupy lower bits */
        BlitRGBtoRGBSurfaceAlpha128MMX(info);
    } else {
        int width = info->dst_w;
        int height = info->dst_h;
        Uint32 *srcp = (Uint32 *) info->src;
        int srcskip = info->src_skip >> 2;
        Uint32 *dstp = (Uint32 *) info->dst;
        int dstskip = info->dst_skip >> 2;
        Uint32 dalpha = df->Amask;
        Uint32 amult;

        __m64 src1, src2, dst1, dst2, mm_alpha, mm_zero, dsta;

        mm_zero = _mm_setzero_si64();   /* 0 -> mm_zero */
        /* form the alpha mult */
        amult = alpha | (alpha << 8);
        amult = amult | (amult << 16);
        chanmask =
            (0xff << df->Rshift) | (0xff << df->
                                    Gshift) | (0xff << df->Bshift);
        mm_alpha = _mm_set_pi32(0, amult & chanmask);   /* 0000AAAA -> mm_alpha, minus 1 chan */
        mm_alpha = _mm_unpacklo_pi8(mm_alpha, mm_zero); /* 0A0A0A0A -> mm_alpha, minus 1 chan */
        /* at this point mm_alpha can be 000A0A0A or 0A0A0A00 or another combo */
        dsta = _mm_set_pi32(dalpha, dalpha);    /* dst alpha mask -> dsta */

        while (height--) {
            int n = width;
            if (n & 1) {
                /* One Pixel Blend */
                src2 = _mm_cvtsi32_si64(*srcp); /* src(ARGB) -> src2 (0000ARGB) */
                src2 = _mm_unpacklo_pi8(src2, mm_zero); /* 0A0R0G0B -> src2 */

                dst1 = _mm_cvtsi32_si64(*dstp); /* dst(ARGB) -> dst1 (0000ARGB) */
                dst1 = _mm_unpacklo_pi8(dst1, mm_zero); /* 0A0R0G0B -> dst1 */

                src2 = _mm_sub_pi16(src2, dst1);        /* src2 - dst2 -> src2 */
                src2 = _mm_mullo_pi16(src2, mm_alpha);  /* src2 * alpha -> src2 */
                src2 = _mm_srli_pi16(src2, 8);  /* src2 >> 8 -> src2 */
                dst1 = _mm_add_pi8(src2, dst1); /* src2 + dst1 -> dst1 */

                dst1 = _mm_packs_pu16(dst1, mm_zero);   /* 0000ARGB -> dst1 */
                dst1 = _mm_or_si64(dst1, dsta); /* dsta | dst1 -> dst1 */
                *dstp = _mm_cvtsi64_si32(dst1); /* dst1 -> pixel */

                ++srcp;
                ++dstp;

                n--;
            }

            for (n >>= 1; n > 0; --n) {
                /* Two Pixels Blend */
                src1 = *(__m64 *) srcp; /* 2 x src -> src1(ARGBARGB) */
                src2 = src1;    /* 2 x src -> src2(ARGBARGB) */
                src1 = _mm_unpacklo_pi8(src1, mm_zero); /* low - 0A0R0G0B -> src1 */
                src2 = _mm_unpackhi_pi8(src2, mm_zero); /* high - 0A0R0G0B -> src2 */

                dst1 = *(__m64 *) dstp; /* 2 x dst -> dst1(ARGBARGB) */
                dst2 = dst1;    /* 2 x dst -> dst2(ARGBARGB) */
                dst1 = _mm_unpacklo_pi8(dst1, mm_zero); /* low - 0A0R0G0B -> dst1 */
                dst2 = _mm_unpackhi_pi8(dst2, mm_zero); /* high - 0A0R0G0B -> dst2 */

                src1 = _mm_sub_pi16(src1, dst1);        /* src1 - dst1 -> src1 */
                src1 = _mm_mullo_pi16(src1, mm_alpha);  /* src1 * alpha -> src1 */
                src1 = _mm_srli_pi16(src1, 8);  /* src1 >> 8 -> src1 */
                dst1 = _mm_add_pi8(src1, dst1); /* src1 + dst1(dst1) -> dst1 */

                src2 = _mm_sub_pi16(src2, dst2);        /* src2 - dst2 -> src2 */
                src2 = _mm_mullo_pi16(src2, mm_alpha);  /* src2 * alpha -> src2 */
                src2 = _mm_srli_pi16(src2, 8);  /* src2 >> 8 -> src2 */
                dst2 = _mm_add_pi8(src2, dst2); /* src2 + dst2(dst2) -> dst2 */

                dst1 = _mm_packs_pu16(dst1, dst2);      /* 0A0R0G0B(res1), 0A0R0G0B(res2) -> dst1(ARGBARGB) */
                dst1 = _mm_or_si64(dst1, dsta); /* dsta | dst1 -> dst1 */

                *(__m64 *) dstp = dst1; /* dst1 -> 2 x pixel */

                srcp += 2;
                dstp += 2;
            }
            srcp += srcskip;
            dstp += dstskip;
        }
        _mm_empty();
    }
}

/* fast ARGB888->(A)RGB888 blending with pixel alpha */
static void
BlitRGBtoRGBPixelAlphaMMX(SDL_BlitInfo * info)
{
    int width = info->dst_w;
    int height = info->dst_h;
    Uint32 *srcp = (Uint32 *) info->src;
    int srcskip = info->src_skip >> 2;
    Uint32 *dstp = (Uint32 *) info->dst;
    int dstskip = info->dst_skip >> 2;
    SDL_PixelFormat *sf = info->src_fmt;
    Uint32 chanmask = sf->Rmask | sf->Gmask | sf->Bmask;
    Uint32 amask = sf->Amask;
    Uint32 ashift = sf->Ashift;
    Uint64 multmask;

    __m64 src1, dst1, mm_alpha, mm_zero, dmask;

    mm_zero = _mm_setzero_si64();       /* 0 -> mm_zero */
    multmask = 0xFFFF;
    multmask <<= (ashift * 2);
    multmask = ~multmask;
    dmask = *(__m64 *) & multmask;      /* dst alpha mask -> dmask */

    while (height--) {
		/* *INDENT-OFF* */
		DUFFS_LOOP4({
		Uint32 alpha = *srcp & amask;
		if (alpha == 0) {
			/* do nothing */
		} else if (alpha == amask) {
			/* opaque alpha -- copy RGB, keep dst alpha */
			*dstp = (*srcp & chanmask) | (*dstp & ~chanmask);
		} else {
			src1 = _mm_cvtsi32_si64(*srcp); /* src(ARGB) -> src1 (0000ARGB)*/
			src1 = _mm_unpacklo_pi8(src1, mm_zero); /* 0A0R0G0B -> src1 */

			dst1 = _mm_cvtsi32_si64(*dstp); /* dst(ARGB) -> dst1 (0000ARGB)*/
			dst1 = _mm_unpacklo_pi8(dst1, mm_zero); /* 0A0R0G0B -> dst1 */

			mm_alpha = _mm_cvtsi32_si64(alpha); /* alpha -> mm_alpha (0000000A) */
			mm_alpha = _mm_srli_si64(mm_alpha, ashift); /* mm_alpha >> ashift -> mm_alpha(0000000A) */
			mm_alpha = _mm_unpacklo_pi16(mm_alpha, mm_alpha); /* 00000A0A -> mm_alpha */
			mm_alpha = _mm_unpacklo_pi32(mm_alpha, mm_alpha); /* 0A0A0A0A -> mm_alpha */
			mm_alpha = _mm_and_si64(mm_alpha, dmask); /* 000A0A0A -> mm_alpha, preserve dst alpha on add */

			/* blend */		    
			src1 = _mm_sub_pi16(src1, dst1);/* src1 - dst1 -> src1 */
			src1 = _mm_mullo_pi16(src1, mm_alpha); /* (src1 - dst1) * alpha -> src1 */
			src1 = _mm_srli_pi16(src1, 8); /* src1 >> 8 -> src1(000R0G0B) */
			dst1 = _mm_add_pi8(src1, dst1); /* src1 + dst1 -> dst1(0A0R0G0B) */
			dst1 = _mm_packs_pu16(dst1, mm_zero);  /* 0000ARGB -> dst1 */
			
			*dstp = _mm_cvtsi64_si32(dst1); /* dst1 -> pixel */
		}
		++srcp;
		++dstp;
	    }, width);
		/* *INDENT-ON* */
        srcp += srcskip;
        dstp += dstskip;
    }
    _mm_empty();
}

#endif /* __MMX__ */

#if SDL_ALTIVEC_BLITTERS
#if __MWERKS__
#pragma altivec_model on
#endif
#if HAVE_ALTIVEC_H
#include <altivec.h>
#endif

#if (defined(__MACOSX__) && (__GNUC__ < 4))
#define VECUINT8_LITERAL(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) \
        (vector unsigned char) ( a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p )
#define VECUINT16_LITERAL(a,b,c,d,e,f,g,h) \
        (vector unsigned short) ( a,b,c,d,e,f,g,h )
#else
#define VECUINT8_LITERAL(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) \
        (vector unsigned char) { a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p }
#define VECUINT16_LITERAL(a,b,c,d,e,f,g,h) \
        (vector unsigned short) { a,b,c,d,e,f,g,h }
#endif

#define UNALIGNED_PTR(x) (((size_t) x) & 0x0000000F)
#define VECPRINT(msg, v) do { \
    vector unsigned int tmpvec = (vector unsigned int)(v); \
    unsigned int *vp = (unsigned int *)&tmpvec; \
    printf("%s = %08X %08X %08X %08X\n", msg, vp[0], vp[1], vp[2], vp[3]); \
} while (0)

/* the permuation vector that takes the high bytes out of all the appropriate shorts 
    (vector unsigned char)(
        0x00, 0x10, 0x02, 0x12,
        0x04, 0x14, 0x06, 0x16,
        0x08, 0x18, 0x0A, 0x1A,
        0x0C, 0x1C, 0x0E, 0x1E );
*/
#define VEC_MERGE_PERMUTE() (vec_add(vec_lvsl(0, (int*)NULL), (vector unsigned char)vec_splat_u16(0x0F)))
#define VEC_U32_24() (vec_add(vec_splat_u32(12), vec_splat_u32(12)))
#define VEC_ALPHA_MASK() ((vector unsigned char)vec_sl((vector unsigned int)vec_splat_s8(-1), VEC_U32_24()))
#define VEC_ALIGNER(src) ((UNALIGNED_PTR(src)) \
    ? vec_lvsl(0, src) \
    : vec_add(vec_lvsl(8, src), vec_splat_u8(8)))


#define VEC_MULTIPLY_ALPHA(vs, vd, valpha, mergePermute, v1_16, v8_16) do { \
    /* vtemp1 contains source AAGGAAGGAAGGAAGG */ \
    vector unsigned short vtemp1 = vec_mule(vs, valpha); \
    /* vtemp2 contains source RRBBRRBBRRBBRRBB */ \
    vector unsigned short vtemp2 = vec_mulo(vs, valpha); \
    /* valpha2 is 255-alpha */ \
    vector unsigned char valpha2 = vec_nor(valpha, valpha); \
    /* vtemp3 contains dest AAGGAAGGAAGGAAGG */ \
    vector unsigned short vtemp3 = vec_mule(vd, valpha2); \
    /* vtemp4 contains dest RRBBRRBBRRBBRRBB */ \
    vector unsigned short vtemp4 = vec_mulo(vd, valpha2); \
    /* add source and dest */ \
    vtemp1 = vec_add(vtemp1, vtemp3); \
    vtemp2 = vec_add(vtemp2, vtemp4); \
    /* vtemp1 = (vtemp1 + 1) + ((vtemp1 + 1) >> 8) */ \
    vtemp1 = vec_add(vtemp1, v1_16); \
    vtemp3 = vec_sr(vtemp1, v8_16); \
    vtemp1 = vec_add(vtemp1, vtemp3); \
    /* vtemp2 = (vtemp2 + 1) + ((vtemp2 + 1) >> 8) */ \
    vtemp2 = vec_add(vtemp2, v1_16); \
    vtemp4 = vec_sr(vtemp2, v8_16); \
    vtemp2 = vec_add(vtemp2, vtemp4); \
    /* (>>8) and get ARGBARGBARGBARGB */ \
    vd = (vector unsigned char)vec_perm(vtemp1, vtemp2, mergePermute); \
} while (0)

/* Calculate the permute vector used for 32->32 swizzling */
static vector unsigned char
calc_swizzle32(const SDL_PixelFormat * srcfmt, const SDL_PixelFormat * dstfmt)
{
    /*
     * We have to assume that the bits that aren't used by other
     *  colors is alpha, and it's one complete byte, since some formats
     *  leave alpha with a zero mask, but we should still swizzle the bits.
     */
    /* ARGB */
    const static struct SDL_PixelFormat default_pixel_format = {
        0, NULL, 0, 0,
        {0, 0},
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000,
        0, 0, 0, 0,
        16, 8, 0, 24,
        0, NULL
    };
    if (!srcfmt) {
        srcfmt = &default_pixel_format;
    }
    if (!dstfmt) {
        dstfmt = &default_pixel_format;
    }
    const vector unsigned char plus = VECUINT8_LITERAL(0x00, 0x00, 0x00, 0x00,
                                                       0x04, 0x04, 0x04, 0x04,
                                                       0x08, 0x08, 0x08, 0x08,
                                                       0x0C, 0x0C, 0x0C,
                                                       0x0C);
    vector unsigned char vswiz;
    vector unsigned int srcvec;
#define RESHIFT(X) (3 - ((X) >> 3))
    Uint32 rmask = RESHIFT(srcfmt->Rshift) << (dstfmt->Rshift);
    Uint32 gmask = RESHIFT(srcfmt->Gshift) << (dstfmt->Gshift);
    Uint32 bmask = RESHIFT(srcfmt->Bshift) << (dstfmt->Bshift);
    Uint32 amask;
    /* Use zero for alpha if either surface doesn't have alpha */
    if (dstfmt->Amask) {
        amask =
            ((srcfmt->Amask) ? RESHIFT(srcfmt->
                                       Ashift) : 0x10) << (dstfmt->Ashift);
    } else {
        amask =
            0x10101010 & ((dstfmt->Rmask | dstfmt->Gmask | dstfmt->Bmask) ^
                          0xFFFFFFFF);
    }
#undef RESHIFT
    ((unsigned int *) (char *) &srcvec)[0] = (rmask | gmask | bmask | amask);
    vswiz = vec_add(plus, (vector unsigned char) vec_splat(srcvec, 0));
    return (vswiz);
}

static void
Blit32to565PixelAlphaAltivec(SDL_BlitInfo * info)
{
    int height = info->dst_h;
    Uint8 *src = (Uint8 *) info->src;
    int srcskip = info->src_skip;
    Uint8 *dst = (Uint8 *) info->dst;
    int dstskip = info->dst_skip;
    SDL_PixelFormat *srcfmt = info->src_fmt;

    vector unsigned char v0 = vec_splat_u8(0);
    vector unsigned short v8_16 = vec_splat_u16(8);
    vector unsigned short v1_16 = vec_splat_u16(1);
    vector unsigned short v2_16 = vec_splat_u16(2);
    vector unsigned short v3_16 = vec_splat_u16(3);
    vector unsigned int v8_32 = vec_splat_u32(8);
    vector unsigned int v16_32 = vec_add(v8_32, v8_32);
    vector unsigned short v3f =
        VECUINT16_LITERAL(0x003f, 0x003f, 0x003f, 0x003f,
                          0x003f, 0x003f, 0x003f, 0x003f);
    vector unsigned short vfc =
        VECUINT16_LITERAL(0x00fc, 0x00fc, 0x00fc, 0x00fc,
                          0x00fc, 0x00fc, 0x00fc, 0x00fc);

    /* 
       0x10 - 0x1f is the alpha
       0x00 - 0x0e evens are the red
       0x01 - 0x0f odds are zero
     */
    vector unsigned char vredalpha1 = VECUINT8_LITERAL(0x10, 0x00, 0x01, 0x01,
                                                       0x10, 0x02, 0x01, 0x01,
                                                       0x10, 0x04, 0x01, 0x01,
                                                       0x10, 0x06, 0x01,
                                                       0x01);
    vector unsigned char vredalpha2 =
        (vector unsigned char) (vec_add((vector unsigned int) vredalpha1,
                                        vec_sl(v8_32, v16_32))
        );
    /*
       0x00 - 0x0f is ARxx ARxx ARxx ARxx
       0x11 - 0x0f odds are blue
     */
    vector unsigned char vblue1 = VECUINT8_LITERAL(0x00, 0x01, 0x02, 0x11,
                                                   0x04, 0x05, 0x06, 0x13,
                                                   0x08, 0x09, 0x0a, 0x15,
                                                   0x0c, 0x0d, 0x0e, 0x17);
    vector unsigned char vblue2 =
        (vector unsigned char) (vec_add((vector unsigned int) vblue1, v8_32)
        );
    /*
       0x00 - 0x0f is ARxB ARxB ARxB ARxB
       0x10 - 0x0e evens are green
     */
    vector unsigned char vgreen1 = VECUINT8_LITERAL(0x00, 0x01, 0x10, 0x03,
                                                    0x04, 0x05, 0x12, 0x07,
                                                    0x08, 0x09, 0x14, 0x0b,
                                                    0x0c, 0x0d, 0x16, 0x0f);
    vector unsigned char vgreen2 =
        (vector unsigned
         char) (vec_add((vector unsigned int) vgreen1, vec_sl(v8_32, v8_32))
        );
    vector unsigned char vgmerge = VECUINT8_LITERAL(0x00, 0x02, 0x00, 0x06,
                                                    0x00, 0x0a, 0x00, 0x0e,
                                                    0x00, 0x12, 0x00, 0x16,
                                                    0x00, 0x1a, 0x00, 0x1e);
    vector unsigned char mergePermute = VEC_MERGE_PERMUTE();
    vector unsigned char vpermute = calc_swizzle32(srcfmt, NULL);
    vector unsigned char valphaPermute =
        vec_and(vec_lvsl(0, (int *) NULL), vec_splat_u8(0xC));

    vector unsigned short vf800 = (vector unsigned short) vec_splat_u8(-7);
    vf800 = vec_sl(vf800, vec_splat_u16(8));

    while (height--) {
        int extrawidth;
        vector unsigned char valigner;
        vector unsigned char vsrc;
        vector unsigned char voverflow;
        int width = info->dst_w;

#define ONE_PIXEL_BLEND(condition, widthvar) \
        while (condition) { \
            Uint32 Pixel; \
            unsigned sR, sG, sB, dR, dG, dB, sA; \
            DISEMBLE_RGBA(src, 4, srcfmt, Pixel, sR, sG, sB, sA); \
            if(sA) { \
                unsigned short dstpixel = *((unsigned short *)dst); \
                dR = (dstpixel >> 8) & 0xf8; \
                dG = (dstpixel >> 3) & 0xfc; \
                dB = (dstpixel << 3) & 0xf8; \
                ALPHA_BLEND(sR, sG, sB, sA, dR, dG, dB); \
                *((unsigned short *)dst) = ( \
                    ((dR & 0xf8) << 8) | ((dG & 0xfc) << 3) | (dB >> 3) \
                ); \
            } \
            src += 4; \
            dst += 2; \
            widthvar--; \
        }
        ONE_PIXEL_BLEND((UNALIGNED_PTR(dst)) && (width), width);
        extrawidth = (width % 8);
        valigner = VEC_ALIGNER(src);
        vsrc = (vector unsigned char) vec_ld(0, src);
        width -= extrawidth;
        while (width) {
            vector unsigned char valpha;
            vector unsigned char vsrc1, vsrc2;
            vector unsigned char vdst1, vdst2;
            vector unsigned short vR, vG, vB;
            vector unsigned short vpixel, vrpixel, vgpixel, vbpixel;

            /* Load 8 pixels from src as ARGB */
            voverflow = (vector unsigned char) vec_ld(15, src);
            vsrc = vec_perm(vsrc, voverflow, valigner);
            vsrc1 = vec_perm(vsrc, vsrc, vpermute);
            src += 16;
            vsrc = (vector unsigned char) vec_ld(15, src);
            voverflow = vec_perm(voverflow, vsrc, valigner);
            vsrc2 = vec_perm(voverflow, voverflow, vpermute);
            src += 16;

            /* Load 8 pixels from dst as XRGB */
            voverflow = vec_ld(0, dst);
            vR = vec_and((vector unsigned short) voverflow, vf800);
            vB = vec_sl((vector unsigned short) voverflow, v3_16);
            vG = vec_sl(vB, v2_16);
            vdst1 =
                (vector unsigned char) vec_perm((vector unsigned char) vR,
                                                (vector unsigned char) vR,
                                                vredalpha1);
            vdst1 = vec_perm(vdst1, (vector unsigned char) vB, vblue1);
            vdst1 = vec_perm(vdst1, (vector unsigned char) vG, vgreen1);
            vdst2 =
                (vector unsigned char) vec_perm((vector unsigned char) vR,
                                                (vector unsigned char) vR,
                                                vredalpha2);
            vdst2 = vec_perm(vdst2, (vector unsigned char) vB, vblue2);
            vdst2 = vec_perm(vdst2, (vector unsigned char) vG, vgreen2);

            /* Alpha blend 8 pixels as ARGB */
            valpha = vec_perm(vsrc1, v0, valphaPermute);
            VEC_MULTIPLY_ALPHA(vsrc1, vdst1, valpha, mergePermute, v1_16,
                               v8_16);
            valpha = vec_perm(vsrc2, v0, valphaPermute);
            VEC_MULTIPLY_ALPHA(vsrc2, vdst2, valpha, mergePermute, v1_16,
                               v8_16);

            /* Convert 8 pixels to 565 */
            vpixel = (vector unsigned short) vec_packpx((vector unsigned int)
                                                        vdst1,
                                                        (vector unsigned int)
                                                        vdst2);
            vgpixel = (vector unsigned short) vec_perm(vdst1, vdst2, vgmerge);
            vgpixel = vec_and(vgpixel, vfc);
            vgpixel = vec_sl(vgpixel, v3_16);
            vrpixel = vec_sl(vpixel, v1_16);
            vrpixel = vec_and(vrpixel, vf800);
            vbpixel = vec_and(vpixel, v3f);
            vdst1 =
                vec_or((vector unsigned char) vrpixel,
                       (vector unsigned char) vgpixel);
            vdst1 = vec_or(vdst1, (vector unsigned char) vbpixel);

            /* Store 8 pixels */
            vec_st(vdst1, 0, dst);

            width -= 8;
            dst += 16;
        }
        ONE_PIXEL_BLEND((extrawidth), extrawidth);
#undef ONE_PIXEL_BLEND
        src += srcskip;
        dst += dstskip;
    }
}

static void
Blit32to32SurfaceAlphaKeyAltivec(SDL_BlitInfo * info)
{
    int height = info->dst_h;
    Uint32 *srcp = (Uint32 *) info->src;
    int srcskip = info->src_skip >> 2;
    Uint32 *dstp = (Uint32 *) info->dst;
    int dstskip = info->dst_skip >> 2;
    SDL_PixelFormat *srcfmt = info->src_fmt;
    SDL_PixelFormat *dstfmt = info->dst_fmt;
    unsigned sA = info->a;
    unsigned dA = dstfmt->Amask ? SDL_ALPHA_OPAQUE : 0;
    Uint32 rgbmask = srcfmt->Rmask | srcfmt->Gmask | srcfmt->Bmask;
    Uint32 ckey = info->colorkey;
    vector unsigned char mergePermute;
    vector unsigned char vsrcPermute;
    vector unsigned char vdstPermute;
    vector unsigned char vsdstPermute;
    vector unsigned char valpha;
    vector unsigned char valphamask;
    vector unsigned char vbits;
    vector unsigned char v0;
    vector unsigned short v1;
    vector unsigned short v8;
    vector unsigned int vckey;
    vector unsigned int vrgbmask;

    mergePermute = VEC_MERGE_PERMUTE();
    v0 = vec_splat_u8(0);
    v1 = vec_splat_u16(1);
    v8 = vec_splat_u16(8);

    /* set the alpha to 255 on the destination surf */
    valphamask = VEC_ALPHA_MASK();

    vsrcPermute = calc_swizzle32(srcfmt, NULL);
    vdstPermute = calc_swizzle32(NULL, dstfmt);
    vsdstPermute = calc_swizzle32(dstfmt, NULL);

    /* set a vector full of alpha and 255-alpha */
    ((unsigned char *) &valpha)[0] = sA;
    valpha = vec_splat(valpha, 0);
    vbits = (vector unsigned char) vec_splat_s8(-1);

    ckey &= rgbmask;
    ((unsigned int *) (char *) &vckey)[0] = ckey;
    vckey = vec_splat(vckey, 0);
    ((unsigned int *) (char *) &vrgbmask)[0] = rgbmask;
    vrgbmask = vec_splat(vrgbmask, 0);

    while (height--) {
        int width = info->dst_w;
#define ONE_PIXEL_BLEND(condition, widthvar) \
        while (condition) { \
            Uint32 Pixel; \
            unsigned sR, sG, sB, dR, dG, dB; \
            RETRIEVE_RGB_PIXEL(((Uint8 *)srcp), 4, Pixel); \
            if(sA && Pixel != ckey) { \
                RGB_FROM_PIXEL(Pixel, srcfmt, sR, sG, sB); \
                DISEMBLE_RGB(((Uint8 *)dstp), 4, dstfmt, Pixel, dR, dG, dB); \
                ALPHA_BLEND(sR, sG, sB, sA, dR, dG, dB); \
                ASSEMBLE_RGBA(((Uint8 *)dstp), 4, dstfmt, dR, dG, dB, dA); \
            } \
            dstp++; \
            srcp++; \
            widthvar--; \
        }
        ONE_PIXEL_BLEND((UNALIGNED_PTR(dstp)) && (width), width);
        if (width > 0) {
            int extrawidth = (width % 4);
            vector unsigned char valigner = VEC_ALIGNER(srcp);
            vector unsigned char vs = (vector unsigned char) vec_ld(0, srcp);
            width -= extrawidth;
            while (width) {
                vector unsigned char vsel;
                vector unsigned char voverflow;
                vector unsigned char vd;
                vector unsigned char vd_orig;

                /* s = *srcp */
                voverflow = (vector unsigned char) vec_ld(15, srcp);
                vs = vec_perm(vs, voverflow, valigner);

                /* vsel is set for items that match the key */
                vsel =
                    (vector unsigned char) vec_and((vector unsigned int) vs,
                                                   vrgbmask);
                vsel = (vector unsigned char) vec_cmpeq((vector unsigned int)
                                                        vsel, vckey);

                /* permute to source format */
                vs = vec_perm(vs, valpha, vsrcPermute);

                /* d = *dstp */
                vd = (vector unsigned char) vec_ld(0, dstp);
                vd_orig = vd = vec_perm(vd, v0, vsdstPermute);

                VEC_MULTIPLY_ALPHA(vs, vd, valpha, mergePermute, v1, v8);

                /* set the alpha channel to full on */
                vd = vec_or(vd, valphamask);

                /* mask out color key */
                vd = vec_sel(vd, vd_orig, vsel);

                /* permute to dest format */
                vd = vec_perm(vd, vbits, vdstPermute);

                /* *dstp = res */
                vec_st((vector unsigned int) vd, 0, dstp);

                srcp += 4;
                dstp += 4;
                width -= 4;
                vs = voverflow;
            }
            ONE_PIXEL_BLEND((extrawidth), extrawidth);
        }
#undef ONE_PIXEL_BLEND

        srcp += srcskip;
        dstp += dstskip;
    }
}


static void
Blit32to32PixelAlphaAltivec(SDL_BlitInfo * info)
{
    int width = info->dst_w;
    int height = info->dst_h;
    Uint32 *srcp = (Uint32 *) info->src;
    int srcskip = info->src_skip >> 2;
    Uint32 *dstp = (Uint32 *) info->dst;
    int dstskip = info->dst_skip >> 2;
    SDL_PixelFormat *srcfmt = info->src_fmt;
    SDL_PixelFormat *dstfmt = info->dst_fmt;
    vector unsigned char mergePermute;
    vector unsigned char valphaPermute;
    vector unsigned char vsrcPermute;
    vector unsigned char vdstPermute;
    vector unsigned char vsdstPermute;
    vector unsigned char valphamask;
    vector unsigned char vpixelmask;
    vector unsigned char v0;
    vector unsigned short v1;
    vector unsigned short v8;

    v0 = vec_splat_u8(0);
    v1 = vec_splat_u16(1);
    v8 = vec_splat_u16(8);
    mergePermute = VEC_MERGE_PERMUTE();
    valphamask = VEC_ALPHA_MASK();
    valphaPermute = vec_and(vec_lvsl(0, (int *) NULL), vec_splat_u8(0xC));
    vpixelmask = vec_nor(valphamask, v0);
    vsrcPermute = calc_swizzle32(srcfmt, NULL);
    vdstPermute = calc_swizzle32(NULL, dstfmt);
    vsdstPermute = calc_swizzle32(dstfmt, NULL);

    while (height--) {
        width = info->dst_w;
#define ONE_PIXEL_BLEND(condition, widthvar) while ((condition)) { \
            Uint32 Pixel; \
            unsigned sR, sG, sB, dR, dG, dB, sA, dA; \
            DISEMBLE_RGBA((Uint8 *)srcp, 4, srcfmt, Pixel, sR, sG, sB, sA); \
            if(sA) { \
              DISEMBLE_RGBA((Uint8 *)dstp, 4, dstfmt, Pixel, dR, dG, dB, dA); \
              ALPHA_BLEND(sR, sG, sB, sA, dR, dG, dB); \
              ASSEMBLE_RGBA((Uint8 *)dstp, 4, dstfmt, dR, dG, dB, dA); \
            } \
            ++srcp; \
            ++dstp; \
            widthvar--; \
        }
        ONE_PIXEL_BLEND((UNALIGNED_PTR(dstp)) && (width), width);
        if (width > 0) {
            /* vsrcPermute */
            /* vdstPermute */
            int extrawidth = (width % 4);
            vector unsigned char valigner = VEC_ALIGNER(srcp);
            vector unsigned char vs = (vector unsigned char) vec_ld(0, srcp);
            width -= extrawidth;
            while (width) {
                vector unsigned char voverflow;
                vector unsigned char vd;
                vector unsigned char valpha;
                vector unsigned char vdstalpha;
                /* s = *srcp */
                voverflow = (vector unsigned char) vec_ld(15, srcp);
                vs = vec_perm(vs, voverflow, valigner);
                vs = vec_perm(vs, v0, vsrcPermute);

                valpha = vec_perm(vs, v0, valphaPermute);

                /* d = *dstp */
                vd = (vector unsigned char) vec_ld(0, dstp);
                vd = vec_perm(vd, v0, vsdstPermute);
                vdstalpha = vec_and(vd, valphamask);

                VEC_MULTIPLY_ALPHA(vs, vd, valpha, mergePermute, v1, v8);

                /* set the alpha to the dest alpha */
                vd = vec_and(vd, vpixelmask);
                vd = vec_or(vd, vdstalpha);
                vd = vec_perm(vd, v0, vdstPermute);

                /* *dstp = res */
                vec_st((vector unsigned int) vd, 0, dstp);

                srcp += 4;
                dstp += 4;
                width -= 4;
                vs = voverflow;

            }
            ONE_PIXEL_BLEND((extrawidth), extrawidth);
        }
        srcp += srcskip;
        dstp += dstskip;
#undef ONE_PIXEL_BLEND
    }
}

/* fast ARGB888->(A)RGB888 blending with pixel alpha */
static void
BlitRGBtoRGBPixelAlphaAltivec(SDL_BlitInfo * info)
{
    int width = info->dst_w;
    int height = info->dst_h;
    Uint32 *srcp = (Uint32 *) info->src;
    int srcskip = info->src_skip >> 2;
    Uint32 *dstp = (Uint32 *) info->dst;
    int dstskip = info->dst_skip >> 2;
    vector unsigned char mergePermute;
    vector unsigned char valphaPermute;
    vector unsigned char valphamask;
    vector unsigned char vpixelmask;
    vector unsigned char v0;
    vector unsigned short v1;
    vector unsigned short v8;
    v0 = vec_splat_u8(0);
    v1 = vec_splat_u16(1);
    v8 = vec_splat_u16(8);
    mergePermute = VEC_MERGE_PERMUTE();
    valphamask = VEC_ALPHA_MASK();
    valphaPermute = vec_and(vec_lvsl(0, (int *) NULL), vec_splat_u8(0xC));


    vpixelmask = vec_nor(valphamask, v0);
    while (height--) {
        width = info->dst_w;
#define ONE_PIXEL_BLEND(condition, widthvar) \
        while ((condition)) { \
            Uint32 dalpha; \
            Uint32 d; \
            Uint32 s1; \
            Uint32 d1; \
            Uint32 s = *srcp; \
            Uint32 alpha = s >> 24; \
            if(alpha) { \
              if(alpha == SDL_ALPHA_OPAQUE) { \
                *dstp = (s & 0x00ffffff) | (*dstp & 0xff000000); \
              } else { \
                d = *dstp; \
                dalpha = d & 0xff000000; \
                s1 = s & 0xff00ff; \
                d1 = d & 0xff00ff; \
                d1 = (d1 + ((s1 - d1) * alpha >> 8)) & 0xff00ff; \
                s &= 0xff00; \
                d &= 0xff00; \
                d = (d + ((s - d) * alpha >> 8)) & 0xff00; \
                *dstp = d1 | d | dalpha; \
              } \
            } \
            ++srcp; \
            ++dstp; \
            widthvar--; \
	    }
        ONE_PIXEL_BLEND((UNALIGNED_PTR(dstp)) && (width), width);
        if (width > 0) {
            int extrawidth = (width % 4);
            vector unsigned char valigner = VEC_ALIGNER(srcp);
            vector unsigned char vs = (vector unsigned char) vec_ld(0, srcp);
            width -= extrawidth;
            while (width) {
                vector unsigned char voverflow;
                vector unsigned char vd;
                vector unsigned char valpha;
                vector unsigned char vdstalpha;
                /* s = *srcp */
                voverflow = (vector unsigned char) vec_ld(15, srcp);
                vs = vec_perm(vs, voverflow, valigner);

                valpha = vec_perm(vs, v0, valphaPermute);

                /* d = *dstp */
                vd = (vector unsigned char) vec_ld(0, dstp);
                vdstalpha = vec_and(vd, valphamask);

                VEC_MULTIPLY_ALPHA(vs, vd, valpha, mergePermute, v1, v8);

                /* set the alpha to the dest alpha */
                vd = vec_and(vd, vpixelmask);
                vd = vec_or(vd, vdstalpha);

                /* *dstp = res */
                vec_st((vector unsigned int) vd, 0, dstp);

                srcp += 4;
                dstp += 4;
                width -= 4;
                vs = voverflow;
            }
            ONE_PIXEL_BLEND((extrawidth), extrawidth);
        }
        srcp += srcskip;
        dstp += dstskip;
    }
#undef ONE_PIXEL_BLEND
}

static void
Blit32to32SurfaceAlphaAltivec(SDL_BlitInfo * info)
{
    /* XXX : 6 */
    int height = info->dst_h;
    Uint32 *srcp = (Uint32 *) info->src;
    int srcskip = info->src_skip >> 2;
    Uint32 *dstp = (Uint32 *) info->dst;
    int dstskip = info->dst_skip >> 2;
    SDL_PixelFormat *srcfmt = info->src_fmt;
    SDL_PixelFormat *dstfmt = info->dst_fmt;
    unsigned sA = info->a;
    unsigned dA = dstfmt->Amask ? SDL_ALPHA_OPAQUE : 0;
    vector unsigned char mergePermute;
    vector unsigned char vsrcPermute;
    vector unsigned char vdstPermute;
    vector unsigned char vsdstPermute;
    vector unsigned char valpha;
    vector unsigned char valphamask;
    vector unsigned char vbits;
    vector unsigned short v1;
    vector unsigned short v8;

    mergePermute = VEC_MERGE_PERMUTE();
    v1 = vec_splat_u16(1);
    v8 = vec_splat_u16(8);

    /* set the alpha to 255 on the destination surf */
    valphamask = VEC_ALPHA_MASK();

    vsrcPermute = calc_swizzle32(srcfmt, NULL);
    vdstPermute = calc_swizzle32(NULL, dstfmt);
    vsdstPermute = calc_swizzle32(dstfmt, NULL);

    /* set a vector full of alpha and 255-alpha */
    ((unsigned char *) &valpha)[0] = sA;
    valpha = vec_splat(valpha, 0);
    vbits = (vector unsigned char) vec_splat_s8(-1);

    while (height--) {
        int width = info->dst_w;
#define ONE_PIXEL_BLEND(condition, widthvar) while ((condition)) { \
            Uint32 Pixel; \
            unsigned sR, sG, sB, dR, dG, dB; \
            DISEMBLE_RGB(((Uint8 *)srcp), 4, srcfmt, Pixel, sR, sG, sB); \
            DISEMBLE_RGB(((Uint8 *)dstp), 4, dstfmt, Pixel, dR, dG, dB); \
            ALPHA_BLEND(sR, sG, sB, sA, dR, dG, dB); \
            ASSEMBLE_RGBA(((Uint8 *)dstp), 4, dstfmt, dR, dG, dB, dA); \
            ++srcp; \
            ++dstp; \
            widthvar--; \
        }
        ONE_PIXEL_BLEND((UNALIGNED_PTR(dstp)) && (width), width);
        if (width > 0) {
            int extrawidth = (width % 4);
            vector unsigned char valigner = VEC_ALIGNER(srcp);
            vector unsigned char vs = (vector unsigned char) vec_ld(0, srcp);
            width -= extrawidth;
            while (width) {
                vector unsigned char voverflow;
                vector unsigned char vd;

                /* s = *srcp */
                voverflow = (vector unsigned char) vec_ld(15, srcp);
                vs = vec_perm(vs, voverflow, valigner);
                vs = vec_perm(vs, valpha, vsrcPermute);

                /* d = *dstp */
                vd = (vector unsigned char) vec_ld(0, dstp);
                vd = vec_perm(vd, vd, vsdstPermute);

                VEC_MULTIPLY_ALPHA(vs, vd, valpha, mergePermute, v1, v8);

                /* set the alpha channel to full on */
                vd = vec_or(vd, valphamask);
                vd = vec_perm(vd, vbits, vdstPermute);

                /* *dstp = res */
                vec_st((vector unsigned int) vd, 0, dstp);

                srcp += 4;
                dstp += 4;
                width -= 4;
                vs = voverflow;
            }
            ONE_PIXEL_BLEND((extrawidth), extrawidth);
        }
#undef ONE_PIXEL_BLEND

        srcp += srcskip;
        dstp += dstskip;
    }

}


/* fast RGB888->(A)RGB888 blending */
static void
BlitRGBtoRGBSurfaceAlphaAltivec(SDL_BlitInfo * info)
{
    unsigned alpha = info->a;
    int height = info->dst_h;
    Uint32 *srcp = (Uint32 *) info->src;
    int srcskip = info->src_skip >> 2;
    Uint32 *dstp = (Uint32 *) info->dst;
    int dstskip = info->dst_skip >> 2;
    vector unsigned char mergePermute;
    vector unsigned char valpha;
    vector unsigned char valphamask;
    vector unsigned short v1;
    vector unsigned short v8;

    mergePermute = VEC_MERGE_PERMUTE();
    v1 = vec_splat_u16(1);
    v8 = vec_splat_u16(8);

    /* set the alpha to 255 on the destination surf */
    valphamask = VEC_ALPHA_MASK();

    /* set a vector full of alpha and 255-alpha */
    ((unsigned char *) &valpha)[0] = alpha;
    valpha = vec_splat(valpha, 0);

    while (height--) {
        int width = info->dst_w;
#define ONE_PIXEL_BLEND(condition, widthvar) while ((condition)) { \
            Uint32 s = *srcp; \
            Uint32 d = *dstp; \
            Uint32 s1 = s & 0xff00ff; \
            Uint32 d1 = d & 0xff00ff; \
            d1 = (d1 + ((s1 - d1) * alpha >> 8)) \
                 & 0xff00ff; \
            s &= 0xff00; \
            d &= 0xff00; \
            d = (d + ((s - d) * alpha >> 8)) & 0xff00; \
            *dstp = d1 | d | 0xff000000; \
            ++srcp; \
            ++dstp; \
            widthvar--; \
        }
        ONE_PIXEL_BLEND((UNALIGNED_PTR(dstp)) && (width), width);
        if (width > 0) {
            int extrawidth = (width % 4);
            vector unsigned char valigner = VEC_ALIGNER(srcp);
            vector unsigned char vs = (vector unsigned char) vec_ld(0, srcp);
            width -= extrawidth;
            while (width) {
                vector unsigned char voverflow;
                vector unsigned char vd;

                /* s = *srcp */
                voverflow = (vector unsigned char) vec_ld(15, srcp);
                vs = vec_perm(vs, voverflow, valigner);

                /* d = *dstp */
                vd = (vector unsigned char) vec_ld(0, dstp);

                VEC_MULTIPLY_ALPHA(vs, vd, valpha, mergePermute, v1, v8);

                /* set the alpha channel to full on */
                vd = vec_or(vd, valphamask);

                /* *dstp = res */
                vec_st((vector unsigned int) vd, 0, dstp);

                srcp += 4;
                dstp += 4;
                width -= 4;
                vs = voverflow;
            }
            ONE_PIXEL_BLEND((extrawidth), extrawidth);
        }
#undef ONE_PIXEL_BLEND

        srcp += srcskip;
        dstp += dstskip;
    }
}

#if __MWERKS__
#pragma altivec_model off
#endif
#endif /* SDL_ALTIVEC_BLITTERS */

/* fast RGB888->(A)RGB888 blending with surface alpha=128 special case */
static void
BlitRGBtoRGBSurfaceAlpha128(SDL_BlitInfo * info)
{
    int width = info->dst_w;
    int height = info->dst_h;
    Uint32 *srcp = (Uint32 *) info->src;
    int srcskip = info->src_skip >> 2;
    Uint32 *dstp = (Uint32 *) info->dst;
    int dstskip = info->dst_skip >> 2;

    while (height--) {
	    /* *INDENT-OFF* */
	    DUFFS_LOOP4({
		    Uint32 s = *srcp++;
		    Uint32 d = *dstp;
		    *dstp++ = ((((s & 0x00fefefe) + (d & 0x00fefefe)) >> 1)
			       + (s & d & 0x00010101)) | 0xff000000;
	    }, width);
	    /* *INDENT-ON* */
        srcp += srcskip;
        dstp += dstskip;
    }
}

/* fast RGB888->(A)RGB888 blending with surface alpha */
static void
BlitRGBtoRGBSurfaceAlpha(SDL_BlitInfo * info)
{
    unsigned alpha = info->a;
    if (alpha == 128) {
        BlitRGBtoRGBSurfaceAlpha128(info);
    } else {
        int width = info->dst_w;
        int height = info->dst_h;
        Uint32 *srcp = (Uint32 *) info->src;
        int srcskip = info->src_skip >> 2;
        Uint32 *dstp = (Uint32 *) info->dst;
        int dstskip = info->dst_skip >> 2;
        Uint32 s;
        Uint32 d;
        Uint32 s1;
        Uint32 d1;

        while (height--) {
			/* *INDENT-OFF* */
			DUFFS_LOOP4({
				s = *srcp;
				d = *dstp;
				s1 = s & 0xff00ff;
				d1 = d & 0xff00ff;
				d1 = (d1 + ((s1 - d1) * alpha >> 8))
				     & 0xff00ff;
				s &= 0xff00;
				d &= 0xff00;
				d = (d + ((s - d) * alpha >> 8)) & 0xff00;
				*dstp = d1 | d | 0xff000000;
				++srcp;
				++dstp;
			}, width);
			/* *INDENT-ON* */
            srcp += srcskip;
            dstp += dstskip;
        }
    }
}

/* fast ARGB888->(A)RGB888 blending with pixel alpha */
static void
BlitRGBtoRGBPixelAlpha(SDL_BlitInfo * info)
{
    int width = info->dst_w;
    int height = info->dst_h;
    Uint32 *srcp = (Uint32 *) info->src;
    int srcskip = info->src_skip >> 2;
    Uint32 *dstp = (Uint32 *) info->dst;
    int dstskip = info->dst_skip >> 2;

    while (height--) {
	    /* *INDENT-OFF* */
	    DUFFS_LOOP4({
		Uint32 dalpha;
		Uint32 d;
		Uint32 s1;
		Uint32 d1;
		Uint32 s = *srcp;
		Uint32 alpha = s >> 24;
		/* FIXME: Here we special-case opaque alpha since the
		   compositioning used (>>8 instead of /255) doesn't handle
		   it correctly. Also special-case alpha=0 for speed?
		   Benchmark this! */
		if(alpha) {   
		  if(alpha == SDL_ALPHA_OPAQUE) {
		    *dstp = (s & 0x00ffffff) | (*dstp & 0xff000000);
		  } else {
		    /*
		     * take out the middle component (green), and process
		     * the other two in parallel. One multiply less.
		     */
		    d = *dstp;
		    dalpha = d & 0xff000000;
		    s1 = s & 0xff00ff;
		    d1 = d & 0xff00ff;
		    d1 = (d1 + ((s1 - d1) * alpha >> 8)) & 0xff00ff;
		    s &= 0xff00;
		    d &= 0xff00;
		    d = (d + ((s - d) * alpha >> 8)) & 0xff00;
		    *dstp = d1 | d | dalpha;
		  }
		}
		++srcp;
		++dstp;
	    }, width);
	    /* *INDENT-ON* */
        srcp += srcskip;
        dstp += dstskip;
    }
}

#ifdef __3dNOW__
/* fast (as in MMX with prefetch) ARGB888->(A)RGB888 blending with pixel alpha */
static void
BlitRGBtoRGBPixelAlphaMMX3DNOW(SDL_BlitInfo * info)
{
    int width = info->dst_w;
    int height = info->dst_h;
    Uint32 *srcp = (Uint32 *) info->src;
    int srcskip = info->src_skip >> 2;
    Uint32 *dstp = (Uint32 *) info->dst;
    int dstskip = info->dst_skip >> 2;
    SDL_PixelFormat *sf = info->src_fmt;
    Uint32 chanmask = sf->Rmask | sf->Gmask | sf->Bmask;
    Uint32 amask = sf->Amask;
    Uint32 ashift = sf->Ashift;
    Uint64 multmask;

    __m64 src1, dst1, mm_alpha, mm_zero, dmask;

    mm_zero = _mm_setzero_si64();       /* 0 -> mm_zero */
    multmask = 0xFFFF;
    multmask <<= (ashift * 2);
    multmask = ~multmask;
    dmask = *(__m64 *) & multmask;      /* dst alpha mask -> dmask */

    while (height--) {
	    /* *INDENT-OFF* */
	    DUFFS_LOOP4({
		Uint32 alpha;

		_m_prefetch(srcp + 16);
		_m_prefetch(dstp + 16);

		alpha = *srcp & amask;
		if (alpha == 0) {
			/* do nothing */
		} else if (alpha == amask) {
			/* copy RGB, keep dst alpha */
			*dstp = (*srcp & chanmask) | (*dstp & ~chanmask);
		} else {
			src1 = _mm_cvtsi32_si64(*srcp); /* src(ARGB) -> src1 (0000ARGB)*/
			src1 = _mm_unpacklo_pi8(src1, mm_zero); /* 0A0R0G0B -> src1 */

			dst1 = _mm_cvtsi32_si64(*dstp); /* dst(ARGB) -> dst1 (0000ARGB)*/
			dst1 = _mm_unpacklo_pi8(dst1, mm_zero); /* 0A0R0G0B -> dst1 */

			mm_alpha = _mm_cvtsi32_si64(alpha); /* alpha -> mm_alpha (0000000A) */
			mm_alpha = _mm_srli_si64(mm_alpha, ashift); /* mm_alpha >> ashift -> mm_alpha(0000000A) */
			mm_alpha = _mm_unpacklo_pi16(mm_alpha, mm_alpha); /* 00000A0A -> mm_alpha */
			mm_alpha = _mm_unpacklo_pi32(mm_alpha, mm_alpha); /* 0A0A0A0A -> mm_alpha */
			mm_alpha = _mm_and_si64(mm_alpha, dmask); /* 000A0A0A -> mm_alpha, preserve dst alpha on add */

			/* blend */		    
			src1 = _mm_sub_pi16(src1, dst1);/* src - dst -> src1 */
			src1 = _mm_mullo_pi16(src1, mm_alpha); /* (src - dst) * alpha -> src1 */
			src1 = _mm_srli_pi16(src1, 8); /* src1 >> 8 -> src1(000R0G0B) */
			dst1 = _mm_add_pi8(src1, dst1); /* src1 + dst1(dst) -> dst1(0A0R0G0B) */
			dst1 = _mm_packs_pu16(dst1, mm_zero);  /* 0000ARGB -> dst1 */
			
			*dstp = _mm_cvtsi64_si32(dst1); /* dst1 -> pixel */
		}
		++srcp;
		++dstp;
	    }, width);
	    /* *INDENT-ON* */
        srcp += srcskip;
        dstp += dstskip;
    }
    _mm_empty();
}

#endif /* __MMX__ */

/* 16bpp special case for per-surface alpha=50%: blend 2 pixels in parallel */

/* blend a single 16 bit pixel at 50% */
#define BLEND16_50(d, s, mask)						\
	((((s & mask) + (d & mask)) >> 1) + (s & d & (~mask & 0xffff)))

/* blend two 16 bit pixels at 50% */
#define BLEND2x16_50(d, s, mask)					     \
	(((s & (mask | mask << 16)) >> 1) + ((d & (mask | mask << 16)) >> 1) \
	 + (s & d & (~(mask | mask << 16))))

static void
Blit16to16SurfaceAlpha128(SDL_BlitInfo * info, Uint16 mask)
{
    int width = info->dst_w;
    int height = info->dst_h;
    Uint16 *srcp = (Uint16 *) info->src;
    int srcskip = info->src_skip >> 1;
    Uint16 *dstp = (Uint16 *) info->dst;
    int dstskip = info->dst_skip >> 1;

    while (height--) {
        if (((uintptr_t) srcp ^ (uintptr_t) dstp) & 2) {
            /*
             * Source and destination not aligned, pipeline it.
             * This is mostly a win for big blits but no loss for
             * small ones
             */
            Uint32 prev_sw;
            int w = width;

            /* handle odd destination */
            if ((uintptr_t) dstp & 2) {
                Uint16 d = *dstp, s = *srcp;
                *dstp = BLEND16_50(d, s, mask);
                dstp++;
                srcp++;
                w--;
            }
            srcp++;             /* srcp is now 32-bit aligned */

            /* bootstrap pipeline with first halfword */
            prev_sw = ((Uint32 *) srcp)[-1];

            while (w > 1) {
                Uint32 sw, dw, s;
                sw = *(Uint32 *) srcp;
                dw = *(Uint32 *) dstp;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
                s = (prev_sw << 16) + (sw >> 16);
#else
                s = (prev_sw >> 16) + (sw << 16);
#endif
                prev_sw = sw;
                *(Uint32 *) dstp = BLEND2x16_50(dw, s, mask);
                dstp += 2;
                srcp += 2;
                w -= 2;
            }

            /* final pixel if any */
            if (w) {
                Uint16 d = *dstp, s;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
                s = (Uint16) prev_sw;
#else
                s = (Uint16) (prev_sw >> 16);
#endif
                *dstp = BLEND16_50(d, s, mask);
                srcp++;
                dstp++;
            }
            srcp += srcskip - 1;
            dstp += dstskip;
        } else {
            /* source and destination are aligned */
            int w = width;

            /* first odd pixel? */
            if ((uintptr_t) srcp & 2) {
                Uint16 d = *dstp, s = *srcp;
                *dstp = BLEND16_50(d, s, mask);
                srcp++;
                dstp++;
                w--;
            }
            /* srcp and dstp are now 32-bit aligned */

            while (w > 1) {
                Uint32 sw = *(Uint32 *) srcp;
                Uint32 dw = *(Uint32 *) dstp;
                *(Uint32 *) dstp = BLEND2x16_50(dw, sw, mask);
                srcp += 2;
                dstp += 2;
                w -= 2;
            }

            /* last odd pixel? */
            if (w) {
                Uint16 d = *dstp, s = *srcp;
                *dstp = BLEND16_50(d, s, mask);
                srcp++;
                dstp++;
            }
            srcp += srcskip;
            dstp += dstskip;
        }
    }
}

#ifdef __MMX__

/* fast RGB565->RGB565 blending with surface alpha */
static void
Blit565to565SurfaceAlphaMMX(SDL_BlitInfo * info)
{
    unsigned alpha = info->a;
    if (alpha == 128) {
        Blit16to16SurfaceAlpha128(info, 0xf7de);
    } else {
        int width = info->dst_w;
        int height = info->dst_h;
        Uint16 *srcp = (Uint16 *) info->src;
        int srcskip = info->src_skip >> 1;
        Uint16 *dstp = (Uint16 *) info->dst;
        int dstskip = info->dst_skip >> 1;
        Uint32 s, d;

        __m64 src1, dst1, src2, dst2, gmask, bmask, mm_res, mm_alpha;

        alpha &= ~(1 + 2 + 4);  /* cut alpha to get the exact same behaviour */
        mm_alpha = _mm_set_pi32(0, alpha);      /* 0000000A -> mm_alpha */
        alpha >>= 3;            /* downscale alpha to 5 bits */

        mm_alpha = _mm_unpacklo_pi16(mm_alpha, mm_alpha);       /* 00000A0A -> mm_alpha */
        mm_alpha = _mm_unpacklo_pi32(mm_alpha, mm_alpha);       /* 0A0A0A0A -> mm_alpha */
        /* position alpha to allow for mullo and mulhi on diff channels
           to reduce the number of operations */
        mm_alpha = _mm_slli_si64(mm_alpha, 3);

        /* Setup the 565 color channel masks */
        gmask = _mm_set_pi32(0x07E007E0, 0x07E007E0);   /* MASKGREEN -> gmask */
        bmask = _mm_set_pi32(0x001F001F, 0x001F001F);   /* MASKBLUE -> bmask */

        while (height--) {
			/* *INDENT-OFF* */
			DUFFS_LOOP_124(
			{
				s = *srcp++;
				d = *dstp;
				/*
				 * shift out the middle component (green) to
				 * the high 16 bits, and process all three RGB
				 * components at the same time.
				 */
				s = (s | s << 16) & 0x07e0f81f;
				d = (d | d << 16) & 0x07e0f81f;
				d += (s - d) * alpha >> 5;
				d &= 0x07e0f81f;
				*dstp++ = (Uint16)(d | d >> 16);
			},{
				s = *srcp++;
				d = *dstp;
				/*
				 * shift out the middle component (green) to
				 * the high 16 bits, and process all three RGB
				 * components at the same time.
				 */
				s = (s | s << 16) & 0x07e0f81f;
				d = (d | d << 16) & 0x07e0f81f;
				d += (s - d) * alpha >> 5;
				d &= 0x07e0f81f;
				*dstp++ = (Uint16)(d | d >> 16);
				s = *srcp++;
				d = *dstp;
				/*
				 * shift out the middle component (green) to
				 * the high 16 bits, and process all three RGB
				 * components at the same time.
				 */
				s = (s | s << 16) & 0x07e0f81f;
				d = (d | d << 16) & 0x07e0f81f;
				d += (s - d) * alpha >> 5;
				d &= 0x07e0f81f;
				*dstp++ = (Uint16)(d | d >> 16);
			},{
				src1 = *(__m64*)srcp; /* 4 src pixels -> src1 */
				dst1 = *(__m64*)dstp; /* 4 dst pixels -> dst1 */

				/* red */
				src2 = src1;
				src2 = _mm_srli_pi16(src2, 11); /* src2 >> 11 -> src2 [000r 000r 000r 000r] */

				dst2 = dst1;
				dst2 = _mm_srli_pi16(dst2, 11); /* dst2 >> 11 -> dst2 [000r 000r 000r 000r] */

				/* blend */
				src2 = _mm_sub_pi16(src2, dst2);/* src - dst -> src2 */
				src2 = _mm_mullo_pi16(src2, mm_alpha); /* src2 * alpha -> src2 */
				src2 = _mm_srli_pi16(src2, 11); /* src2 >> 11 -> src2 */
				dst2 = _mm_add_pi16(src2, dst2); /* src2 + dst2 -> dst2 */
				dst2 = _mm_slli_pi16(dst2, 11); /* dst2 << 11 -> dst2 */

				mm_res = dst2; /* RED -> mm_res */

				/* green -- process the bits in place */
				src2 = src1;
				src2 = _mm_and_si64(src2, gmask); /* src & MASKGREEN -> src2 */

				dst2 = dst1;
				dst2 = _mm_and_si64(dst2, gmask); /* dst & MASKGREEN -> dst2 */

				/* blend */
				src2 = _mm_sub_pi16(src2, dst2);/* src - dst -> src2 */
				src2 = _mm_mulhi_pi16(src2, mm_alpha); /* src2 * alpha -> src2 */
				src2 = _mm_slli_pi16(src2, 5); /* src2 << 5 -> src2 */
				dst2 = _mm_add_pi16(src2, dst2); /* src2 + dst2 -> dst2 */

				mm_res = _mm_or_si64(mm_res, dst2); /* RED | GREEN -> mm_res */

				/* blue */
				src2 = src1;
				src2 = _mm_and_si64(src2, bmask); /* src & MASKBLUE -> src2[000b 000b 000b 000b] */

				dst2 = dst1;
				dst2 = _mm_and_si64(dst2, bmask); /* dst & MASKBLUE -> dst2[000b 000b 000b 000b] */

				/* blend */
				src2 = _mm_sub_pi16(src2, dst2);/* src - dst -> src2 */
				src2 = _mm_mullo_pi16(src2, mm_alpha); /* src2 * alpha -> src2 */
				src2 = _mm_srli_pi16(src2, 11); /* src2 >> 11 -> src2 */
				dst2 = _mm_add_pi16(src2, dst2); /* src2 + dst2 -> dst2 */
				dst2 = _mm_and_si64(dst2, bmask); /* dst2 & MASKBLUE -> dst2 */

				mm_res = _mm_or_si64(mm_res, dst2); /* RED | GREEN | BLUE -> mm_res */

				*(__m64*)dstp = mm_res; /* mm_res -> 4 dst pixels */

				srcp += 4;
				dstp += 4;
			}, width);
			/* *INDENT-ON* */
            srcp += srcskip;
            dstp += dstskip;
        }
        _mm_empty();
    }
}

/* fast RGB555->RGB555 blending with surface alpha */
static void
Blit555to555SurfaceAlphaMMX(SDL_BlitInfo * info)
{
    unsigned alpha = info->a;
    if (alpha == 128) {
        Blit16to16SurfaceAlpha128(info, 0xfbde);
    } else {
        int width = info->dst_w;
        int height = info->dst_h;
        Uint16 *srcp = (Uint16 *) info->src;
        int srcskip = info->src_skip >> 1;
        Uint16 *dstp = (Uint16 *) info->dst;
        int dstskip = info->dst_skip >> 1;
        Uint32 s, d;

        __m64 src1, dst1, src2, dst2, rmask, gmask, bmask, mm_res, mm_alpha;

        alpha &= ~(1 + 2 + 4);  /* cut alpha to get the exact same behaviour */
        mm_alpha = _mm_set_pi32(0, alpha);      /* 0000000A -> mm_alpha */
        alpha >>= 3;            /* downscale alpha to 5 bits */

        mm_alpha = _mm_unpacklo_pi16(mm_alpha, mm_alpha);       /* 00000A0A -> mm_alpha */
        mm_alpha = _mm_unpacklo_pi32(mm_alpha, mm_alpha);       /* 0A0A0A0A -> mm_alpha */
        /* position alpha to allow for mullo and mulhi on diff channels
           to reduce the number of operations */
        mm_alpha = _mm_slli_si64(mm_alpha, 3);

        /* Setup the 555 color channel masks */
        rmask = _mm_set_pi32(0x7C007C00, 0x7C007C00);   /* MASKRED -> rmask */
        gmask = _mm_set_pi32(0x03E003E0, 0x03E003E0);   /* MASKGREEN -> gmask */
        bmask = _mm_set_pi32(0x001F001F, 0x001F001F);   /* MASKBLUE -> bmask */

        while (height--) {
			/* *INDENT-OFF* */
			DUFFS_LOOP_124(
			{
				s = *srcp++;
				d = *dstp;
				/*
				 * shift out the middle component (green) to
				 * the high 16 bits, and process all three RGB
				 * components at the same time.
				 */
				s = (s | s << 16) & 0x03e07c1f;
				d = (d | d << 16) & 0x03e07c1f;
				d += (s - d) * alpha >> 5;
				d &= 0x03e07c1f;
				*dstp++ = (Uint16)(d | d >> 16);
			},{
				s = *srcp++;
				d = *dstp;
				/*
				 * shift out the middle component (green) to
				 * the high 16 bits, and process all three RGB
				 * components at the same time.
				 */
				s = (s | s << 16) & 0x03e07c1f;
				d = (d | d << 16) & 0x03e07c1f;
				d += (s - d) * alpha >> 5;
				d &= 0x03e07c1f;
				*dstp++ = (Uint16)(d | d >> 16);
			        s = *srcp++;
				d = *dstp;
				/*
				 * shift out the middle component (green) to
				 * the high 16 bits, and process all three RGB
				 * components at the same time.
				 */
				s = (s | s << 16) & 0x03e07c1f;
				d = (d | d << 16) & 0x03e07c1f;
				d += (s - d) * alpha >> 5;
				d &= 0x03e07c1f;
				*dstp++ = (Uint16)(d | d >> 16);
			},{
				src1 = *(__m64*)srcp; /* 4 src pixels -> src1 */
				dst1 = *(__m64*)dstp; /* 4 dst pixels -> dst1 */

				/* red -- process the bits in place */
				src2 = src1;
				src2 = _mm_and_si64(src2, rmask); /* src & MASKRED -> src2 */

				dst2 = dst1;
				dst2 = _mm_and_si64(dst2, rmask); /* dst & MASKRED -> dst2 */

				/* blend */
				src2 = _mm_sub_pi16(src2, dst2);/* src - dst -> src2 */
				src2 = _mm_mulhi_pi16(src2, mm_alpha); /* src2 * alpha -> src2 */
				src2 = _mm_slli_pi16(src2, 5); /* src2 << 5 -> src2 */
				dst2 = _mm_add_pi16(src2, dst2); /* src2 + dst2 -> dst2 */
				dst2 = _mm_and_si64(dst2, rmask); /* dst2 & MASKRED -> dst2 */

				mm_res = dst2; /* RED -> mm_res */
				
				/* green -- process the bits in place */
				src2 = src1;
				src2 = _mm_and_si64(src2, gmask); /* src & MASKGREEN -> src2 */

				dst2 = dst1;
				dst2 = _mm_and_si64(dst2, gmask); /* dst & MASKGREEN -> dst2 */

				/* blend */
				src2 = _mm_sub_pi16(src2, dst2);/* src - dst -> src2 */
				src2 = _mm_mulhi_pi16(src2, mm_alpha); /* src2 * alpha -> src2 */
				src2 = _mm_slli_pi16(src2, 5); /* src2 << 5 -> src2 */
				dst2 = _mm_add_pi16(src2, dst2); /* src2 + dst2 -> dst2 */

				mm_res = _mm_or_si64(mm_res, dst2); /* RED | GREEN -> mm_res */

				/* blue */
				src2 = src1; /* src -> src2 */
				src2 = _mm_and_si64(src2, bmask); /* src & MASKBLUE -> src2[000b 000b 000b 000b] */

				dst2 = dst1; /* dst -> dst2 */
				dst2 = _mm_and_si64(dst2, bmask); /* dst & MASKBLUE -> dst2[000b 000b 000b 000b] */

				/* blend */
				src2 = _mm_sub_pi16(src2, dst2);/* src - dst -> src2 */
				src2 = _mm_mullo_pi16(src2, mm_alpha); /* src2 * alpha -> src2 */
				src2 = _mm_srli_pi16(src2, 11); /* src2 >> 11 -> src2 */
				dst2 = _mm_add_pi16(src2, dst2); /* src2 + dst2 -> dst2 */
				dst2 = _mm_and_si64(dst2, bmask); /* dst2 & MASKBLUE -> dst2 */

				mm_res = _mm_or_si64(mm_res, dst2); /* RED | GREEN | BLUE -> mm_res */

				*(__m64*)dstp = mm_res; /* mm_res -> 4 dst pixels */

				srcp += 4;
				dstp += 4;
			}, width);
			/* *INDENT-ON* */
            srcp += srcskip;
            dstp += dstskip;
        }
        _mm_empty();
    }
}

#endif /* __MMX__ */

/* fast RGB565->RGB565 blending with surface alpha */
static void
Blit565to565SurfaceAlpha(SDL_BlitInfo * info)
{
    unsigned alpha = info->a;
    if (alpha == 128) {
        Blit16to16SurfaceAlpha128(info, 0xf7de);
    } else {
        int width = info->dst_w;
        int height = info->dst_h;
        Uint16 *srcp = (Uint16 *) info->src;
        int srcskip = info->src_skip >> 1;
        Uint16 *dstp = (Uint16 *) info->dst;
        int dstskip = info->dst_skip >> 1;
        alpha >>= 3;            /* downscale alpha to 5 bits */

        while (height--) {
			/* *INDENT-OFF* */
			DUFFS_LOOP4({
				Uint32 s = *srcp++;
				Uint32 d = *dstp;
				/*
				 * shift out the middle component (green) to
				 * the high 16 bits, and process all three RGB
				 * components at the same time.
				 */
				s = (s | s << 16) & 0x07e0f81f;
				d = (d | d << 16) & 0x07e0f81f;
				d += (s - d) * alpha >> 5;
				d &= 0x07e0f81f;
				*dstp++ = (Uint16)(d | d >> 16);
			}, width);
			/* *INDENT-ON* */
            srcp += srcskip;
            dstp += dstskip;
        }
    }
}

/* fast RGB555->RGB555 blending with surface alpha */
static void
Blit555to555SurfaceAlpha(SDL_BlitInfo * info)
{
    unsigned alpha = info->a;   /* downscale alpha to 5 bits */
    if (alpha == 128) {
        Blit16to16SurfaceAlpha128(info, 0xfbde);
    } else {
        int width = info->dst_w;
        int height = info->dst_h;
        Uint16 *srcp = (Uint16 *) info->src;
        int srcskip = info->src_skip >> 1;
        Uint16 *dstp = (Uint16 *) info->dst;
        int dstskip = info->dst_skip >> 1;
        alpha >>= 3;            /* downscale alpha to 5 bits */

        while (height--) {
			/* *INDENT-OFF* */
			DUFFS_LOOP4({
				Uint32 s = *srcp++;
				Uint32 d = *dstp;
				/*
				 * shift out the middle component (green) to
				 * the high 16 bits, and process all three RGB
				 * components at the same time.
				 */
				s = (s | s << 16) & 0x03e07c1f;
				d = (d | d << 16) & 0x03e07c1f;
				d += (s - d) * alpha >> 5;
				d &= 0x03e07c1f;
				*dstp++ = (Uint16)(d | d >> 16);
			}, width);
			/* *INDENT-ON* */
            srcp += srcskip;
            dstp += dstskip;
        }
    }
}

/* fast ARGB8888->RGB565 blending with pixel alpha */
static void
BlitARGBto565PixelAlpha(SDL_BlitInfo * info)
{
    int width = info->dst_w;
    int height = info->dst_h;
    Uint32 *srcp = (Uint32 *) info->src;
    int srcskip = info->src_skip >> 2;
    Uint16 *dstp = (Uint16 *) info->dst;
    int dstskip = info->dst_skip >> 1;

    while (height--) {
	    /* *INDENT-OFF* */
	    DUFFS_LOOP4({
		Uint32 s = *srcp;
		unsigned alpha = s >> 27; /* downscale alpha to 5 bits */
		/* FIXME: Here we special-case opaque alpha since the
		   compositioning used (>>8 instead of /255) doesn't handle
		   it correctly. Also special-case alpha=0 for speed?
		   Benchmark this! */
		if(alpha) {   
		  if(alpha == (SDL_ALPHA_OPAQUE >> 3)) {
		    *dstp = (Uint16)((s >> 8 & 0xf800) + (s >> 5 & 0x7e0) + (s >> 3  & 0x1f));
		  } else {
		    Uint32 d = *dstp;
		    /*
		     * convert source and destination to G0RAB65565
		     * and blend all components at the same time
		     */
		    s = ((s & 0xfc00) << 11) + (s >> 8 & 0xf800)
		      + (s >> 3 & 0x1f);
		    d = (d | d << 16) & 0x07e0f81f;
		    d += (s - d) * alpha >> 5;
		    d &= 0x07e0f81f;
		    *dstp = (Uint16)(d | d >> 16);
		  }
		}
		srcp++;
		dstp++;
	    }, width);
	    /* *INDENT-ON* */
        srcp += srcskip;
        dstp += dstskip;
    }
}

/* fast ARGB8888->RGB555 blending with pixel alpha */
static void
BlitARGBto555PixelAlpha(SDL_BlitInfo * info)
{
    int width = info->dst_w;
    int height = info->dst_h;
    Uint32 *srcp = (Uint32 *) info->src;
    int srcskip = info->src_skip >> 2;
    Uint16 *dstp = (Uint16 *) info->dst;
    int dstskip = info->dst_skip >> 1;

    while (height--) {
	    /* *INDENT-OFF* */
	    DUFFS_LOOP4({
		unsigned alpha;
		Uint32 s = *srcp;
		alpha = s >> 27; /* downscale alpha to 5 bits */
		/* FIXME: Here we special-case opaque alpha since the
		   compositioning used (>>8 instead of /255) doesn't handle
		   it correctly. Also special-case alpha=0 for speed?
		   Benchmark this! */
		if(alpha) {   
		  if(alpha == (SDL_ALPHA_OPAQUE >> 3)) {
		    *dstp = (Uint16)((s >> 9 & 0x7c00) + (s >> 6 & 0x3e0) + (s >> 3  & 0x1f));
		  } else {
		    Uint32 d = *dstp;
		    /*
		     * convert source and destination to G0RAB65565
		     * and blend all components at the same time
		     */
		    s = ((s & 0xf800) << 10) + (s >> 9 & 0x7c00)
		      + (s >> 3 & 0x1f);
		    d = (d | d << 16) & 0x03e07c1f;
		    d += (s - d) * alpha >> 5;
		    d &= 0x03e07c1f;
		    *dstp = (Uint16)(d | d >> 16);
		  }
		}
		srcp++;
		dstp++;
	    }, width);
	    /* *INDENT-ON* */
        srcp += srcskip;
        dstp += dstskip;
    }
}

/* General (slow) N->N blending with per-surface alpha */
static void
BlitNtoNSurfaceAlpha(SDL_BlitInfo * info)
{
    int width = info->dst_w;
    int height = info->dst_h;
    Uint8 *src = info->src;
    int srcskip = info->src_skip;
    Uint8 *dst = info->dst;
    int dstskip = info->dst_skip;
    SDL_PixelFormat *srcfmt = info->src_fmt;
    SDL_PixelFormat *dstfmt = info->dst_fmt;
    int srcbpp = srcfmt->BytesPerPixel;
    int dstbpp = dstfmt->BytesPerPixel;
    unsigned sA = info->a;
    unsigned dA = dstfmt->Amask ? SDL_ALPHA_OPAQUE : 0;

    if (sA) {
        while (height--) {
	    /* *INDENT-OFF* */
	    DUFFS_LOOP4(
	    {
		Uint32 Pixel;
		unsigned sR;
		unsigned sG;
		unsigned sB;
		unsigned dR;
		unsigned dG;
		unsigned dB;
		DISEMBLE_RGB(src, srcbpp, srcfmt, Pixel, sR, sG, sB);
		DISEMBLE_RGB(dst, dstbpp, dstfmt, Pixel, dR, dG, dB);
		ALPHA_BLEND(sR, sG, sB, sA, dR, dG, dB);
		ASSEMBLE_RGBA(dst, dstbpp, dstfmt, dR, dG, dB, dA);
		src += srcbpp;
		dst += dstbpp;
	    },
	    width);
	    /* *INDENT-ON* */
            src += srcskip;
            dst += dstskip;
        }
    }
}

/* General (slow) colorkeyed N->N blending with per-surface alpha */
static void
BlitNtoNSurfaceAlphaKey(SDL_BlitInfo * info)
{
    int width = info->dst_w;
    int height = info->dst_h;
    Uint8 *src = info->src;
    int srcskip = info->src_skip;
    Uint8 *dst = info->dst;
    int dstskip = info->dst_skip;
    SDL_PixelFormat *srcfmt = info->src_fmt;
    SDL_PixelFormat *dstfmt = info->dst_fmt;
    Uint32 ckey = info->colorkey;
    int srcbpp = srcfmt->BytesPerPixel;
    int dstbpp = dstfmt->BytesPerPixel;
    unsigned sA = info->a;
    unsigned dA = dstfmt->Amask ? SDL_ALPHA_OPAQUE : 0;

    while (height--) {
	    /* *INDENT-OFF* */
	    DUFFS_LOOP4(
	    {
		Uint32 Pixel;
		unsigned sR;
		unsigned sG;
		unsigned sB;
		unsigned dR;
		unsigned dG;
		unsigned dB;
		RETRIEVE_RGB_PIXEL(src, srcbpp, Pixel);
		if(sA && Pixel != ckey) {
		    RGB_FROM_PIXEL(Pixel, srcfmt, sR, sG, sB);
		    DISEMBLE_RGB(dst, dstbpp, dstfmt, Pixel, dR, dG, dB);
		    ALPHA_BLEND(sR, sG, sB, sA, dR, dG, dB);
		    ASSEMBLE_RGBA(dst, dstbpp, dstfmt, dR, dG, dB, dA);
		}
		src += srcbpp;
		dst += dstbpp;
	    },
	    width);
	    /* *INDENT-ON* */
        src += srcskip;
        dst += dstskip;
    }
}

/* General (slow) N->N blending with pixel alpha */
static void
BlitNtoNPixelAlpha(SDL_BlitInfo * info)
{
    int width = info->dst_w;
    int height = info->dst_h;
    Uint8 *src = info->src;
    int srcskip = info->src_skip;
    Uint8 *dst = info->dst;
    int dstskip = info->dst_skip;
    SDL_PixelFormat *srcfmt = info->src_fmt;
    SDL_PixelFormat *dstfmt = info->dst_fmt;

    int srcbpp;
    int dstbpp;

    /* Set up some basic variables */
    srcbpp = srcfmt->BytesPerPixel;
    dstbpp = dstfmt->BytesPerPixel;

    while (height--) {
	    /* *INDENT-OFF* */
	    DUFFS_LOOP4(
	    {
		Uint32 Pixel;
		unsigned sR;
		unsigned sG;
		unsigned sB;
		unsigned dR;
		unsigned dG;
		unsigned dB;
		unsigned sA;
		unsigned dA;
		DISEMBLE_RGBA(src, srcbpp, srcfmt, Pixel, sR, sG, sB, sA);
		if(sA) {
		  DISEMBLE_RGBA(dst, dstbpp, dstfmt, Pixel, dR, dG, dB, dA);
		  ALPHA_BLEND(sR, sG, sB, sA, dR, dG, dB);
		  ASSEMBLE_RGBA(dst, dstbpp, dstfmt, dR, dG, dB, dA);
		}
		src += srcbpp;
		dst += dstbpp;
	    },
	    width);
	    /* *INDENT-ON* */
        src += srcskip;
        dst += dstskip;
    }
}


SDL_BlitFunc
SDL_CalculateBlitA(SDL_Surface * surface)
{
    SDL_PixelFormat *sf = surface->format;
    SDL_PixelFormat *df = surface->map->dst->format;

    switch (surface->map->info.flags & ~SDL_COPY_RLE_MASK) {
    case SDL_COPY_BLEND:
        /* Per-pixel alpha blits */
        switch (df->BytesPerPixel) {
        case 1:
            return BlitNto1PixelAlpha;

        case 2:
#if SDL_ALTIVEC_BLITTERS
            if (sf->BytesPerPixel == 4
                && df->Gmask == 0x7e0 && df->Bmask == 0x1f
                && SDL_HasAltiVec())
                return Blit32to565PixelAlphaAltivec;
            else
#endif
                if (sf->BytesPerPixel == 4 && sf->Amask == 0xff000000
                    && sf->Gmask == 0xff00
                    && ((sf->Rmask == 0xff && df->Rmask == 0x1f)
                        || (sf->Bmask == 0xff && df->Bmask == 0x1f))) {
                if (df->Gmask == 0x7e0)
                    return BlitARGBto565PixelAlpha;
                else if (df->Gmask == 0x3e0)
                    return BlitARGBto555PixelAlpha;
            }
            return BlitNtoNPixelAlpha;

        case 4:
            if (sf->Rmask == df->Rmask
                && sf->Gmask == df->Gmask
                && sf->Bmask == df->Bmask && sf->BytesPerPixel == 4) {
#if defined(__MMX__) || defined(__3dNOW__)
                if (sf->Rshift % 8 == 0
                    && sf->Gshift % 8 == 0
                    && sf->Bshift % 8 == 0
                    && sf->Ashift % 8 == 0 && sf->Aloss == 0) {
#ifdef __3dNOW__
                    if (SDL_Has3DNow())
                        return BlitRGBtoRGBPixelAlphaMMX3DNOW;
#endif
#ifdef __MMX__
                    if (SDL_HasMMX())
                        return BlitRGBtoRGBPixelAlphaMMX;
#endif
                }
#endif /* __MMX__ || __3dNOW__ */
                if (sf->Amask == 0xff000000) {
#if SDL_ALTIVEC_BLITTERS
                    if (SDL_HasAltiVec())
                        return BlitRGBtoRGBPixelAlphaAltivec;
#endif
                    return BlitRGBtoRGBPixelAlpha;
                }
            }
#if SDL_ALTIVEC_BLITTERS
            if (sf->Amask && sf->BytesPerPixel == 4 && SDL_HasAltiVec())
                return Blit32to32PixelAlphaAltivec;
            else
#endif
                return BlitNtoNPixelAlpha;

        case 3:
        default:
            return BlitNtoNPixelAlpha;
        }
        break;

    case SDL_COPY_MODULATE_ALPHA | SDL_COPY_BLEND:
        if (sf->Amask == 0) {
            /* Per-surface alpha blits */
            switch (df->BytesPerPixel) {
            case 1:
                return BlitNto1SurfaceAlpha;

            case 2:
                if (surface->map->identity) {
                    if (df->Gmask == 0x7e0) {
#ifdef __MMX__
                        if (SDL_HasMMX())
                            return Blit565to565SurfaceAlphaMMX;
                        else
#endif
                            return Blit565to565SurfaceAlpha;
                    } else if (df->Gmask == 0x3e0) {
#ifdef __MMX__
                        if (SDL_HasMMX())
                            return Blit555to555SurfaceAlphaMMX;
                        else
#endif
                            return Blit555to555SurfaceAlpha;
                    }
                }
                return BlitNtoNSurfaceAlpha;

            case 4:
                if (sf->Rmask == df->Rmask
                    && sf->Gmask == df->Gmask
                    && sf->Bmask == df->Bmask && sf->BytesPerPixel == 4) {
#ifdef __MMX__
                    if (sf->Rshift % 8 == 0
                        && sf->Gshift % 8 == 0
                        && sf->Bshift % 8 == 0 && SDL_HasMMX())
                        return BlitRGBtoRGBSurfaceAlphaMMX;
#endif
                    if ((sf->Rmask | sf->Gmask | sf->Bmask) == 0xffffff) {
#if SDL_ALTIVEC_BLITTERS
                        if (SDL_HasAltiVec())
                            return BlitRGBtoRGBSurfaceAlphaAltivec;
#endif
                        return BlitRGBtoRGBSurfaceAlpha;
                    }
                }
#if SDL_ALTIVEC_BLITTERS
                if ((sf->BytesPerPixel == 4) && SDL_HasAltiVec())
                    return Blit32to32SurfaceAlphaAltivec;
                else
#endif
                    return BlitNtoNSurfaceAlpha;

            case 3:
            default:
                return BlitNtoNSurfaceAlpha;
            }
        }
        break;

    case SDL_COPY_COLORKEY | SDL_COPY_MODULATE_ALPHA | SDL_COPY_BLEND:
        if (sf->Amask == 0) {
            if (df->BytesPerPixel == 1)
                return BlitNto1SurfaceAlphaKey;
            else
#if SDL_ALTIVEC_BLITTERS
            if (sf->BytesPerPixel == 4 && df->BytesPerPixel == 4 &&
                    SDL_HasAltiVec())
                return Blit32to32SurfaceAlphaKeyAltivec;
            else
#endif
                return BlitNtoNSurfaceAlphaKey;
        }
        break;
    }

    return NULL;
}

/* vi: set ts=4 sw=4 expandtab: */
