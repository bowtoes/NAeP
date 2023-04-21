#ifndef NAeP_riff_extension_h
#define NAeP_riff_extension_h

/* Add extended support for different chunk types, BASICS being basic chunks and ROOTS being root chunks.
 * '_processor_' is the generic processor that will take the chunk type ('basic' or 'root') and chunk name to
 * generate enum values and strings for said types*/

#define RIFF_EXTENDED_BASICS(_processor_)\
	_processor_(basic,vorb)\

#define RIFF_EXTENDED_ROOTS(_processor_)\
	_processor_(root,BKHD)\

#endif /* NAeP_riff_extension_h */
