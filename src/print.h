#ifndef PRINT_H
#define PRINT_H

#include <brrtools/brrlog.h>
#include "input.h"

#define LOG_FORMAT(...) BRRLOG_FONTNP(__VA_ARGS__)

#define LOG_COLOR_OGG  brrlog_color_blue
#define LOG_BGCOL_OGG  brrlog_color_normal
#define LOG_STYLE_OGG  brrlog_style_normal
#define LOG_FONT_OGG   brrlog_font_normal
#define LOG_PARAMS_OGG LOG_COLOR_OGG, LOG_BGCOL_OGG, LOG_STYLE_OGG, LOG_FONT_OGG
#define LOG_FORMAT_OGG ((brrlog_format_t){LOG_PARAMS_OGG})

#define LOG_COLOR_WEM  brrlog_color_green
#define LOG_BGCOL_WEM  brrlog_color_normal
#define LOG_STYLE_WEM  brrlog_style_normal
#define LOG_FONT_WEM   brrlog_font_normal
#define LOG_PARAMS_WEM LOG_COLOR_WEM, LOG_BGCOL_WEM, LOG_STYLE_WEM, LOG_FONT_WEM
#define LOG_FORMAT_WEM ((brrlog_format_t){LOG_PARAMS_WEM})

#define LOG_COLOR_WSP  brrlog_color_yellow
#define LOG_BGCOL_WSP  brrlog_color_normal
#define LOG_STYLE_WSP  brrlog_style_normal
#define LOG_FONT_WSP   brrlog_font_normal
#define LOG_PARAMS_WSP LOG_COLOR_WSP, LOG_BGCOL_WSP, LOG_STYLE_WSP, LOG_FONT_WSP
#define LOG_FORMAT_WSP ((brrlog_format_t){LOG_PARAMS_WSP})

#define LOG_COLOR_BNK  brrlog_color_red
#define LOG_BGCOL_BNK  brrlog_color_normal
#define LOG_STYLE_BNK  brrlog_style_normal
#define LOG_FONT_BNK   brrlog_font_normal
#define LOG_PARAMS_BNK LOG_COLOR_BNK, LOG_BGCOL_BNK, LOG_STYLE_BNK, LOG_FONT_BNK
#define LOG_FORMAT_BNK ((brrlog_format_t){LOG_PARAMS_BNK})

#define LOG_COLOR_AUT  brrlog_color_magenta
#define LOG_BGCOL_AUT  brrlog_color_normal
#define LOG_STYLE_AUT  brrlog_style_bold
#define LOG_FONT_AUT   brrlog_font_normal
#define LOG_PARAMS_AUT LOG_COLOR_AUT, LOG_BGCOL_AUT, LOG_STYLE_AUT, LOG_FONT_AUT
#define LOG_FORMAT_AUT ((brrlog_format_t){LOG_PARAMS_AUT})

#define LOG_COLOR_SUCCESS  brrlog_color_green
#define LOG_BGCOL_SUCCESS  brrlog_color_normal
#define LOG_STYLE_SUCCESS  brrlog_style_bold
#define LOG_FONT_SUCCESS   brrlog_font_normal
#define LOG_PARAMS_SUCCESS LOG_COLOR_SUCCESS, LOG_BGCOL_SUCCESS, LOG_STYLE_SUCCESS, LOG_FONT_SUCCESS
#define LOG_FORMAT_SUCCESS ((brrlog_format_t){LOG_PARAMS_SUCCESS})

#define LOG_COLOR_FAILURE  brrlog_color_red
#define LOG_BGCOL_FAILURE  brrlog_color_normal
#define LOG_STYLE_FAILURE  brrlog_style_bold
#define LOG_FONT_FAILURE   brrlog_font_normal
#define LOG_PARAMS_FAILURE LOG_COLOR_FAILURE, LOG_BGCOL_FAILURE, LOG_STYLE_FAILURE, LOG_FONT_FAILURE
#define LOG_FORMAT_FAILURE ((brrlog_format_t){LOG_PARAMS_FAILURE})

#define LOG_COLOR_DRY  brrlog_color_magenta
#define LOG_BGCOL_DRY  brrlog_color_normal
#define LOG_STYLE_DRY  brrlog_style_normal
#define LOG_FONT_DRY   brrlog_font_normal
#define LOG_PARAMS_DRY LOG_COLOR_DRY, LOG_BGCOL_DRY, LOG_STYLE_DRY, LOG_FONT_DRY
#define LOG_FORMAT_DRY ((brrlog_format_t){LOG_PARAMS_DRY})

