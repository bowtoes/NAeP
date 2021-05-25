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

#ifndef NeErrors_h
#define NeErrors_h

typedef enum NeErr {
	NeERGNONE    , // No error.
	NeERGINVALID , // Invalid input.

	NeERFEXIST   , // File doesn't exist.
	NeERFPATH    , // Invalid file path.
	NeERFTYPE    , // File isn't regular.
	NeERFSTAT    , // Unspecified stat error.
	NeERFMODE    , // Invalid mode.
	NeERFOPEN    , // Could not open.
	NeERFClose   , // Failed to close.
	NeERFREAD    , // Unspecified read error.
	NeERFREADP   , // Read permission error.
	NeERFWRITE   , // Unspecified write error.
	NeERFWRITEP  , // Write permission error.
	NeERFREMOVE  , // Unspecified remove error.
	NeERFRENAME  , // Unspecified rename error.
	NeERFFILE    , // Unspecified file error.

	NeERWEMPTY   , // Empty wisp.
	NeERWREAD    , // Generic wisp read error.
	NeERWSIZE    , // Weem size error.

	NeERRREVORB  , // Unspecified revorb error.

	NeERMUNSPECIFIED  , // Unspecified error.
} NeErrT;

#endif /* NeErrors_h */
