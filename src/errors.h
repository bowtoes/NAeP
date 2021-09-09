#ifndef ERROR_CODES_H
#define ERROR_CODES_H

/* Common internal return codes */
#define I_SUCCESS 0
#define I_BUFFER_ERROR -1
#define I_IO_ERROR -2
#define I_FILE_TRUNCATED -3
#define I_INIT_ERROR -4
#define I_NOT_VORBIS -5
#define I_DESYNC -6
#define I_CORRUPT -7
#define I_NOT_RIFF -8
#define I_UNRECOGNIZED_DATA -9
#define I_INSUFFICIENT_DATA -10
#define I_BAD_ERROR -99

/* Ogg/Vorbis function return codes */
#define SYNC_PAGEOUT_SUCCESS 1
#define SYNC_PAGEOUT_INCOMPLETE 0
#define SYNC_PAGEOUT_DESYNC -1
#define SYNC_WROTE_SUCCESS 0
#define SYNC_WROTE_FAILURE -1
#define STREAM_INIT_SUCCESS 0
#define STREAM_PAGEIN_SUCCESS 0
#define STREAM_PACKETIN_SUCCESS 0
#define STREAM_PACKETOUT_SUCCESS 1
#define STREAM_PACKETOUT_INCOMPLETE 0
#define STREAM_PACKETOUT_DESYNC -1
#define VORBIS_SYNTHESIS_HEADERIN_SUCCESS 0
#define VORBIS_SYNTHESIS_HEADERIN_FAULT OV_EFAULT
#define VORBIS_SYNTHESIS_HEADERIN_NOTVORBIS OV_ENOTVORBIS
#define VORBIS_SYNTHESIS_HEADERIN_BADHEADER OV_EBADHEADER

/* RIFF function returns */
#define RIFF_BUFFER_APPLY_SUCCESS 0
#define RIFF_BUFFER_APPLY_FAILURE -1

#endif /* ERROR_CODES_H */