#define LOG_COLOR_WET  brrlog_color_cyan
#define LOG_BGCOL_WET  brrlog_color_normal
#define LOG_STYLE_WET  brrlog_style_normal
#define LOG_FONT_WET   brrlog_font_normal
#define LOG_PARAMS_WET LOG_COLOR_WET, LOG_BGCOL_WET, LOG_STYLE_WET, LOG_FONT_WET
#define LOG_FORMAT_WET ((brrlog_format_t){LOG_PARAMS_WET})

#define LOG_COLOR_MANUAL   brrlog_color_red
#define LOG_BGCOL_MANUAL  brrlog_color_normal
#define LOG_STYLE_MANUAL  brrlog_style_normal
#define LOG_FONT_MANUAL   brrlog_font_normal
#define LOG_PARAMS_MANUAL LOG_COLOR_MANUAL, LOG_BGCOL_MANUAL, LOG_STYLE_MANUAL, LOG_FONT_MANUAL
#define LOG_FORMAT_MANUAL ((brrlog_format_t){LOG_PARAMS_MANUAL})

#define LOG_COLOR_INPLACE  brrlog_color_yellow
#define LOG_BGCOL_INPLACE  brrlog_color_normal
#define LOG_STYLE_INPLACE  brrlog_style_normal
#define LOG_FONT_INPLACE   brrlog_font_normal
#define LOG_PARAMS_INPLACE LOG_COLOR_INPLACE, LOG_BGCOL_INPLACE, LOG_STYLE_INPLACE, LOG_FONT_INPLACE
#define LOG_FORMAT_INPLACE ((brrlog_format_t){LOG_PARAMS_INPLACE})

#define LOG_COLOR_SEPARATE  brrlog_color_cyan
#define LOG_BGCOL_SEPARATE  brrlog_color_normal
#define LOG_STYLE_SEPARATE  brrlog_style_normal
#define LOG_FONT_SEPARATE   brrlog_font_normal
#define LOG_PARAMS_SEPARATE LOG_COLOR_SEPARATE, LOG_BGCOL_SEPARATE, LOG_STYLE_SEPARATE, LOG_FONT_SEPARATE
#define LOG_FORMAT_SEPARATE ((brrlog_format_t){LOG_PARAMS_SEPARATE})

#define LOG_COLOR_ENABLED  brrlog_color_green
#define LOG_BGCOL_ENABLED  brrlog_color_normal
#define LOG_STYLE_ENABLED  brrlog_style_normal
#define LOG_FONT_ENABLED   brrlog_font_normal
#define LOG_PARAMS_ENABLED LOG_COLOR_ENABLED, LOG_BGCOL_ENABLED, LOG_STYLE_ENABLED, LOG_FONT_ENABLED
#define LOG_FORMAT_ENABLED ((brrlog_format_t){LOG_PARAMS_ENABLED})

#define LOG_COLOR_DISABLED  brrlog_color_red
#define LOG_BGCOL_DISABLED  brrlog_color_normal
#define LOG_STYLE_DISABLED  brrlog_style_normal
#define LOG_FONT_DISABLED   brrlog_font_normal
#define LOG_PARAMS_DISABLED LOG_COLOR_DISABLED, LOG_BGCOL_DISABLED, LOG_STYLE_DISABLED, LOG_FONT_DISABLED
#define LOG_FORMAT_DISABLED ((brrlog_format_t){LOG_PARAMS_DISABLED})

#define LOG_COLOR_PATH  brrlog_color_cyan
#define LOG_BGCOL_PATH  brrlog_color_normal
#define LOG_STYLE_PATH  brrlog_style_normal
#define LOG_FONT_PATH   brrlog_font_normal
#define LOG_PARAMS_PATH LOG_COLOR_PATH, LOG_BGCOL_PATH, LOG_STYLE_PATH, LOG_FONT_PATH
#define LOG_FORMAT_PATH ((brrlog_format_t){LOG_PARAMS_PATH})

#define LOG_COLOR_INFO  brrlog_color_magenta
#define LOG_BGCOL_INFO  brrlog_color_normal
#define LOG_STYLE_INFO  brrlog_style_normal
#define LOG_FONT_INFO   brrlog_font_normal
#define LOG_PARAMS_INFO LOG_COLOR_INFO, LOG_BGCOL_INFO, LOG_STYLE_INFO, LOG_FONT_INFO
#define LOG_FORMAT_INFO ((brrlog_format_t){LOG_PARAMS_INFO})

int print_usage(void);
int print_help(void);
int print_report(const nestate_t *const state);

#endif /* PRINT_H */
