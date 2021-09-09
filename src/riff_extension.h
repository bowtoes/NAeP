#ifndef RIFF_EXTENSION_H
#define RIFF_EXTENSION_H

#define RIFF_EXTENDED_BASICS(_processor_) \
	/* Wwise WEM */ \
	_processor_(basic,akd) \
	_processor_(basic,vorb) \
	/* Wwise BNK */ \

#define RIFF_EXTENDED_FORMATS(_processor_) \
	_processor_(format,BKHD) \

#endif /* RIFF_EXTENSION_H */
