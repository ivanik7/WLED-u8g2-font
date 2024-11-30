#include "wled.h"
#include "fcn_declare.h"

/*

  u8g2_font.c

  Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)

  Copyright (c) 2016, olikraus@gmail.com
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this list
    of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice, this
    list of conditions and the following disclaimer in the documentation and/or other
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/* size of the font data structure, there is no struct or class... */
/* this is the size for the new font format */

#define U8G2_FONT_DATA_STRUCT_SIZE 23



#ifndef u8x8_pgm_read
#ifndef CHAR_BIT
#define u8x8_pgm_read(adr) (*(const uint8_t*)(adr))
#else
#if CHAR_BIT > 8
#define u8x8_pgm_read(adr) ((*(const uint8_t*)(adr)) & 0x0ff)
#else
#define u8x8_pgm_read(adr) (*(const uint8_t*)(adr))
#endif
#endif
#endif

#define U8G2_FONT_HEIGHT_MODE_TEXT 0
#define U8G2_FONT_HEIGHT_MODE_XTEXT 1
#define U8G2_FONT_HEIGHT_MODE_ALL 2

/*
  font data:

  offset	bytes	description
  0		1		glyph_cnt		number of glyphs
  1		1		bbx_mode	0: proportional, 1: common height, 2: monospace, 3: multiple of 8
  2		1		bits_per_0	glyph rle parameter
  3		1		bits_per_1	glyph rle parameter

  4		1		bits_per_char_width		glyph rle parameter
  5		1		bits_per_char_height	glyph rle parameter
  6		1		bits_per_char_x		glyph rle parameter
  7		1		bits_per_char_y		glyph rle parameter
  8		1		bits_per_delta_x		glyph rle parameter

  9		1		max_char_width
  10		1		max_char_height
  11		1		x offset
  12		1		y offset (descent)

  13		1		ascent (capital A)
  14		1		descent (lower g)
  15		1		ascent '('
  16		1		descent ')'

  17		1		start pos 'A' high byte
  18		1		start pos 'A' low byte

  19		1		start pos 'a' high byte
  20		1		start pos 'a' low byte

  21		1		start pos unicode high byte
  22		1		start pos unicode low byte

  Font build mode, 0: proportional, 1: common height, 2: monospace, 3: multiple of 8

  Font build mode 0:
    - "t"
    - Ref height mode: U8G2_FONT_HEIGHT_MODE_TEXT, U8G2_FONT_HEIGHT_MODE_XTEXT or U8G2_FONT_HEIGHT_MODE_ALL
    - use in transparent mode only (does not look good in solid mode)
    - most compact format
    - different font heights possible

  Font build mode 1:
    - "h"
    - Ref height mode: U8G2_FONT_HEIGHT_MODE_ALL
    - transparent or solid mode
    - The height of the glyphs depend on the largest glyph in the font. This means font height depends on postfix "r", "f" and "n".

*/

/* use case: What is the width and the height of the minimal box into which string s fints? */
void u8g2_font_GetStrSize(const void* font, const char* s, u8g2_uint_t* width, u8g2_uint_t* height);
void u8g2_font_GetStrSizeP(const void* font, const char* s, u8g2_uint_t* width, u8g2_uint_t* height);

/* use case: lower left edge of a minimal box is known, what is the correct x, y position for the string draw procedure */
void u8g2_font_AdjustXYToDraw(const void* font, const char* s, u8g2_uint_t* x, u8g2_uint_t* y);
void u8g2_font_AdjustXYToDrawP(const void* font, const char* s, u8g2_uint_t* x, u8g2_uint_t* y);

/* use case: Baseline origin known, return minimal box */
void u8g2_font_GetStrMinBox(u8g2_t* u8g2, const void* font, const char* s, u8g2_uint_t* x, u8g2_uint_t* y, u8g2_uint_t* width, u8g2_uint_t* height);

/* procedures */

/*========================================================================*/
/* low level byte and word access */

/* removed NOINLINE, because it leads to smaller code, might also be faster */
static uint8_t u8g2_font_get_byte(const uint8_t* font, uint8_t offset)
{
    font += offset;
    return u8x8_pgm_read(font);
}

static uint16_t u8g2_font_get_word(const uint8_t* font, uint8_t offset)
{
    uint16_t pos;
    font += offset;
    pos = u8x8_pgm_read(font);
    font++;
    pos <<= 8;
    pos += u8x8_pgm_read(font);
    return pos;
}

void u8g2_utf8_init(u8g2_t* u8g2)
{
    u8g2->utf8_state = 0; /* also reset during u8x8_SetupDefaults() */
}

