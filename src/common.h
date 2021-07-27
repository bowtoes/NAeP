#ifndef COMMON_H
#define COMMON_H

#if defined(BRRTOOLS_BRRLOG_H)
# define NeTODO(...) do { \
	BRRLOG_DEBUGNP(""); \
	BRRLOG_FONTNP(brrlog_color_green, brrlog_color_normal, -1, -1, " TODO:"); \
	BRRLOG_FONTNP(brrlog_color_normal, brrlog_color_normal, -1, -1, __VA_ARGS__); \
} while (0)
#endif /* BRRTOOLS_BRRLOG_H */

#endif /* COMMON_H */
