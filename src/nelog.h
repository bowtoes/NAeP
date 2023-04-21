/* Copyright (c), bowtoes (bow.toes@mailfence.com)
Apache 2.0 license, http://www.apache.org/licenses/LICENSE-2.0
Full license can be found in 'license' file */

#ifndef NAeP_nelog_h
#define NAeP_nelog_h

#include <brrtools/brrlog.h>

extern int neerror; // todo maybe

int
print_usage(void);
int
print_help(void);

extern brrlog_style_t style_extra_manual;
extern brrlog_style_t style_extra_inplace;
extern brrlog_style_t style_extra_separate;

extern brrlog_style_t style_extra_enabled;
extern brrlog_style_t style_extra_disabled;
extern brrlog_style_t style_extra_path;
extern brrlog_style_t style_extra_info;

extern brrlog_style_t style_meta_success;
extern brrlog_style_t style_meta_failure;
extern brrlog_style_t style_meta_dry;
extern brrlog_style_t style_meta_wet;

#define nest_normal ""

#define nest_meta_success "f=gs=b"
#define nest_meta_failure "f=rs=b"
#define nest_meta_dry "f=ys=b"
#define nest_meta_wet "f=c"

#define nest_extra_manual "f=r"
#define nest_extra_inplace "f=y"
#define nest_extra_separate "f=c"

#define nest_extra_enabled "f=g"
#define nest_extra_disabled "f=r"
#define nest_extra_path "f=c"
#define nest_extra_info "f=m"

#define nest_filetype_ogg  "f=b"
#define nest_filetype_wem  "f=g"
#define nest_filetype_wsp  "f=y"
#define nest_filetype_bnk  "f=r"
#define nest_filetype_auto "f=ms=b"

#define nemessage_len 4096
extern char nemessage[nemessage_len + 1];

#define logpri_critical 1
#define logpri_error    2
#define logpri_warning  3
#define logpri_normal   4
#define logpri_info     5
#define logpri_debug    6
#define logpri_programmer_error 0

#define logpri_min logpri_programmer_error
#define logpri_max logpri_debug
#define logpri_default logpri_normal

int
nelog_init(int style_enabled);

#ifdef Ne_extra_debug
#define XLog(...) brrlog(__VA_ARGS__)
#define SXLog(...) brrlogs(__VA_ARGS__)
#else
#define XLog(...)
#define SXLog(...)
#endif

#ifdef Ne_zero_debug
#define Pro(_t_, ...)
#else
#define Pro(_t_, ...)      brrlog(_t_, logpri_programmer_error, __VA_ARGS__)
#endif

#define Cri(_t_, ...)      brrlog(_t_, logpri_critical, __VA_ARGS__)
#define Err(_t_, ...)      brrlog(_t_, logpri_error,    __VA_ARGS__)
#define War(_t_, ...)      brrlog(_t_, logpri_warning,  __VA_ARGS__)
#define Nor(_t_, ...)      brrlog(_t_, logpri_normal,   __VA_ARGS__)
#define Inf(_t_, ...)      brrlog(_t_, logpri_info,     __VA_ARGS__)
#define Deb(_t_, ...)      brrlog(_t_, logpri_debug,    __VA_ARGS__)

#define XPro(...) Pro(__VA_ARGS__)
#define XCri( _t_, ...)      XLog(_t_, logpri_critical, __VA_ARGS__)
#define XErr( _t_, ...)      XLog(_t_, logpri_error,    __VA_ARGS__)
#define XWar( _t_, ...)      XLog(_t_, logpri_warning,  __VA_ARGS__)
#define XNor( _t_, ...)      XLog(_t_, logpri_normal,   __VA_ARGS__)
#define XInf( _t_, ...)      XLog(_t_, logpri_info,     __VA_ARGS__)
#define XDeb( _t_, ...)      XLog(_t_, logpri_debug,    __VA_ARGS__)

#define ExtraPlace() do { ExtraDeb(,"Function '%s', in '%s' @ %i", __func__, __FILE__, __LINE__); } while (0)

#endif /* nelog_h */