/*========================================================================*/
/* new font format */
void u8g2_read_font_info(u8g2_font_info_t* font_info, const uint8_t* font)
{
    /* offset 0 */
    font_info->glyph_cnt = u8g2_font_get_byte(font, 0);
    font_info->bbx_mode = u8g2_font_get_byte(font, 1);
    font_info->bits_per_0 = u8g2_font_get_byte(font, 2);
    font_info->bits_per_1 = u8g2_font_get_byte(font, 3);

    /* offset 4 */
    font_info->bits_per_char_width = u8g2_font_get_byte(font, 4);
    font_info->bits_per_char_height = u8g2_font_get_byte(font, 5);
    font_info->bits_per_char_x = u8g2_font_get_byte(font, 6);
    font_info->bits_per_char_y = u8g2_font_get_byte(font, 7);
    font_info->bits_per_delta_x = u8g2_font_get_byte(font, 8);

    /* offset 9 */
    font_info->max_char_width = u8g2_font_get_byte(font, 9);
    font_info->max_char_height = u8g2_font_get_byte(font, 10);
    font_info->x_offset = u8g2_font_get_byte(font, 11);
    font_info->y_offset = u8g2_font_get_byte(font, 12);

    /* offset 13 */
    font_info->ascent_A = u8g2_font_get_byte(font, 13);
    font_info->descent_g = u8g2_font_get_byte(font, 14);
    font_info->ascent_para = u8g2_font_get_byte(font, 15);
    font_info->descent_para = u8g2_font_get_byte(font, 16);

    /* offset 17 */
    font_info->start_pos_upper_A = u8g2_font_get_word(font, 17);
    font_info->start_pos_lower_a = u8g2_font_get_word(font, 19);

    /* offset 21 */
    font_info->start_pos_unicode = u8g2_font_get_word(font, 21);
}

/* calculate the overall length of the font, only used to create the picture for the google wiki */
size_t u8g2_GetFontSize(const uint8_t* font_arg)
{
    uint16_t e;
    const uint8_t* font = font_arg;
    font += U8G2_FONT_DATA_STRUCT_SIZE;

    for (;;) {
        if (u8x8_pgm_read(font + 1) == 0)
            break;
        font += u8x8_pgm_read(font + 1);
    }

    /* continue with unicode section */
    font += 2;

    /* skip unicode lookup table */
    font += u8g2_font_get_word(font, 0);

    for (;;) {
        e = u8x8_pgm_read(font);
        e <<= 8;
        e |= u8x8_pgm_read(font + 1);
        if (e == 0)
            break;
        font += u8x8_pgm_read(font + 2);
    }

    return (font - font_arg) + 2;
}

/*========================================================================*/
/* u8g2 interface, font access */

uint8_t u8g2_GetFontBBXWidth(u8g2_t* u8g2)
{
    return u8g2->font_info.max_char_width; /* new font info structure */
}

uint8_t u8g2_GetFontBBXHeight(u8g2_t* u8g2)
{
    return u8g2->font_info.max_char_height; /* new font info structure */
}

int8_t u8g2_GetFontBBXOffX(u8g2_t* u8g2)
{
    return u8g2->font_info.x_offset; /* new font info structure */
}

int8_t u8g2_GetFontBBXOffY(u8g2_t* u8g2)
{
    return u8g2->font_info.y_offset; /* new font info structure */
}

uint8_t u8g2_GetFontCapitalAHeight(u8g2_t* u8g2)
{
    return u8g2->font_info.ascent_A; /* new font info structure */
}

/*========================================================================*/
/* glyph handling */

/* optimized */
uint8_t u8g2_font_decode_get_unsigned_bits(u8g2_font_decode_t* f, uint8_t cnt)
{
    uint8_t val;
    uint8_t bit_pos = f->decode_bit_pos;
    uint8_t bit_pos_plus_cnt;

    // val = *(f->decode_ptr);
    val = u8x8_pgm_read(f->decode_ptr);

    val >>= bit_pos;
    bit_pos_plus_cnt = bit_pos;
    bit_pos_plus_cnt += cnt;
    if (bit_pos_plus_cnt >= 8) {
        uint8_t s = 8;
        s -= bit_pos;
        f->decode_ptr++;
        // val |= *(f->decode_ptr) << (8-bit_pos);
        val |= u8x8_pgm_read(f->decode_ptr) << (s);
        // bit_pos -= 8;
        bit_pos_plus_cnt -= 8;
    }
    val &= (1U << cnt) - 1;
    // bit_pos += cnt;

    f->decode_bit_pos = bit_pos_plus_cnt;
    return val;
}

