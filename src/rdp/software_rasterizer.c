#include "rdp/software_rasterizer.h"

#include "common.h"

#define SCANBUFFER_HEIGHT 480
#define MAX_WIDTH 640

__attribute__((__always_inline__)) static inline float get_float_value_from_frmt(uint32_t integer, uint32_t decimal, float decimal_max)
{
    return ((int)integer) + ((float)decimal / decimal_max);
}

__attribute__((__always_inline__)) static inline float get_ten_point_two(uint16_t value)
{
    return get_float_value_from_frmt(value >> 2, value & 0x3, 3.0f);
}

__attribute__((__always_inline__)) static inline float get_ten_point_five(uint16_t value)
{
    return get_float_value_from_frmt(value >> 5, value & 0x1F, 31.0f);
}

__attribute__((__always_inline__)) static inline float get_five_point_ten(uint16_t value)
{
    return get_float_value_from_frmt(value >> 10, value & 0x3FF, 1024.0f);
}

__attribute__((__always_inline__)) static inline uint32_t get_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    if (curr_colorimage.image_size == BPP_32)
        return (r << 24) | (g << 16) | (b << 8) | a;
    else if (curr_colorimage.image_size == BPP_16)
        return (((r & 0b11111) << 11) | ((g & 0b11111) << 6) | ((b & 0b11111) << 1) | (a > 0)) 
            | ((((r & 0b11111) << 11) | ((g & 0b11111) << 6) | ((b & 0b11111) << 1) | (a > 0)) << 16);
    return 0;
}

__attribute__((__always_inline__)) static inline void set_pixel(uint32_t x, uint32_t y, uint32_t packed_color, uint32_t SX1, uint32_t SY1, uint32_t SX2, uint32_t SY2)
{
    if ((x < SX1 || y < SY1) || (x > SX2 || y > SY2)) return;

    uint32_t index = x + y * (curr_colorimage.image_width+1);
    if (curr_colorimage.image_size == BPP_16)
        index >>= 1;
    uint32_t* framebuffer = get_real_memory_loc(curr_colorimage.image_addr);
    framebuffer[index] = bswap_32(packed_color);
}

