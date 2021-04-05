#include <stdio.h>
#include <stdint.h>
#include "scaler.h"

#define AVERAGE(z, x) ((((z) & 0xF7DEF7DE) >> 1) + (((x) & 0xF7DEF7DE) >> 1))
#define AVERAGEHI(AB) ((((AB) & 0xF7DE0000) >> 1) + (((AB) & 0xF7DE) << 15))
#define AVERAGELO(CD) ((((CD) & 0xF7DE) >> 1) + (((CD) & 0xF7DE0000) >> 17))

// Support math
#define Half(A) (((A) >> 1) & 0x7BEF)
#define Quarter(A) (((A) >> 2) & 0x39E7)
// Error correction expressions to piece back the lower bits together
#define RestHalf(A) ((A) & 0x0821)
#define RestQuarter(A) ((A) & 0x1863)

// Error correction expressions for quarters of pixels
#define Corr1_3(A, B)     Quarter(RestQuarter(A) + (RestHalf(B) << 1) + RestQuarter(B))
#define Corr3_1(A, B)     Quarter((RestHalf(A) << 1) + RestQuarter(A) + RestQuarter(B))

// Error correction expressions for halves
#define Corr1_1(A, B)     ((A) & (B) & 0x0821)

// Quarters
#define Weight1_3(A, B)   (Quarter(A) + Half(B) + Quarter(B) + Corr1_3(A, B))
#define Weight3_1(A, B)   (Half(A) + Quarter(A) + Quarter(B) + Corr3_1(A, B))

// Halves
#define Weight1_1(A, B)   (Half(A) + Half(B) + Corr1_1(A, B))

#define AVERAGE16(c1, c2) (((c1) + (c2) + (((c1) ^ (c2)) & 0x0821))>>1)  //More accurate

void upscale_240x208_to_320x240(uint16_t *dst, uint16_t *src)
{
    int Eh = 0;
    int dh = 8;
    int width = 320;
    int vf = 0;

    for (int y = 0; y < 240; y++)
    {
        int source = dh * width + 8;
        for (int x = 0; x < 320/4; x++)
        {
            register uint16_t a, b, c;

            a = src[source];
            b = src[source+1];
            c = src[source+2];

            if(vf == 1){
                a = AVERAGE16(a, src[source+width]);
                b = AVERAGE16(b, src[source+width+1]);
                c = AVERAGE16(c, src[source+width+2]);
            }
            *dst++ = a;
            *dst++ = (AVERAGE16(a,b) & 0b0000000000011111) | (b & 0b1111111111100000);
            *dst++ = (b & 0b0000011111111111) | (AVERAGE16(b,c) & 0b1111100000000000);
            *dst++ = c;
            source+=3;

        }
        Eh += 208;
        if(Eh >= 240) {
            Eh -= 240;
            dh++;
            vf = 0;
        }
        else
            vf = 1;
    }
}


void upscale_256x240_to_320x240_bilinearish(uint32_t* restrict dst, uint32_t* restrict src, uint_fast16_t width, uint_fast16_t height)
{
	uint16_t* Src16 = (uint16_t*) src;
	uint16_t* Dst16 = (uint16_t*) dst;
	// There are 64 blocks of 4 pixels horizontally, and 239 of 1 vertically.
	// Each block of 4x1 becomes 5x1.
	uint32_t BlockX, BlockY;
	register uint16_t _1,_2,_3,_4;
	register uint16_t* BlockSrc;
	register uint16_t* BlockDst;

	for (BlockY = 0; BlockY < height; BlockY++)
	{
		BlockSrc = Src16 + BlockY * width * 1;
		BlockDst = Dst16 + BlockY * 320 * 1;
		for (BlockX = 0; BlockX < 64; BlockX++)
		{
			/* Horizontally:
			 * Before(4):
			 * (a)(b)(c)(d)
			 * After(5):
			 * (a)(abbb)(bc)(cccd)(d)
			 */

			// -- Row 1 --
			_1 = *(BlockSrc               );
			*(BlockDst               ) = _1;
			_2 = *(BlockSrc            + 1);
			*(BlockDst            + 1) = Weight1_3( _1,  _2);
			_3 = *(BlockSrc            + 2);
			*(BlockDst            + 2) = Weight1_1( _2,  _3);
			_4 = *(BlockSrc            + 3);
			*(BlockDst            + 3) = Weight3_1( _3,  _4);
			*(BlockDst            + 4) = _4;

			BlockSrc += 4;
			BlockDst += 5;
		}
	}
}


void upscale_256xXXX_to_320x240(uint32_t* restrict dst, uint32_t* restrict src, uint_fast16_t width, uint_fast16_t height, uint32_t midh)
{
    uint32_t Eh = 0;
    uint32_t source;
    uint32_t dh = 0;
    uint32_t y, x;

    for (y = 0; y < 240; y++)
    {
        source = dh * width / 2;

        for (x = 0; x < 320/10; x++)
        {
            register uint32_t ab, cd, ef, gh;

            __builtin_prefetch(dst + 4, 1);
            __builtin_prefetch(src + source + 4, 0);

            ab = src[source] & 0xF7DEF7DE;
            cd = src[source + 1] & 0xF7DEF7DE;
            ef = src[source + 2] & 0xF7DEF7DE;
            gh = src[source + 3] & 0xF7DEF7DE;

            if(Eh >= midh) {
                ab = AVERAGE(ab, src[source + width/2]) & 0xF7DEF7DE; // to prevent overflow
                cd = AVERAGE(cd, src[source + width/2 + 1]) & 0xF7DEF7DE; // to prevent overflow
                ef = AVERAGE(ef, src[source + width/2 + 2]) & 0xF7DEF7DE; // to prevent overflow
                gh = AVERAGE(gh, src[source + width/2 + 3]) & 0xF7DEF7DE; // to prevent overflow
            }

            *dst++ = ab;
            *dst++  = ((ab >> 17) + ((cd & 0xFFFF) >> 1)) + (cd << 16);
            *dst++  = (cd >> 16) + (ef << 16);
            *dst++  = (ef >> 16) + (((ef & 0xFFFF0000) >> 1) + ((gh & 0xFFFF) << 15));
            *dst++  = gh;

            source += 4;

        }
        Eh += height; if(Eh >= 240) { Eh -= 240; dh++; }
    }
}


/* alekmaul's scaler taken from mame4all */
void bitmap_scale(uint32_t startx, uint32_t starty, uint32_t viswidth, uint32_t visheight, uint32_t newwidth, uint32_t newheight,uint32_t pitchsrc,uint32_t pitchdest, uint16_t* restrict src, uint16_t* restrict dst)
{
    uint32_t W,H,ix,iy,x,y;
    x=startx<<16;
    y=starty<<16;
    W=newwidth;
    H=newheight;
    ix=(viswidth<<16)/W;
    iy=(visheight<<16)/H;

    do 
    {
        uint16_t* restrict buffer_mem=&src[(y>>16)*pitchsrc];
        W=newwidth; x=startx<<16;
        do 
        {
            *dst++=buffer_mem[x>>16];
            x+=ix;
        } while (--W);
        dst+=pitchdest;
        y+=iy;
	} while (--H);
}