/*
    2 bit --> cnt = 2
      -2,-1,0. 1

    3 bit --> cnt = 3
      -2,-1,0. 1
      -4,-3,-2,-1,0,1,2,3

      if ( x < 0 )
        r = bits(x-1)+1;
    else
        r = bits(x)+1;

*/
/* optimized */
int8_t u8g2_font_decode_get_signed_bits(u8g2_font_decode_t* f, uint8_t cnt)
{
    int8_t v, d;
    v = (int8_t)u8g2_font_decode_get_unsigned_bits(f, cnt);
    d = 1;
    cnt--;
    d <<= cnt;
    v -= d;
    return v;
    // return (int8_t)u8g2_font_decode_get_unsigned_bits(f, cnt) - ((1<<cnt)>>1);
}

u8g2_uint_t u8g2_add_vector_y(u8g2_uint_t dy, int8_t x, int8_t y, uint8_t dir)
{
    switch (dir) {
    case 0:
        dy += y;
        break;
    case 1:
        dy += x;
        break;
    case 2:
        dy -= y;
        break;
    default:
        dy -= x;
        break;
    }
    return dy;
}

u8g2_uint_t u8g2_add_vector_x(u8g2_uint_t dx, int8_t x, int8_t y, uint8_t dir)
{
    switch (dir) {
    case 0:
        dx += x;
        break;
    case 1:
        dx -= y;
        break;
    case 2:
        dx -= x;
        break;
    default:
        dx += y;
        break;
    }
    return dx;
}

/*
  This is the toplevel function for the hv line draw procedures.
  This function should be called by the user.

  "dir" may have 4 directions: 0 (left to right), 1, 2, 3 (down up)
*/
void u8g2_DrawHVLine(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, uint8_t dir)
{
    /* Make a call to the callback function (e.g. u8g2_draw_l90_r0). */
    /* The callback may rotate the hv line */
    /* after rotation this will call u8g2_draw_hv_line_4dir() */

    if (len != 0) {

        /* convert to two directions */
        if (len > 1) {
            if (dir == 2) {
                x -= len;
                x++;
            } else if (dir == 3) {
                y -= len;
                y++;
            }
        }

        dir &= 1;

        /* clip against the user window */
        // if ( dir == 0 )
        // {
        //   if ( y < u8g2->user_y0 )
        //     return;

        //   if ( y >= u8g2->user_y1 )
        //     return;
        // }
        // else
        // {
        //   if ( x < u8g2->user_x0 )
        //     return;
        //   if ( x >= u8g2->user_x1 )
        //     return;
        // }

        u8g2->cb(x, y, len, dir);

        
        // TODO: DRAW HERE
    }
}

/*
  Description:
    Draw a run-length area of the glyph. "len" can have any size and the line
    length has to be wrapped at the glyph border.
  Args:
    len: 					Length of the line
    is_foreground			foreground/background?
    u8g2->font_decode.target_x		X position
    u8g2->font_decode.target_y		Y position
    u8g2->font_decode.is_transparent	Transparent mode
  Return:
    -
  Calls:
    u8g2_Draw90Line()
  Called by:
    u8g2_font_decode_glyph()
*/
/* optimized */
void u8g2_font_decode_len(u8g2_t* u8g2, uint8_t len, uint8_t is_foreground)
{
    uint8_t cnt; /* total number of remaining pixels, which have to be drawn */
    uint8_t rem; /* remaining pixel to the right edge of the glyph */
    uint8_t current; /* number of pixels, which need to be drawn for the draw procedure */
    /* current is either equal to cnt or equal to rem */

    /* local coordinates of the glyph */
    uint8_t lx, ly;

    /* target position on the screen */
    u8g2_uint_t x, y;

    u8g2_font_decode_t* decode = &(u8g2->font_decode);

    cnt = len;

    /* get the local position */
    lx = decode->x;
    ly = decode->y;

    for (;;) {
        /* calculate the number of pixel to the right edge of the glyph */
        rem = decode->glyph_width;
        rem -= lx;

        /* calculate how many pixel to draw. This is either to the right edge */
        /* or lesser, if not enough pixel are left */
        current = rem;
        if (cnt < rem)
            current = cnt;

        /* now draw the line, but apply the rotation around the glyph target position */
        // u8g2_font_decode_draw_pixel(u8g2, lx,ly,current, is_foreground);

        /* get target position */
        x = decode->target_x;
        y = decode->target_y;

        /* apply rotation */

        x = u8g2_add_vector_x(x, lx, ly, decode->dir);
        y = u8g2_add_vector_y(y, lx, ly, decode->dir);

        /* draw foreground and background (if required) */
        if (is_foreground) {

        u8g2_DrawHVLine(u8g2,
            x,
            y,
            current,
            /* dir */ decode->dir);
        }
        /* check, whether the end of the run length code has been reached */
        if (cnt < rem)
            break;
        cnt -= rem;
        lx = 0;
        ly++;
    }
    lx += cnt;

    decode->x = lx;
    decode->y = ly;
}