void draw_scanbuffer(uint32_t* scanbuffer, edgecoeff_t* edges, shadecoeff_t* shade)
{
    uint32_t screen_x1 = (uint32_t)(scissor_border.border.XH >> 2);
    uint32_t screen_y1 = (uint32_t)(scissor_border.border.YH >> 2);
    uint32_t screen_x2 = (uint32_t)(scissor_border.border.XL >> 2);
    uint32_t screen_y2 = (uint32_t)(scissor_border.border.YL >> 2);

    if (curr_colorimage.image_format == FRMT_RGBA)
    {
        float shd_red   = 0;
        float shd_green = 0;
        float shd_blue  = 0;
        float shd_alpha = 0;

        float shd_DrDx = 0;
        float shd_DgDx = 0;
        float shd_DbDx = 0;
        float shd_DaDx = 0;

        float shd_DrDe = 0;
        float shd_DgDe = 0;
        float shd_DbDe = 0;
        float shd_DaDe = 0;

        float shd_DrDy = 0;
        float shd_DgDy = 0;
        float shd_DbDy = 0;
        float shd_DaDy = 0;

        float shd_red_temp   = 0;
        float shd_green_temp = 0;
        float shd_blue_temp  = 0;
        float shd_alpha_temp = 0;

        bool is_cycles = othermodes.cycle_type == CYCLE_1 || othermodes.cycle_type == CYCLE_2;

        if (shade && is_cycles)
        {
            shd_red   = get_float_value_from_frmt((short)shade->red,   shade->red_frac,   65535.0f);
            shd_green = get_float_value_from_frmt((short)shade->green, shade->green_frac, 65535.0f);
            shd_blue  = get_float_value_from_frmt((short)shade->blue,  shade->blue_frac,  65535.0f);
            shd_alpha = get_float_value_from_frmt((short)shade->blue,  shade->blue_frac,  65535.0f);

            shd_DrDx = get_float_value_from_frmt((short)shade->DrDx, shade->DrDx_frac, 65535.0f);
            shd_DgDx = get_float_value_from_frmt((short)shade->DgDx, shade->DgDx_frac, 65535.0f);
            shd_DbDx = get_float_value_from_frmt((short)shade->DbDx, shade->DbDx_frac, 65535.0f);
            shd_DaDx = get_float_value_from_frmt((short)shade->DaDx, shade->DaDx_frac, 65535.0f);

            shd_DrDe = get_float_value_from_frmt((short)shade->DrDe, shade->DrDe_frac, 65535.0f);
            shd_DgDe = get_float_value_from_frmt((short)shade->DgDe, shade->DgDe_frac, 65535.0f);
            shd_DbDe = get_float_value_from_frmt((short)shade->DbDe, shade->DbDe_frac, 65535.0f);
            shd_DaDe = get_float_value_from_frmt((short)shade->DaDe, shade->DaDe_frac, 65535.0f);

            shd_DrDy = get_float_value_from_frmt((short)shade->DrDy, shade->DrDy_frac, 65535.0f);
            shd_DgDy = get_float_value_from_frmt((short)shade->DgDy, shade->DgDy_frac, 65535.0f);
            shd_DbDy = get_float_value_from_frmt((short)shade->DbDy, shade->DbDy_frac, 65535.0f);
            shd_DaDy = get_float_value_from_frmt((short)shade->DaDy, shade->DaDy_frac, 65535.0f);

            /*
            printf("red:  %f green: %f blue: %f alpha: %f\n", shd_red, shd_green, shd_blue, shd_alpha);
            printf("DrDx: %f DgDx:  %f DbDx: %f DaDx:  %f\n", shd_DrDx, shd_DgDx, shd_DbDx, shd_DaDx);
            printf("DrDe: %f DgDe:  %f DbDe: %f DaDe:  %f\n", shd_DrDe, shd_DgDe, shd_DbDe, shd_DaDe);
            printf("DrDy: %f DgDy:  %f DbDy: %f DaDy:  %f\n", shd_DrDy, shd_DgDy, shd_DbDy, shd_DaDy);
            */
        }

        for (size_t y = screen_y1; y < screen_y2; ++y)
        {
            uint32_t xmin = scanbuffer[(y * 2)    ];
            uint32_t xmax = scanbuffer[(y * 2) + 1];

            if (shade && is_cycles && xmax != 0)
            {
                shd_red   += shd_DrDy;
                shd_green += shd_DgDy;
                shd_blue  += shd_DbDy;
                shd_alpha += shd_DaDy;

                shd_red_temp   = shd_red;
                shd_green_temp = shd_green;
                shd_blue_temp  = shd_blue;
                shd_alpha_temp = shd_alpha;
            }

            for (size_t x = xmin; x < xmax; ++x)
            {
                if (xmax == 0) break;
                uint32_t color = 0;
                if (othermodes.cycle_type == CYCLE_FILL)
                    color = fill_color;
                else if (is_cycles)
                {
                    if (shade)
                    {
                        // this isn't how you do this at all, and this really needs fixing (produces wildly incorrect results thus is disabled for now)
                        // other than this, shading seems to work fine, but needs this to look correct at all.
                        bool edge_cond = false;
                        if      (edges->lft == 0) edge_cond = (x == xmax-1);
                        else if (edges->lft == 1) edge_cond = (x == xmin);

                        if (edge_cond && y < edges->YM >> 2)
                        {
                            //shd_red_temp   += shd_DrDe;
                            //shd_green_temp += shd_DgDe;
                            //shd_blue_temp  += shd_DbDe;
                            //shd_alpha_temp += shd_DaDe;

                            //shd_red   += shd_DrDe;
                            //shd_green += shd_DgDe;
                            //shd_blue  += shd_DbDe;
                            //shd_alpha += shd_DaDe;
                        }

                        shd_red   += shd_DrDx;
                        shd_green += shd_DgDx;
                        shd_blue  += shd_DbDx;
                        shd_alpha += shd_DaDx;

                        //printf("%f %f %f %f\n", shd_red, shd_green, shd_blue, shd_alpha);

                        color = get_color((uint8_t)(shd_red), (uint8_t)(shd_green), (uint8_t)(shd_blue), (uint8_t)(shd_alpha));
                    }
                }
                
                set_pixel(x, y, color, screen_x1, screen_y1, screen_x2, screen_y2);
            }

            if (shade && is_cycles && xmax != 0)
            {
                shd_red   = shd_red_temp;
                shd_green = shd_green_temp;
                shd_blue  = shd_blue_temp;
                shd_alpha = shd_alpha_temp;
            }
        }
    }
    else
    {
        puts("Drawing Scanbuffers isn't supported in any other mode other than RGBA currently.");
    }
}

