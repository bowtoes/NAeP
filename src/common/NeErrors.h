#ifndef NeErrors_h
#define NeErrors_h

/* Generic Errors  */
#define NeERGNONE       0 /* no error */
#define NeERGINVALID   -1 /* invalid input */

/* File Errors */
#define NeERFEXIST     -2 /* file doesn't exist */
#define NeERFTYPE      -3 /* file isn't regular */
#define NeERFSTAT      -4 /* unspecified stat error */
#define NeERFMODE      -5 /* invalid mode */
#define NeERFOPEN      -6 /* could not open */
#define NeERFREAD      -7 /* unspecified read error */
#define NeERFREADP     -8 /* read permission error */
#define NeERFWRITE     -9 /* unspecified write error */
#define NeERFWRITEP   -10 /* write permission error */
#define NeERFREMOVE   -11 /* unspecified remove error */
#define NeERFRENAME   -12 /* unspecified rename error */
#define NeERFFILE     -14 /* unspecified file error */

/* Wisp Errors */

#define NeERWEMPTY    -15 /* empty wisp */
#define NeERWREAD     -16 /* generic wisp read error */
#define NeERWSIZE     -17 /* weem size error */


/* Revorb Errors */
#define NeERRREVORB -13 /* unspecified revorb error */

#endif /* NeErrors_h */