static void u8g2_font_setup_decode(u8g2_t* u8g2, const uint8_t* glyph_data)
{
    u8g2_font_decode_t* decode = &(u8g2->font_decode);
    decode->decode_ptr = glyph_data;
    decode->decode_bit_pos = 0;

    /* 8 Nov 2015, this is already done in the glyph data search procedure */
    /*
    decode->decode_ptr += 1;
    decode->decode_ptr += 1;
    */

    decode->glyph_width = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_char_width);
    decode->glyph_height = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_char_height);
}

/*
  Description:
    Decode and draw a glyph.
  Args:
    glyph_data: 					Pointer to the compressed glyph data of the font
    u8g2->font_decode.target_x		X position
    u8g2->font_decode.target_y		Y position
    u8g2->font_decode.is_transparent	Transparent mode
  Return:
    Width (delta x advance) of the glyph.
  Calls:
    u8g2_font_decode_len()
*/
/* optimized */
int8_t u8g2_font_decode_glyph(u8g2_t* u8g2, const uint8_t* glyph_data)
{
    uint8_t a, b;
    int8_t x, y;
    int8_t d;
    int8_t h;
    u8g2_font_decode_t* decode = &(u8g2->font_decode);

    u8g2_font_setup_decode(u8g2, glyph_data); /* set values in u8g2->font_decode data structure */
    h = u8g2->font_decode.glyph_height;

    x = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_char_x);
    y = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_char_y);
    d = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_delta_x);

    if (decode->glyph_width > 0) {
        decode->target_x = u8g2_add_vector_x(decode->target_x, x, -(h + y), decode->dir);
        decode->target_y = u8g2_add_vector_y(decode->target_y, x, -(h + y), decode->dir);

        /* reset local x/y position */
        decode->x = 0;
        decode->y = 0;

        /* decode glyph */
        for (;;) {
            a = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_0);
            b = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_1);
            do {
                u8g2_font_decode_len(u8g2, a, 0);
                u8g2_font_decode_len(u8g2, b, 1);
            } while (u8g2_font_decode_get_unsigned_bits(decode, 1) != 0);

            if (decode->y >= h)
                break;
        }
    }

    return d;
}

/*
  Description:
    Find the starting point of the glyph data.
  Args:
    encoding: Encoding (ASCII or Unicode) of the glyph
  Return:
    Address of the glyph data or NULL, if the encoding is not avialable in the font.
*/
const uint8_t* u8g2_font_get_glyph_data(u8g2_t* u8g2, uint16_t encoding)
{
    const uint8_t* font = u8g2->font;
    font += U8G2_FONT_DATA_STRUCT_SIZE;

    if (encoding <= 255) {
        if (encoding >= 'a') {
            font += u8g2->font_info.start_pos_lower_a;
        } else if (encoding >= 'A') {
            font += u8g2->font_info.start_pos_upper_A;
        }

        for (;;) {
            if (u8x8_pgm_read(font + 1) == 0)
                break;
            if (u8x8_pgm_read(font) == encoding) {
                return font + 2; /* skip encoding and glyph size */
            }
            font += u8x8_pgm_read(font + 1);
        }
    } else {
        uint16_t e;
        const uint8_t* unicode_lookup_table;

        font += u8g2->font_info.start_pos_unicode;
        unicode_lookup_table = font;

        /* issue 596: search for the glyph start in the unicode lookup table */
        do {
            font += u8g2_font_get_word(unicode_lookup_table, 0);
            e = u8g2_font_get_word(unicode_lookup_table, 2);
            unicode_lookup_table += 4;
        } while (e < encoding);

        for (;;) {
            e = u8x8_pgm_read(font);
            e <<= 8;
            e |= u8x8_pgm_read(font + 1);

            if (e == 0)
                break;

            if (e == encoding) {

                return font + 3; /* skip encoding and glyph size */
            }
            font += u8x8_pgm_read(font + 2);
        }
    }

    return NULL;
}

static u8g2_uint_t u8g2_font_draw_glyph(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, uint16_t encoding)
{
    u8g2_uint_t dx = 0;
    u8g2->font_decode.target_x = x;
    u8g2->font_decode.target_y = y;
    // u8g2->font_decode.is_transparent = is_transparent; this is already set
    // u8g2->font_decode.dir = dir;
    const uint8_t* glyph_data = u8g2_font_get_glyph_data(u8g2, encoding);

    if (glyph_data != NULL) {
        dx = u8g2_font_decode_glyph(u8g2, glyph_data);
    }

    return dx;
}