void scan_convert_line(uint32_t* scanbuffer, float xstep, uint32_t xstart, uint32_t ystart, uint32_t yend, int side)
{
    float currx = xstart;

    for (uint32_t y = ystart; y < yend; ++y)
    {
        size_t index = (y * 2) + side;
        if (index > SCANBUFFER_HEIGHT * 2) break;
        scanbuffer[index] = (uint32_t)currx;
        currx += xstep;
    }
}

void scan_convert_triangle(uint32_t* scanbuffer, edgecoeff_t* edges)
{
    float xstep1 = get_float_value_from_frmt((short)edges->DxHDy, edges->DxHDy_frac, 65535.0f);
    float xstep2 = get_float_value_from_frmt((short)edges->DxLDy, edges->DxLDy_frac, 65535.0f);
    float xstep3 = get_float_value_from_frmt((short)edges->DxMDy, edges->DxMDy_frac, 65535.0f);

    scan_convert_line(scanbuffer, xstep1, edges->XH, edges->YH >> 2, edges->YL >> 2, (int)(1 - edges->lft));
    scan_convert_line(scanbuffer, xstep2, edges->XL, edges->YM >> 2, edges->YL >> 2, edges->lft);
    scan_convert_line(scanbuffer, xstep3, edges->XM, edges->YH >> 2, edges->YM >> 2, edges->lft);
}

void draw_triangle(edgecoeff_t* edges, shadecoeff_t* shade, texcoeff_t* texture, zbuffercoeff_t* zbuf)
{
    uint32_t tri_scanbuffer[SCANBUFFER_HEIGHT * 2];
    memset(tri_scanbuffer, 0, (SCANBUFFER_HEIGHT * 2) * sizeof(uint32_t));

    rgbacolor_t shade_edge_table[MAX_WIDTH][SCANBUFFER_HEIGHT];
    memset(&shade_edge_table, 0, sizeof(shade_edge_table));

    scan_convert_triangle(tri_scanbuffer, edges);

    draw_scanbuffer(tri_scanbuffer, edges, shade);
}

void fill_rect(rect_t* rect)
{
    uint32_t rect_x1 = (uint32_t)(rect->XH >> 2);
    uint32_t rect_y1 = (uint32_t)(rect->YH >> 2);
    uint32_t rect_x2 = (uint32_t)(rect->XL >> 2) + 1;
    uint32_t rect_y2 = (uint32_t)(rect->YL >> 2) + 1;

    uint32_t rect_scanbuffer[SCANBUFFER_HEIGHT * 2];
    memset(rect_scanbuffer, 0, (SCANBUFFER_HEIGHT * 2) * sizeof(uint32_t));

    for (uint32_t y = rect_y1; y < rect_y2; ++y)
    {
        rect_scanbuffer[(y * 2)    ] = rect_x1;
        rect_scanbuffer[(y * 2) + 1] = rect_x2;
    }

    draw_scanbuffer(rect_scanbuffer, NULL, NULL);
}