uint8_t u8x8_pgm_read_esp(const uint8_t * addr);   /* u8x8_8x8.c */
#define U8X8_FONT_SECTION(name) __attribute__((section(".text." name)))
#define u8x8_pgm_read(adr) u8x8_pgm_read_esp(adr)
#define U8X8_PROGMEM

#define U8G2_FONT_SECTION(name) U8X8_FONT_SECTION(name) 


//u8g2_font.cpp

typedef uint16_t u8g2_uint_t; /* for pixel position only */
typedef int16_t u8g2_int_t; /* introduced for circle calculation */
typedef int32_t u8g2_long_t; /* introduced for ellipse calculation */

typedef struct u8g2_struct u8g2_t;

struct _u8g2_font_decode_t {
    const uint8_t* decode_ptr; /* pointer to the compressed data */

    u8g2_uint_t target_x;
    u8g2_uint_t target_y;

    int8_t x; /* local coordinates, (0,0) is upper left */
    int8_t y;
    int8_t glyph_width;
    int8_t glyph_height;

    uint8_t decode_bit_pos; /* bitpos inside a byte of the compressed data */
    uint8_t dir; /* direction */
};

typedef struct _u8g2_font_decode_t u8g2_font_decode_t;

/* from ucglib... */
struct _u8g2_font_info_t {
    /* offset 0 */
    uint8_t glyph_cnt;
    uint8_t bbx_mode;
    uint8_t bits_per_0;
    uint8_t bits_per_1;

    /* offset 4 */
    uint8_t bits_per_char_width;
    uint8_t bits_per_char_height;
    uint8_t bits_per_char_x;
    uint8_t bits_per_char_y;
    uint8_t bits_per_delta_x;

    /* offset 9 */
    int8_t max_char_width;
    int8_t max_char_height; /* overall height, NOT ascent. Instead ascent = max_char_height + y_offset */
    int8_t x_offset;
    int8_t y_offset;

    /* offset 13 */
    int8_t ascent_A;
    int8_t descent_g; /* usually a negative value */
    int8_t ascent_para;
    int8_t descent_para;

    /* offset 17 */
    uint16_t start_pos_upper_A;
    uint16_t start_pos_lower_a;

    /* offset 21 */
    uint16_t start_pos_unicode;
};

typedef struct _u8g2_font_info_t u8g2_font_info_t;

typedef u8g2_uint_t (*u8g2_font_calc_vref_fnptr)(u8g2_t* u8g2);

// typedef void (*u8g2_draw_l90_cb)(u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, uint8_t dir);

struct u8g2_struct {
    // const u8g2_cb_t* cb; /* callback drawprocedures, can be replaced for rotation */

    uint8_t utf8_state;
    uint16_t encoding; /* encoding result for utf8 decoder in next_cb */

    /* display dimensions in pixel for the user, calculated in u8g2_update_dimension_common()  */
    u8g2_uint_t width;
    u8g2_uint_t height;

    /* information about the current font */
    const uint8_t* font; /* current font for all text procedures */
    // removed: const u8g2_kerning_t *kerning;		/* can be NULL */
    // removed: u8g2_get_kerning_cb get_kerning_cb;

    // u8g2_font_calc_vref_fnptr font_calc_vref;
    u8g2_font_decode_t font_decode; /* new font decode structure */
    u8g2_font_info_t font_info; /* new font info structure */

    uint8_t font_height_mode;
    int8_t font_ref_ascent;
    int8_t font_ref_descent;

    int8_t glyph_x_offset; /* set by u8g2_GetGlyphWidth as a side effect */

    // u8g2_draw_l90_cb cb;
};


struct _u8g2_kerning_t
{
  uint16_t first_table_cnt;
  uint16_t second_table_cnt;
  const uint16_t *first_encoding_table;  
  const uint16_t *index_to_second_table;
  const uint16_t *second_encoding_table;
  const uint8_t *kerning_values;
};

typedef struct _u8g2_kerning_t u8g2_kerning_t;

void u8g2_SetFont(u8g2_t* u8g2, const uint8_t* font);
u8g2_uint_t u8g2_DrawUTF8(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, const char* str);

extern const uint8_t u8g2_font_u8glib_4_tf[] U8G2_FONT_SECTION("u8g2_font_u8glib_4_tf");