uint8_t u8g2_IsGlyph(u8g2_t* u8g2, uint16_t requested_encoding)
{
    /* updated to new code */
    if (u8g2_font_get_glyph_data(u8g2, requested_encoding) != NULL)
        return 1;
    return 0;
}

/* side effect: updates u8g2->font_decode and u8g2->glyph_x_offset */
int8_t u8g2_GetGlyphWidth(u8g2_t* u8g2, uint16_t requested_encoding)
{
    const uint8_t* glyph_data = u8g2_font_get_glyph_data(u8g2, requested_encoding);
    if (glyph_data == NULL)
        return 0;

    u8g2_font_setup_decode(u8g2, glyph_data);
    u8g2->glyph_x_offset = u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_char_x);
    u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_char_y);

    /* glyph width is here: u8g2->font_decode.glyph_width */

    return u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_delta_x);
}


u8g2_uint_t u8g2_DrawGlyph(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, uint16_t encoding)
{
    // switch (u8g2->font_decode.dir) {
    // case 0:
    //     y += u8g2->font_calc_vref(u8g2);
    //     break;
    // case 1:
    //     x -= u8g2->font_calc_vref(u8g2);
    //     break;
    // case 2:
    //     y -= u8g2->font_calc_vref(u8g2);
    //     break;
    // case 3:
    //     x += u8g2->font_calc_vref(u8g2);
    //     break;
    // }

    return u8g2_font_draw_glyph(u8g2, x, y, encoding);
}


/*
  pass a byte from an utf8 encoded string to the utf8 decoder state machine
  returns
    0x0fffe: no glyph, just continue
    0x0ffff: end of string
    anything else: The decoded encoding
*/
uint16_t u8g2_utf8_next(u8g2_t* u8g2, uint8_t b)
{
    if (b == 0 || b == '\n') /* '\n' terminates the string to support the string list procedures */
        return 0x0ffff; /* end of string detected, pending UTF8 is discarded */
    if (u8g2->utf8_state == 0) {
        if (b >= 0xfc) /* 6 byte sequence */
        {
            u8g2->utf8_state = 5;
            b &= 1;
        } else if (b >= 0xf8) {
            u8g2->utf8_state = 4;
            b &= 3;
        } else if (b >= 0xf0) {
            u8g2->utf8_state = 3;
            b &= 7;
        } else if (b >= 0xe0) {
            u8g2->utf8_state = 2;
            b &= 15;
        } else if (b >= 0xc0) {
            u8g2->utf8_state = 1;
            b &= 0x01f;
        } else {
            /* do nothing, just use the value as encoding */
            return b;
        }
        u8g2->encoding = b;
        return 0x0fffe;
    } else {
        u8g2->utf8_state--;
        /* The case b < 0x080 (an illegal UTF8 encoding) is not checked here. */
        u8g2->encoding <<= 6;
        b &= 0x03f;
        u8g2->encoding |= b;
        if (u8g2->utf8_state != 0)
            return 0x0fffe; /* nothing to do yet */
    }
    return u8g2->encoding;
}

static u8g2_uint_t u8g2_draw_string(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, const char* str)
{
            // u8g2_DrawGlyph(u8g2, x, y, 97);

    uint16_t e;
    u8g2_uint_t delta, sum;
    u8g2_utf8_init(u8g2);
    sum = 0;
    for (;;) {
        e = u8g2_utf8_next(u8g2, (uint8_t)*str);
        if (e == 0x0ffff)
            break;

        str++;

        if (e != 0x0fffe) {
            delta = u8g2_DrawGlyph(u8g2, x, y, e);

            switch (u8g2->font_decode.dir) {
            case 0:
                x += delta;
                break;
            case 1:
                y += delta;
                break;
            case 2:
                x -= delta;
                break;
            case 3:
                y -= delta;
                break;
            }

            sum += delta;
        }
    }
    return sum;
}


/*
source: https://en.wikipedia.org/wiki/UTF-8
Bits	from 		to			bytes	Byte 1 		Byte 2 		Byte 3 		Byte 4 		Byte 5 		Byte 6
  7 	U+0000 		U+007F 		1 		0xxxxxxx
11 	U+0080 		U+07FF 		2 		110xxxxx 	10xxxxxx
16 	U+0800 		U+FFFF 		3 		1110xxxx 	10xxxxxx 	10xxxxxx
21 	U+10000 	U+1FFFFF 	4 		11110xxx 	10xxxxxx 	10xxxxxx 	10xxxxxx
26 	U+200000 	U+3FFFFFF 	5 		111110xx 	10xxxxxx 	10xxxxxx 	10xxxxxx 	10xxxxxx
31 	U+4000000 	U+7FFFFFFF 	6 		1111110x 	10xxxxxx 	10xxxxxx 	10xxxxxx 	10xxxxxx 	10xxxxxx
*/
u8g2_uint_t u8g2_DrawUTF8(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, const char* str)
{
    return u8g2_draw_string(u8g2, x, y, str);
}

