#ifndef NeErrors_h
#define NeErrors_h

typedef enum {
/* Generic Errors  */
	NeERGNONE,        // No error.
	NeERGINVALID,     // Invalid input.

/* File Errors */
	NeERFEXIST,       // File doesn't exist.
	NeERFPATH,        // Invalid file path.
	NeERFTYPE,        // File isn't regular.
	NeERFSTAT,        // Unspecified stat error.
	NeERFMODE,        // Invalid mode.
	NeERFOPEN,        // Could not open.
	NeERFClose,       // Failed to close.
	NeERFREAD,        // Unspecified read error.
	NeERFREADP,       // Read permission error.
	NeERFWRITE,       // Unspecified write error.
	NeERFWRITEP,      // Write permission error.
	NeERFREMOVE,      // Unspecified remove error.
	NeERFRENAME,      // Unspecified rename error.
	NeERFFILE,        // Unspecified file error.

/* Wisp Errors */

	NeERWEMPTY,       // Empty wisp.
	NeERWREAD,        // Generic wisp read error.
	NeERWSIZE,        // Weem size error.

/* Revorb Errors */
	NeERRREVORB,      // Unspecified revorb error.
} NeErr;

#endif /* NeErrors_h */
