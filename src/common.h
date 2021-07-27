/*
Copyright 2021 BowToes (bow.toes@mailfence.com)

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