/* this function is used as "u8g2_get_kerning_cb" */
uint8_t u8g2_GetKerning(u8g2_t* u8g2, u8g2_kerning_t* kerning, uint16_t e1, uint16_t e2)
{
    uint16_t i1, i2, cnt, end;
    if (kerning == NULL)
        return 0;

    /* search for the encoding in the first table */
    cnt = kerning->first_table_cnt;
    cnt--; /* ignore the last element of the table, which is 0x0ffff */
    for (i1 = 0; i1 < cnt; i1++) {
        if (kerning->first_encoding_table[i1] == e1)
            break;
    }
    if (i1 >= cnt)
        return 0; /* e1 not part of the kerning table, return 0 */

    /* get the upper index for i2 */
    end = kerning->index_to_second_table[i1 + 1];
    for (i2 = kerning->index_to_second_table[i1]; i2 < end; i2++) {
        if (kerning->second_encoding_table[i2] == e2)
            break;
    }

    if (i2 >= end)
        return 0; /* e2 not part of any pair with e1, return 0 */

    return kerning->kerning_values[i2];
}

uint8_t u8g2_GetKerningByTable(u8g2_t* u8g2, const uint16_t* kt, uint16_t e1, uint16_t e2)
{
    uint16_t i;
    i = 0;
    if (kt == NULL)
        return 0;
    for (;;) {
        if (kt[i] == 0x0ffff)
            break;
        if (kt[i] == e1 && kt[i + 1] == e2)
            return kt[i + 2];
        i += 3;
    }
    return 0;
}

u8g2_uint_t u8g2_DrawExtendedUTF8(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, uint8_t to_left, u8g2_kerning_t* kerning, const char* str)
{
    uint16_t e_prev = 0x0ffff;
    uint16_t e;
    u8g2_uint_t delta, sum, k;
    u8g2_utf8_init(u8g2);
    sum = 0;
    for (;;) {
        e = u8g2_utf8_next(u8g2, (uint8_t)*str);
        if (e == 0x0ffff)
            break;
        str++;
        if (e != 0x0fffe) {
            delta = u8g2_GetGlyphWidth(u8g2, e);

            if (to_left) {
                k = u8g2_GetKerning(u8g2, kerning, e, e_prev);
                delta -= k;
                x -= delta;
            } else {
                k = u8g2_GetKerning(u8g2, kerning, e_prev, e);
                delta -= k;
            }
            e_prev = e;

            u8g2_DrawGlyph(u8g2, x, y, e);
            if (to_left) {
            } else {
                x += delta;
                x -= k;
            }

            sum += delta;
        }
    }
    return sum;
}

u8g2_uint_t u8g2_DrawExtUTF8(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, uint8_t to_left, const uint16_t* kerning_table, const char* str)
{
    uint16_t e_prev = 0x0ffff;
    uint16_t e;
    u8g2_uint_t delta, sum, k;
    u8g2_utf8_init(u8g2);
    sum = 0;
    for (;;) {
        e = u8g2_utf8_next(u8g2, (uint8_t)*str);
        if (e == 0x0ffff)
            break;
        str++;
        if (e != 0x0fffe) {
            delta = u8g2_GetGlyphWidth(u8g2, e);

            if (to_left) {
                k = u8g2_GetKerningByTable(u8g2, kerning_table, e, e_prev);
                delta -= k;
                x -= delta;
            } else {
                k = u8g2_GetKerningByTable(u8g2, kerning_table, e_prev, e);
                delta -= k;
            }
            e_prev = e;

            if (to_left) {
            } else {
                x += delta;
            }
            u8g2_DrawGlyph(u8g2, x, y, e);
            if (to_left) {
            } else {
                // x += delta;
                // x -= k;
            }

            sum += delta;
        }
    }
    return sum;
}

/*===============================================*/

/* set ascent/descent for reference point calculation */

