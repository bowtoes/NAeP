/*
Copyright 2021-2022 BowToes (bow.toes@mailfence.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef NAeP_nelog_h
#define NAeP_nelog_h

#include <brrtools/brrlog.h>

extern int neerror; // todo maybe

int
print_usage(void);
int
print_help(void);

#define nestylefg_e(_type_) nestylefg_##_type_
#define nestylebg_e(_type_) nestylebg_##_type_
#define nestylest_e(_type_) nestylest_##_type_
#define nestylefn_e(_type_) nestylefn_##_type_
#define nestylefg(_type_) ((brrlog_color_t)nestylefg_e(_type_))
#define nestylebg(_type_) ((brrlog_color_t)nestylebg_e(_type_))
#define nestylest(_type_) ((brrlog_style_t)nestylest_e(_type_))
#define nestylefn(_type_) ((brrlog_font_t)nestylefn_e(_type_))
#define nestyleprm(_type_) nestylefg(_type_), nestylebg(_type_), nestylest(_type_), nestylefn(_type_)
#define nestylefmt(_type_) ((brrlog_format_t){nestyleprm(_type_)})

#define _style_def(_type_, _fg_, _bg_, _st_, _fn_) \
typedef enum nestyle_##_type_ {\
	nestylefg_e(_type_) = brrlog_color_##_fg_,\
	nestylebg_e(_type_) = brrlog_color_##_bg_,\
	nestylest_e(_type_) = brrlog_style_##_st_,\
	nestylefn_e(_type_) = brrlog_font_##_fn_,\
} nestyle_##_type_##_t

_style_def(normal, normal, normal, normal, normal);
_style_def(last,   last,   last,   last,   last);

_style_def(ft_ogg,  blue,    normal, normal, normal);
_style_def(ft_wem,  green,   normal, normal, normal);
_style_def(ft_wsp,  yellow,  normal, normal, normal);
_style_def(ft_bnk,  red,     normal, normal, normal);
_style_def(ft_auto, magenta, normal, bold,   normal);

_style_def(meta_success, green,  normal, bold,   normal);
_style_def(meta_failure, red,    normal, bold,   normal);
_style_def(meta_dry,     yellow, normal, bold,   normal);
_style_def(meta_wet,     cyan,   normal, normal, normal);

_style_def(extra_manual,   red,     normal, normal, normal);
_style_def(extra_inplace,  yellow,  normal, normal, normal);
_style_def(extra_separate, cyan,    normal, normal, normal);

_style_def(extra_enabled,  green,   normal, normal, normal);
_style_def(extra_disabled, red,     normal, normal, normal);
_style_def(extra_path,     cyan,    normal, normal, normal);
_style_def(extra_info,     magenta, normal, normal, normal);

#define _log(_level_, _style_, ...)   BRRLOG_MESSAGET(gbrrlog_level(_level_), nestylefmt(_style_), __VA_ARGS__)
#define _logn(_level_, _style_, ...)  BRRLOG_MESSAGETN(gbrrlog_level(_level_), nestylefmt(_style_), __VA_ARGS__)
#define _logp(_level_, _style_, ...)  BRRLOG_MESSAGETP(gbrrlog_level(_level_), nestylefmt(_style_), __VA_ARGS__)
#define _lognp(_level_, _style_, ...) BRRLOG_MESSAGETNP(gbrrlog_level(_level_), nestylefmt(_style_), __VA_ARGS__)

#define Logg(_variant_, _level_, _style_, ...) _log##_variant_(_level_, _style_, __VA_ARGS__)
#define Style(_variant_, _style_, ...)   Logg(_variant_, last, _style_, __VA_ARGS__)
#define Log(_variant_, _level_, ...)     Logg(_variant_, _level_, last, __VA_ARGS__)

#define Cri(_variant_, ...) Logg(_variant_, critical, normal, __VA_ARGS__)
#define Err(_variant_, ...) Logg(_variant_, error,    normal, __VA_ARGS__)
#define War(_variant_, ...) Logg(_variant_, warning,  normal, __VA_ARGS__)
#define Nor(_variant_, ...) Logg(_variant_, normal,   normal, __VA_ARGS__)
#define Deb(_variant_, ...) Logg(_variant_, debug,    normal, __VA_ARGS__)
#define Lst(_variant_, ...) Logg(_variant_, last,     normal, __VA_ARGS__)

#define SCri(_variant_, ...) Logg(_variant_, critical, __VA_ARGS__)
#define SErr(_variant_, ...) Logg(_variant_, error,    __VA_ARGS__)
#define SWar(_variant_, ...) Logg(_variant_, warning,  __VA_ARGS__)
#define SNor(_variant_, ...) Logg(_variant_, normal,   __VA_ARGS__)
#define SDeb(_variant_, ...) Logg(_variant_, debug,    __VA_ARGS__)
#define SLst(_variant_, ...) Logg(_variant_, last,     __VA_ARGS__)

#ifdef Ne_extra_debug
#define ExtraCri(_variant_, ...) Log(_variant_, critical, __VA_ARGS__)
#define ExtraErr(_variant_, ...) Log(_variant_, error,    __VA_ARGS__)
#define ExtraWar(_variant_, ...) Log(_variant_, warning,  __VA_ARGS__)
#define ExtraNor(_variant_, ...) Log(_variant_, normal,   __VA_ARGS__)
#define ExtraDeb(_variant_, ...) Log(_variant_, debug,    __VA_ARGS__)
#define ExtraLst(_variant_, ...) Log(_variant_, last,     __VA_ARGS__)

#define SExtraCri(_variant_, ...) Logg(_variant_, critical, __VA_ARGS__)
#define SExtraErr(_variant_, ...) Logg(_variant_, error,    __VA_ARGS__)
#define SExtraWar(_variant_, ...) Logg(_variant_, warning,  __VA_ARGS__)
#define SExtraNor(_variant_, ...) Logg(_variant_, normal,   __VA_ARGS__)
#define SExtraDeb(_variant_, ...) Logg(_variant_, debug,    __VA_ARGS__)
#define SExtraLst(_variant_, ...) Logg(_variant_, last,     __VA_ARGS__)
#else
#define ExtraCri(_variant_, ...)
#define ExtraErr(_variant_, ...)
#define ExtraWar(_variant_, ...)
#define ExtraNor(_variant_, ...)
#define ExtraDeb(_variant_, ...)
#define ExtraLst(_variant_, ...)

#define SExtraCri(_variant_, ...)
#define SExtraErr(_variant_, ...)
#define SExtraWar(_variant_, ...)
#define SExtraNor(_variant_, ...)
#define SExtraDeb(_variant_, ...)
#define SExtraLst(_variant_, ...)
#endif

#define ExtraPlace() do { ExtraDeb(,"Function '%s', in '%s' @ %i", __func__, __FILE__, __LINE__); } while (0)

#endif /* nelog_h */