void u8g2_UpdateRefHeight(u8g2_t* u8g2)
{
    if (u8g2->font == NULL)
        return;
    u8g2->font_ref_ascent = u8g2->font_info.ascent_A;
    u8g2->font_ref_descent = u8g2->font_info.descent_g;
    if (u8g2->font_height_mode == U8G2_FONT_HEIGHT_MODE_TEXT) {
    } else if (u8g2->font_height_mode == U8G2_FONT_HEIGHT_MODE_XTEXT) {
        if (u8g2->font_ref_ascent < u8g2->font_info.ascent_para)
            u8g2->font_ref_ascent = u8g2->font_info.ascent_para;
        if (u8g2->font_ref_descent > u8g2->font_info.descent_para)
            u8g2->font_ref_descent = u8g2->font_info.descent_para;
    } else {
        if (u8g2->font_ref_ascent < u8g2->font_info.max_char_height + u8g2->font_info.y_offset)
            u8g2->font_ref_ascent = u8g2->font_info.max_char_height + u8g2->font_info.y_offset;
        if (u8g2->font_ref_descent > u8g2->font_info.y_offset)
            u8g2->font_ref_descent = u8g2->font_info.y_offset;
    }
}

// void u8g2_SetFontRefHeightText(u8g2_t* u8g2)
// {
//     u8g2->font_height_mode = U8G2_FONT_HEIGHT_MODE_TEXT;
//     u8g2_UpdateRefHeight(u8g2);
// }

// void u8g2_SetFontRefHeightExtendedText(u8g2_t* u8g2)
// {
//     u8g2->font_height_mode = U8G2_FONT_HEIGHT_MODE_XTEXT;
//     u8g2_UpdateRefHeight(u8g2);
// }

// void u8g2_SetFontRefHeightAll(u8g2_t* u8g2)
// {
//     u8g2->font_height_mode = U8G2_FONT_HEIGHT_MODE_ALL;
//     u8g2_UpdateRefHeight(u8g2);
// }

/*===============================================*/
/* callback procedures to correct the y position */

// u8g2_uint_t u8g2_font_calc_vref_top(u8g2_t* u8g2)
// {
//     u8g2_uint_t tmp;
//     /* reference pos is one pixel above the upper edge of the reference glyph */
//     tmp = (u8g2_uint_t)(u8g2->font_ref_ascent);
//     tmp++;
//     return tmp;
// }

// void u8g2_SetFontPosTop(u8g2_t* u8g2)
// {
//     u8g2->font_calc_vref = u8g2_font_calc_vref_top;
// }

// u8g2_uint_t u8g2_font_calc_vref_center(u8g2_t* u8g2)
// {
//     int8_t tmp;
//     tmp = u8g2->font_ref_ascent;
//     tmp -= u8g2->font_ref_descent;
//     tmp /= 2;
//     tmp += u8g2->font_ref_descent;
//     return tmp;
// }

// void u8g2_SetFontPosCenter(u8g2_t* u8g2)
// {
//     u8g2->font_calc_vref = u8g2_font_calc_vref_center;
// }

/*===============================================*/


#if defined(ESP8266)
uint8_t u8x8_pgm_read_esp(const uint8_t * addr) 
{
    uint32_t bytes;
    bytes = *(uint32_t*)((uint32_t)addr & ~3);
    return ((uint8_t*)&bytes)[(uint32_t)addr & 3];
}
#endif


void u8g2_SetFont(u8g2_t* u8g2, const uint8_t* font)
{
    if (u8g2->font != font) {
        u8g2->font = font;
        u8g2_read_font_info(&(u8g2->font_info), font);
        u8g2_UpdateRefHeight(u8g2);
    }
}

/*===============================================*/

static uint8_t u8g2_is_all_valid(u8g2_t* u8g2, const char* str)
{
    uint16_t e;
    u8g2_utf8_init(u8g2);
    for (;;) {
        e = u8g2_utf8_next(u8g2, (uint8_t)*str);
        if (e == 0x0ffff)
            break;
        str++;
        if (e != 0x0fffe) {
            if (u8g2_font_get_glyph_data(u8g2, e) == NULL)
                return 0;
        }
    }
    return 1;
}

uint8_t u8g2_IsAllValidUTF8(u8g2_t* u8g2, const char* str)
{
    return u8g2_is_all_valid(u8g2, str);
}

/* string calculation is stilll not 100% perfect as it addes the initial string offset to the overall size */
static u8g2_uint_t u8g2_string_width(u8g2_t* u8g2, const char* str)
{
    uint16_t e;
    u8g2_uint_t w, dx;
#ifdef U8G2_BALANCED_STR_WIDTH_CALCULATION
    int8_t initial_x_offset = -64;
#endif

    u8g2->font_decode.glyph_width = 0;
    u8g2_utf8_init(u8g2);

    /* reset the total width to zero, this will be expanded during calculation */
    w = 0;
    dx = 0;

    // printf("str=<%s>\n", str);

    for (;;) {
        e = u8g2_utf8_next(u8g2, (uint8_t)*str);
        if (e == 0x0ffff)
            break;
        str++;
        if (e != 0x0fffe) {
            dx = u8g2_GetGlyphWidth(u8g2, e); /* delta x value of the glyph, side effect: updates u8g2->glyph_x_offset */
#ifdef U8G2_BALANCED_STR_WIDTH_CALCULATION
            if (initial_x_offset == -64)
                initial_x_offset = u8g2->glyph_x_offset;
#endif
            // printf("'%c' x=%d dx=%d w=%d io=%d ", e, u8g2->glyph_x_offset, dx, u8g2->font_decode.glyph_width, initial_x_offset);
            w += dx;
        }
    }
    // printf("\n");

    /* adjust the last glyph, check for issue #16: do not adjust if width is 0 */
    if (u8g2->font_decode.glyph_width != 0) {
        // printf("string width adjust dx=%d glyph_width=%d x-offset=%d\n", dx, u8g2->font_decode.glyph_width, u8g2->glyph_x_offset);
        w -= dx;
        w += u8g2->font_decode.glyph_width; /* the real pixel width of the glyph, sideeffect of GetGlyphWidth */
        /* issue #46: we have to add the x offset also */
        w += u8g2->glyph_x_offset; /* this value is set as a side effect of u8g2_GetGlyphWidth() */
#ifdef U8G2_BALANCED_STR_WIDTH_CALCULATION
        /* https://github.com/olikraus/u8g2/issues/1561 */
        if (initial_x_offset > 0)
            w += initial_x_offset;
#endif
    }
    // printf("w=%d \n", w);

    return w;
}

int8_t u8g2_GetXOffsetGlyph(u8g2_t* u8g2, uint16_t encoding)
{
    u8g2_GetGlyphWidth(u8g2, encoding); /* delta x value of the glyph, side effect: updates u8g2->glyph_x_offset */
    return u8g2->glyph_x_offset;
}

int8_t u8g2_GetXOffsetUTF8(u8g2_t* u8g2, const char* utf8)
{
    uint16_t e;
    u8g2_utf8_init(u8g2);
    for (;;) // extract encoding from UTF8 byte stream
    {
        e = u8g2_utf8_next(u8g2, (uint8_t)*utf8);
        if (e == 0x0ffff)
            return 0;
        if (e < 0x0fffe) // 0x0fffe means: just continue
            break;
        utf8++;
    }
    return u8g2_GetXOffsetGlyph(u8g2, e);
}

static void u8g2_GetGlyphHorizontalProperties(u8g2_t* u8g2, uint16_t requested_encoding, uint8_t* w, int8_t* ox, int8_t* dx)
{
    const uint8_t* glyph_data = u8g2_font_get_glyph_data(u8g2, requested_encoding);
    if (glyph_data == NULL)
        return;

    u8g2_font_setup_decode(u8g2, glyph_data);
    *w = u8g2->font_decode.glyph_width;
    *ox = u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_char_x);
    u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_char_y);
    *dx = u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_delta_x);
}

/* u8g compatible GetStrX function */
int8_t u8g2_GetStrX(u8g2_t* u8g2, const char* s)
{
    uint8_t w;
    int8_t dx;
    int8_t ox = 0;
    u8g2_GetGlyphHorizontalProperties(u8g2, *s, &w, &ox, &dx);
    return ox;
}

u8g2_uint_t u8g2_GetStrWidth(u8g2_t* u8g2, const char* s)
{
    return u8g2_string_width(u8g2, s);
}

/*
source: https://en.wikipedia.org/wiki/UTF-8
Bits	from 		to			bytes	Byte 1 		Byte 2 		Byte 3 		Byte 4 		Byte 5 		Byte 6
  7 	U+0000 		U+007F 		1 		0xxxxxxx
11 	U+0080 		U+07FF 		2 		110xxxxx 	10xxxxxx
16 	U+0800 		U+FFFF 		3 		1110xxxx 	10xxxxxx 	10xxxxxx
21 	U+10000 	U+1FFFFF 	4 		11110xxx 	10xxxxxx 	10xxxxxx 	10xxxxxx
26 	U+200000 	U+3FFFFFF 	5 		111110xx 	10xxxxxx 	10xxxxxx 	10xxxxxx 	10xxxxxx
31 	U+4000000 	U+7FFFFFFF 	6 		1111110x 	10xxxxxx 	10xxxxxx 	10xxxxxx 	10xxxxxx 	10xxxxxx
*/
u8g2_uint_t u8g2_GetUTF8Width(u8g2_t* u8g2, const char* str)
{
    return u8g2_string_width(u8g2, str);
}

void u8g2_SetFontDirection(u8g2_t* u8g2, uint8_t dir)
{
    u8g2->font_decode.dir = dir;
}

