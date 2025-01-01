//=============================================================================//
//
// Purpose: streamed & buffered pak decoder
//
//=============================================================================//
#include "tier0/binstream.h"
#include "tier1/fmtstr.h"

#include "rtech/ipakfile.h"

#include "paktools.h"
#include "pakdecode.h"

//-----------------------------------------------------------------------------
// lookup table for default pak decoder
//-----------------------------------------------------------------------------
static const unsigned char /*141313180*/ s_defaultDecoderLUT[] =
{
	0x04, 0xFE, 0xFC, 0x08, 0x04, 0xEF, 0x11, 0xF9, 0x04, 0xFD, 0xFC, 0x07, 0x04, 0x05, 0xFF, 0xF4,
	0x04, 0xFE, 0xFC, 0x10, 0x04, 0xEF, 0x11, 0xF6, 0x04, 0xFD, 0xFC, 0xFB, 0x04, 0x06, 0xFF, 0x0B,
	0x04, 0xFE, 0xFC, 0x08, 0x04, 0xEF, 0x11, 0xF8, 0x04, 0xFD, 0xFC, 0x0C, 0x04, 0x05, 0xFF, 0xF7,
	0x04, 0xFE, 0xFC, 0x10, 0x04, 0xEF, 0x11, 0xF5, 0x04, 0xFD, 0xFC, 0xFA, 0x04, 0x06, 0xFF, 0xF3,
	0x04, 0xFE, 0xFC, 0x08, 0x04, 0xEF, 0x11, 0xF9, 0x04, 0xFD, 0xFC, 0x07, 0x04, 0x05, 0xFF, 0xF4,
	0x04, 0xFE, 0xFC, 0x10, 0x04, 0xEF, 0x11, 0xF6, 0x04, 0xFD, 0xFC, 0xFB, 0x04, 0x06, 0xFF, 0x0E,
	0x04, 0xFE, 0xFC, 0x08, 0x04, 0xEF, 0x11, 0xF8, 0x04, 0xFD, 0xFC, 0x0C, 0x04, 0x05, 0xFF, 0x09,
	0x04, 0xFE, 0xFC, 0x10, 0x04, 0xEF, 0x11, 0xF5, 0x04, 0xFD, 0xFC, 0xFA, 0x04, 0x06, 0xFF, 0xF1,
	0x04, 0xFE, 0xFC, 0x08, 0x04, 0xEF, 0x11, 0xF9, 0x04, 0xFD, 0xFC, 0x07, 0x04, 0x05, 0xFF, 0xF4,
	0x04, 0xFE, 0xFC, 0x10, 0x04, 0xEF, 0x11, 0xF6, 0x04, 0xFD, 0xFC, 0xFB, 0x04, 0x06, 0xFF, 0x0D,
	0x04, 0xFE, 0xFC, 0x08, 0x04, 0xEF, 0x11, 0xF8, 0x04, 0xFD, 0xFC, 0x0C, 0x04, 0x05, 0xFF, 0xF7,
	0x04, 0xFE, 0xFC, 0x10, 0x04, 0xEF, 0x11, 0xF5, 0x04, 0xFD, 0xFC, 0xFA, 0x04, 0x06, 0xFF, 0xF2,
	0x04, 0xFE, 0xFC, 0x08, 0x04, 0xEF, 0x11, 0xF9, 0x04, 0xFD, 0xFC, 0x07, 0x04, 0x05, 0xFF, 0xF4,
	0x04, 0xFE, 0xFC, 0x10, 0x04, 0xEF, 0x11, 0xF6, 0x04, 0xFD, 0xFC, 0xFB, 0x04, 0x06, 0xFF, 0x0F,
	0x04, 0xFE, 0xFC, 0x08, 0x04, 0xEF, 0x11, 0xF8, 0x04, 0xFD, 0xFC, 0x0C, 0x04, 0x05, 0xFF, 0x0A,
	0x04, 0xFE, 0xFC, 0x10, 0x04, 0xEF, 0x11, 0xF5, 0x04, 0xFD, 0xFC, 0xFA, 0x04, 0x06, 0xFF, 0xF0,
	0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x07, 0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x11,
	0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x08, 0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x0C,
	0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x07, 0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x09,
	0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x08, 0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x0E,
	0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x07, 0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x11,
	0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x08, 0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x0B,
	0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x07, 0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x0A,
	0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x08, 0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x10,
	0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x07, 0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x11,
	0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x08, 0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x0C,
	0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x07, 0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x09,
	0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x08, 0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x0F,
	0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x07, 0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x11,
	0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x08, 0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x0D,
	0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x07, 0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x0A,
	0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0x08, 0x04, 0x05, 0x04, 0x06, 0x04, 0x05, 0x04, 0xFF,
	0x02, 0x04, 0x03, 0x05, 0x02, 0x04, 0x04, 0x06, 0x02, 0x04, 0x03, 0x06, 0x02, 0x05, 0x04, 0x06,
	0x02, 0x04, 0x03, 0x05, 0x02, 0x04, 0x04, 0x06, 0x02, 0x04, 0x03, 0x06, 0x02, 0x05, 0x04, 0x08,
	0x02, 0x04, 0x03, 0x05, 0x02, 0x04, 0x04, 0x06, 0x02, 0x04, 0x03, 0x06, 0x02, 0x05, 0x04, 0x07,
	0x02, 0x04, 0x03, 0x05, 0x02, 0x04, 0x04, 0x06, 0x02, 0x04, 0x03, 0x06, 0x02, 0x05, 0x04, 0x08,
	0x02, 0x04, 0x03, 0x05, 0x02, 0x04, 0x04, 0x06, 0x02, 0x04, 0x03, 0x06, 0x02, 0x05, 0x04, 0x06,
	0x02, 0x04, 0x03, 0x05, 0x02, 0x04, 0x04, 0x06, 0x02, 0x04, 0x03, 0x06, 0x02, 0x05, 0x04, 0x08,
	0x02, 0x04, 0x03, 0x05, 0x02, 0x04, 0x04, 0x06, 0x02, 0x04, 0x03, 0x06, 0x02, 0x05, 0x04, 0x08,
	0x02, 0x04, 0x03, 0x05, 0x02, 0x04, 0x04, 0x06, 0x02, 0x04, 0x03, 0x06, 0x02, 0x05, 0x04, 0x08,
	0x02, 0x04, 0x03, 0x05, 0x02, 0x04, 0x04, 0x06, 0x02, 0x04, 0x03, 0x06, 0x02, 0x05, 0x04, 0x06,
	0x02, 0x04, 0x03, 0x05, 0x02, 0x04, 0x04, 0x06, 0x02, 0x04, 0x03, 0x06, 0x02, 0x05, 0x04, 0x08,
	0x02, 0x04, 0x03, 0x05, 0x02, 0x04, 0x04, 0x06, 0x02, 0x04, 0x03, 0x06, 0x02, 0x05, 0x04, 0x07,
	0x02, 0x04, 0x03, 0x05, 0x02, 0x04, 0x04, 0x06, 0x02, 0x04, 0x03, 0x06, 0x02, 0x05, 0x04, 0x08,
	0x02, 0x04, 0x03, 0x05, 0x02, 0x04, 0x04, 0x06, 0x02, 0x04, 0x03, 0x06, 0x02, 0x05, 0x04, 0x06,
	0x02, 0x04, 0x03, 0x05, 0x02, 0x04, 0x04, 0x06, 0x02, 0x04, 0x03, 0x06, 0x02, 0x05, 0x04, 0x08,
	0x02, 0x04, 0x03, 0x05, 0x02, 0x04, 0x04, 0x06, 0x02, 0x04, 0x03, 0x06, 0x02, 0x05, 0x04, 0x08,
	0x02, 0x04, 0x03, 0x05, 0x02, 0x04, 0x04, 0x06, 0x02, 0x04, 0x03, 0x06, 0x02, 0x05, 0x04, 0x08,
	0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x05, 0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x06,
	0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x05, 0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x07,
	0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x05, 0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x07,
	0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x05, 0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x08,
	0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x05, 0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x06,
	0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x05, 0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x08,
	0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x05, 0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x07,
	0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x05, 0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x08,
	0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x05, 0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x06,
	0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x05, 0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x07,
	0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x05, 0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x07,
	0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x05, 0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x08,
	0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x05, 0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x06,
	0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x05, 0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x08,
	0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x05, 0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x07,
	0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x05, 0x01, 0x02, 0x01, 0x03, 0x01, 0x02, 0x01, 0x08,
	0x00, 0x08, 0x00, 0x04, 0x00, 0x08, 0x00, 0x06, 0x00, 0x08, 0x00, 0x01, 0x00, 0x08, 0x00, 0x0B,
	0x00, 0x08, 0x00, 0x0C, 0x00, 0x08, 0x00, 0x09, 0x00, 0x08, 0x00, 0x03, 0x00, 0x08, 0x00, 0x0E,
	0x00, 0x08, 0x00, 0x04, 0x00, 0x08, 0x00, 0x07, 0x00, 0x08, 0x00, 0x02, 0x00, 0x08, 0x00, 0x0D,
	0x00, 0x08, 0x00, 0x0C, 0x00, 0x08, 0x00, 0x0A, 0x00, 0x08, 0x00, 0x05, 0x00, 0x08, 0x00, 0x0F,
	0x01, 0x02, 0x01, 0x05, 0x01, 0x02, 0x01, 0x06, 0x01, 0x02, 0x01, 0x06, 0x01, 0x02, 0x01, 0x06,
	0x01, 0x02, 0x01, 0x05, 0x01, 0x02, 0x01, 0x06, 0x01, 0x02, 0x01, 0x06, 0x01, 0x02, 0x01, 0x06,
	0x01, 0x02, 0x01, 0x05, 0x01, 0x02, 0x01, 0x06, 0x01, 0x02, 0x01, 0x06, 0x01, 0x02, 0x01, 0x06,
	0x01, 0x02, 0x01, 0x05, 0x01, 0x02, 0x01, 0x06, 0x01, 0x02, 0x01, 0x06, 0x01, 0x02, 0x01, 0x06,
	0x4A, 0x00, 0x00, 0x00, 0x6A, 0x00, 0x00, 0x00, 0x8A, 0x00, 0x00, 0x00, 0xAA, 0x00, 0x00, 0x00,
	0xCA, 0x00, 0x00, 0x00, 0xEA, 0x00, 0x00, 0x00, 0x0A, 0x01, 0x00, 0x00, 0x2A, 0x01, 0x00, 0x00,
	0x4A, 0x01, 0x00, 0x00, 0x6A, 0x01, 0x00, 0x00, 0x8A, 0x01, 0x00, 0x00, 0xAA, 0x01, 0x00, 0x00,
	0xAA, 0x03, 0x00, 0x00, 0xAA, 0x05, 0x00, 0x00, 0xAA, 0x25, 0x00, 0x00, 0xAA, 0x25, 0x02, 0x00,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x09, 0x09, 0x0D, 0x11, 0x15,
	0x00, 0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x2A, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x05, 0x05,
	0x11, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0xBF, 0x00, 0x00, 0x00, 0xBF, 0x00, 0x00, 0x00, 0xBF, 0x00, 0x00, 0x00, 0xBF,
	0xA8, 0xAA, 0x2A, 0xBE, 0xA8, 0xAA, 0x2A, 0xBE, 0xA8, 0xAA, 0x2A, 0xBE, 0xA8, 0xAA, 0x2A, 0xBE,
	0xD2, 0x85, 0x08, 0x3C, 0xD2, 0x85, 0x08, 0x3C, 0xD2, 0x85, 0x08, 0x3C, 0xD2, 0x85, 0x08, 0x3C,
	0x83, 0xF9, 0x22, 0x3F, 0x83, 0xF9, 0x22, 0x3F, 0x83, 0xF9, 0x22, 0x3F, 0x83, 0xF9, 0x22, 0x3F,
	0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x10, 0xC9, 0x3F, 0x00, 0x10, 0xC9, 0x3F, 0x00, 0x10, 0xC9, 0x3F, 0x00, 0x10, 0xC9, 0x3F,
	0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x3F,
	0x02, 0x61, 0x4D, 0xB9, 0x02, 0x61, 0x4D, 0xB9, 0x02, 0x61, 0x4D, 0xB9, 0x02, 0x61, 0x4D, 0xB9,
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
	0xC2, 0x14, 0xCF, 0x37, 0xC2, 0x14, 0xCF, 0x37, 0xC2, 0x14, 0xCF, 0x37, 0xC2, 0x14, 0xCF, 0x37,
	0x9E, 0x4B, 0x6F, 0xB0, 0x9E, 0x4B, 0x6F, 0xB0, 0x9E, 0x4B, 0x6F, 0xB0, 0x9E, 0x4B, 0x6F, 0xB0,
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
	0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0xF1, 0x1D, 0xC1, 0xF6, 0x7F, 0x00, 0x00,
	0x22, 0x0B, 0xB6, 0xBA, 0x22, 0x0B, 0xB6, 0xBA, 0x22, 0x0B, 0xB6, 0xBA, 0x22, 0x0B, 0xB6, 0xBA,
	0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x3F,
	0x02, 0x61, 0x4D, 0xB9, 0x02, 0x61, 0x4D, 0xB9, 0x02, 0x61, 0x4D, 0xB9, 0x02, 0x61, 0x4D, 0xB9,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
	0xC2, 0x14, 0xCF, 0x37, 0xC2, 0x14, 0xCF, 0x37, 0xC2, 0x14, 0xCF, 0x37, 0xC2, 0x14, 0xCF, 0x37,
	0x9E, 0x4B, 0x6F, 0xB0, 0x9E, 0x4B, 0x6F, 0xB0, 0x9E, 0x4B, 0x6F, 0xB0, 0x9E, 0x4B, 0x6F, 0xB0,
	0x22, 0x0B, 0xB6, 0xBA, 0x22, 0x0B, 0xB6, 0xBA, 0x22, 0x0B, 0xB6, 0xBA, 0x22, 0x0B, 0xB6, 0xBA,
	0x00, 0x70, 0x95, 0xB6, 0x00, 0x70, 0x95, 0xB6, 0x00, 0x70, 0x95, 0xB6, 0x00, 0x70, 0x95, 0xB6,
	0xA9, 0xAA, 0x2A, 0x3D, 0xA9, 0xAA, 0x2A, 0x3D, 0xA9, 0xAA, 0x2A, 0x3D, 0xA9, 0xAA, 0x2A, 0x3D,
	0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F,
	0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0xBF, 0x00, 0x00, 0x00, 0xBF, 0x00, 0x00, 0x00, 0xBF, 0x00, 0x00, 0x00, 0xBF,
	0xA8, 0xAA, 0x2A, 0xBE, 0xA8, 0xAA, 0x2A, 0xBE, 0xA8, 0xAA, 0x2A, 0xBE, 0xA8, 0xAA, 0x2A, 0xBE,
	0xD2, 0x85, 0x08, 0x3C, 0xD2, 0x85, 0x08, 0x3C, 0xD2, 0x85, 0x08, 0x3C, 0xD2, 0x85, 0x08, 0x3C,
	0x83, 0xF9, 0x22, 0x3F, 0x83, 0xF9, 0x22, 0x3F, 0x83, 0xF9, 0x22, 0x3F, 0x83, 0xF9, 0x22, 0x3F,
	0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x10, 0xC9, 0x3F, 0x00, 0x10, 0xC9, 0x3F, 0x00, 0x10, 0xC9, 0x3F, 0x00, 0x10, 0xC9, 0x3F,
	0x4C, 0x39, 0x56, 0x75, 0x42, 0x52, 0x65, 0x75, 0x70, 0x35, 0x31, 0x77, 0x4C, 0x51, 0x64, 0x61,
};

//-----------------------------------------------------------------------------
// checks if we have enough output buffer room to decode the data stream
//-----------------------------------------------------------------------------
static bool Pak_HasEnoughDecodeBufferAvailable(PakDecoder_s* const decoder, const size_t outLen)
{
	// make sure caller has copied all data out the ring buffer first before
	// overwriting it with new decoded data
	const uint64_t bytesWritten = (decoder->outBufBytePos & ~decoder->outputInvMask);
	return (outLen >= decoder->outputInvMask + (bytesWritten +1) || outLen >= decoder->decompSize);
}

//-----------------------------------------------------------------------------
// checks if we have enough source data streamed to decode the next block
//-----------------------------------------------------------------------------
static bool Pak_HasEnoughStreamedDataForDecode(PakDecoder_s* const decoder, const size_t inLen)
{
	// the decoder needs at least this amount of input data streamed in order
	// to decode the rest of the pak file, as this is where reading has stopped
	// this value may equal the currently streamed input size, as its possible
	// this function is getting called to flush the remainder decoded data into
	// the out buffer which got truncated off on the call prior due to wrapping
	return (inLen >= decoder->bufferSizeNeeded);
}

//-----------------------------------------------------------------------------
// gets the frame for the data in the ring buffer, the frame returned is always
// ending to the end of the ring buffer, or the end of the data itself
//-----------------------------------------------------------------------------
static PakRingBufferFrame_s Pak_DetermineRingBufferFrame(const uint64_t bufMask, const size_t seekPos, const size_t dataLen)
{
	PakRingBufferFrame_s ring;
	ring.bufIndex = seekPos & bufMask;

	// the total amount of bytes used and available in this frame
	const size_t bytesUsed = ring.bufIndex & bufMask;
	const size_t totalAvail = bufMask +1 - bytesUsed;

	// the last part of the data might be smaller than the remainder of the ring
	// buffer; clamp it
	ring.frameLen = Min(dataLen - seekPos, totalAvail);
	return ring;
}

//-----------------------------------------------------------------------------
// initializes the RTech decoder
//-----------------------------------------------------------------------------
static size_t Pak_RTechDecoderInit(PakDecoder_s* const decoder, const uint8_t* const fileBuffer,
	const uint64_t inputMask, const size_t dataSize, const size_t dataOffset, const size_t headerSize)
{
	uint64_t frameHeader = *(_QWORD*)((inputMask & (dataOffset + headerSize)) + fileBuffer);
	const int decompressedSizeBits = frameHeader & 0x3F;

	frameHeader >>= 6;
	decoder->decompSize = (1i64 << decompressedSizeBits) | frameHeader & ((1i64 << decompressedSizeBits) - 1);

	const uint64_t bytePos = dataOffset + headerSize + 8;
	const int64_t currByteLow = *(_QWORD*)((inputMask & bytePos) + fileBuffer) << (64 - ((uint8_t)decompressedSizeBits + 6));

	decoder->inBufBytePos = bytePos + ((uint64_t)(uint32_t)(decompressedSizeBits + 6) >> 3);
	const uint32_t bitPosFinal = ((decompressedSizeBits + 6) & 7) + 13;

	const uint64_t currByte = (0xFFFFFFFFFFFFFFFFui64 >> ((decompressedSizeBits + 6) & 7)) & ((frameHeader >> decompressedSizeBits) | currByteLow);
	const uint32_t currbits = (((_BYTE)currByte - 1) & 0x3F) + 1;

	const uint64_t invMaskIn = 0xFFFFFFFFFFFFFFFFui64 >> (64 - (uint8_t)currbits);
	decoder->inputInvMask = invMaskIn;

	const uint64_t invMaskOut = 0xFFFFFFFFFFFFFFFFui64 >> (64 - ((((currByte >> 6) - 1) & 0x3F) + 1));
	decoder->outputInvMask = invMaskOut;

	const uint64_t finalByteFull = (currByte >> 13) | (*(_QWORD*)((inputMask & decoder->inBufBytePos) + fileBuffer) << (64 - (uint8_t)bitPosFinal));
	const uint32_t finalBitOffset = bitPosFinal & 7;

	decoder->inBufBytePos += bitPosFinal >> 3;
	const uint64_t finalByte = (0xFFFFFFFFFFFFFFFFui64 >> finalBitOffset) & finalByteFull;

	if (decoder->inputInvMask == 0xFFFFFFFFFFFFFFFFui64)
	{
		decoder->headerOffset = 0;
		decoder->bufferSizeNeeded = dataSize;
	}
	else
	{
		const uint64_t finalPos = inputMask & decoder->inBufBytePos;
		decoder->headerOffset = (currbits >> 3) + 1;
		decoder->inBufBytePos += (currbits >> 3) + 1;
		decoder->bufferSizeNeeded = *(_QWORD*)(finalPos + fileBuffer) & ((1i64 << (8 * ((uint8_t)(currbits >> 3) + 1))) - 1);
	}

	decoder->bufferSizeNeeded += dataOffset;
	decoder->currentByte = finalByte;
	decoder->currentBit = finalBitOffset;
	decoder->qword70 = decoder->inputInvMask + dataOffset - 6;
	decoder->dword6C = 0;
	decoder->compressedStreamSize = decoder->bufferSizeNeeded;
	decoder->decompressedStreamSize = decoder->decompSize;

	if ((((uint8_t)(currByte >> 6) - 1) & 0x3F) != -1i64 && decoder->decompSize - 1 > decoder->outputInvMask)
	{
		const uint64_t streamCompressedSize = decoder->bufferSizeNeeded - decoder->headerOffset;
		decoder->compressedStreamSize = streamCompressedSize;

		decoder->decompressedStreamSize = decoder->outputInvMask + 1;
	}

	return decoder->decompSize;
}

//-----------------------------------------------------------------------------
// decodes the RTech data stream up to available buffer or data
//-----------------------------------------------------------------------------
static bool Pak_RTechStreamDecode(PakDecoder_s* const decoder, const size_t inLen, const size_t outLen)
{
	bool result; // al
	uint64_t outBufBytePos; // r15
	uint8_t* outputBuf; // r11
	uint32_t currentBit; // ebp
	uint64_t currentByte; // rsi
	uint64_t inBufBytePos; // rdi
	size_t qword70; // r12
	const uint8_t* inputBuf; // r13
	uint32_t dword6C; // ecx
	uint64_t v13; // rsi
	unsigned __int64 i; // rax
	unsigned __int64 v15; // r8
	__int64 v16; // r9
	int v17; // ecx
	unsigned __int64 v18; // rax
	uint64_t v19; // rsi
	__int64 v20; // r14
	int v21; // ecx
	unsigned __int64 v22; // r11
	int v23; // edx
	uint64_t outputMask; // rax
	int v25; // r8d
	unsigned int v26; // r13d
	uint64_t v27; // r10
	uint8_t* v28; // rax
	uint8_t* v29; // r10
	size_t decompSize; // r9
	uint64_t inputInvMask; // r10
	uint64_t headerOffset; // r8
	uint64_t v33; // rax
	uint64_t v34; // rax
	uint64_t v35; // rax
	size_t v36; // rcx
	__int64 v37; // rdx
	size_t v38; // r14
	size_t v39; // r11
	uint64_t v40; // cl
	uint64_t v41; // rsi
	__int64 v42; // rcx
	uint64_t v43; // r8
	int v44; // r11d
	unsigned __int8 v45; // r9
	uint64_t v46; // rcx
	uint64_t v47; // rcx
	__int64 v48; // r9
	__int64 m; // r8
	__int64 v50; // r9d
	__int64 v51; // r8
	__int64 v52; // rdx
	__int64 k; // r8
	signed __int64 v54; // r10
	__int64 v55; // rdx
	unsigned int v56; // r14d
	const uint8_t* v57; // rdx
	uint8_t* v58; // r8
	uint64_t v59; // al
	uint64_t v60; // rsi
	__int64 v61; // rax
	uint64_t v62; // r9
	int v63; // r10d
	unsigned __int8 v64; // cl
	uint64_t v65; // rax
	unsigned int v66; // r14d
	unsigned int j; // ecx
	__int64 v68; // rax
	uint64_t v69; // rcx
	uint8_t* v70; // [rsp+0h] [rbp-58h]
	uint32_t v71; // [rsp+60h] [rbp+8h]
	const uint8_t* v74; // [rsp+78h] [rbp+20h]

	outBufBytePos = decoder->outBufBytePos;

	outputBuf = decoder->outputBuf;
	currentBit = decoder->currentBit;
	currentByte = decoder->currentByte;
	inBufBytePos = decoder->inBufBytePos;
	qword70 = decoder->qword70;
	inputBuf = decoder->inputBuf;

	if (decoder->compressedStreamSize < qword70)
		qword70 = decoder->compressedStreamSize;

	dword6C = decoder->dword6C;
	v74 = inputBuf;
	v70 = outputBuf;
	v71 = dword6C;
	if (!currentBit)
		goto LABEL_11;

	v13 = (*(_QWORD*)&inputBuf[inBufBytePos & decoder->inputMask] << (64 - (unsigned __int8)currentBit)) | currentByte;
	for (i = currentBit; ; i = currentBit)
	{
		currentBit &= 7u;
		inBufBytePos += i >> 3;
		dword6C = v71;
		currentByte = (0xFFFFFFFFFFFFFFFFui64 >> currentBit) & v13;
	LABEL_11:
		v15 = (unsigned __int64)dword6C << 8;
		v16 = dword6C;
		v17 = s_defaultDecoderLUT[(unsigned __int8)currentByte + 512 + v15];
		v18 = (unsigned __int8)currentByte + v15;
		currentBit += v17;
		v19 = currentByte >> v17;
		v20 = (unsigned int)(char)s_defaultDecoderLUT[v18];
		if ((s_defaultDecoderLUT[v18] & 0x80u) != 0)
		{
			v56 = -(int)v20;
			v57 = &inputBuf[inBufBytePos & decoder->inputMask];
			v71 = 1;
			v58 = &outputBuf[outBufBytePos & decoder->outputMask];
			if (v56 == s_defaultDecoderLUT[v16 + 1248])
			{
				if ((~inBufBytePos & decoder->inputInvMask) < 0xF || (decoder->outputInvMask & ~outBufBytePos) < 0xF || decoder->decompSize - outBufBytePos < 0x10)
					v56 = 1;
				v59 = v19;
				v60 = v19 >> 3;
				v61 = v59 & 7;
				v62 = v60;
				if (v61)
				{
					v63 = s_defaultDecoderLUT[v61 + 1232];
					v64 = s_defaultDecoderLUT[v61 + 1240];
				}
				else
				{
					v62 = v60 >> 4;
					v65 = v60 & 0xF;
					currentBit += 4;
					v63 = *(_DWORD*)&s_defaultDecoderLUT[4 * v65 + 1152];
					v64 = s_defaultDecoderLUT[v65 + 1216];
				}
				currentBit += v64 + 3;
				v19 = v62 >> v64;
				v66 = v63 + (v62 & ((1 << v64) - 1)) + v56;
				for (j = v66 >> 3; j; --j)
				{
					v68 = *(_QWORD*)v57;
					v57 += 8;
					*(_QWORD*)v58 = v68;
					v58 += 8;
				}
				if ((v66 & 4) != 0)
				{
					*(_DWORD*)v58 = *(_DWORD*)v57;
					v58 += 4;
					v57 += 4;
				}
				if ((v66 & 2) != 0)
				{
					*(_WORD*)v58 = *(_WORD*)v57;
					v58 += 2;
					v57 += 2;
				}
				if ((v66 & 1) != 0)
					*v58 = *v57;
				inBufBytePos += v66;
				outBufBytePos += v66;
			}
			else
			{
				*(_QWORD*)v58 = *(_QWORD*)v57;
				*((_QWORD*)v58 + 1) = *((_QWORD*)v57 + 1);
				inBufBytePos += v56;
				outBufBytePos += v56;
			}
		}
		else
		{
			v21 = v19 & 0xF;
			v71 = 0;
			v22 = ((unsigned __int64)(unsigned int)v19 >> (((unsigned int)(v21 - 31) >> 3) & 6)) & 0x3F;
			v23 = 1 << (v21 + ((v19 >> 4) & ((24 * (((unsigned int)(v21 - 31) >> 3) & 2)) >> 4)));
			currentBit += (((unsigned int)(v21 - 31) >> 3) & 6) + s_defaultDecoderLUT[v22 + 1088] + v21 + ((v19 >> 4) & ((24 * (((unsigned int)(v21 - 31) >> 3) & 2)) >> 4));
			outputMask = decoder->outputMask;
			v25 = 16 * (v23 + ((v23 - 1) & (v19 >> ((((unsigned int)(v21 - 31) >> 3) & 6) + s_defaultDecoderLUT[v22 + 1088]))));
			v19 >>= (((unsigned int)(v21 - 31) >> 3) & 6) + s_defaultDecoderLUT[v22 + 1088] + v21 + ((v19 >> 4) & ((24 * (((unsigned int)(v21 - 31) >> 3) & 2)) >> 4));
			v26 = v25 + s_defaultDecoderLUT[v22 + 1024] - 16;
			v27 = outputMask & (outBufBytePos - v26);
			v28 = &v70[outBufBytePos & outputMask];
			v29 = &v70[v27];
			if ((_DWORD)v20 == 17)
			{
				v40 = v19;
				v41 = v19 >> 3;
				v42 = v40 & 7;
				v43 = v41;
				if (v42)
				{
					v44 = s_defaultDecoderLUT[v42 + 1232];
					v45 = s_defaultDecoderLUT[v42 + 1240];
				}
				else
				{
					currentBit += 4;
					v46 = v41 & 0xF;
					v43 = v41 >> 4;
					v44 = *(_DWORD*)&s_defaultDecoderLUT[4 * v46 + 1152];
					v45 = s_defaultDecoderLUT[v46 + 1216];
					if (v74 && currentBit + v45 >= 61)
					{
						v47 = inBufBytePos++ & decoder->inputMask;
						v43 |= (unsigned __int64)v74[v47] << (61 - (unsigned __int8)currentBit);
						currentBit -= 8;
					}
				}
				currentBit += v45 + 3;
				v19 = v43 >> v45;
				v48 = ((unsigned int)v43 & ((1 << v45) - 1)) + v44 + 17;
				outBufBytePos += v48;
				if (v26 < 8)
				{
					v50 = v48 - 13;
					outBufBytePos -= 13i64;
					if (v26 == 1)
					{
						v51 = *v29;
						//++dword_14D40B2BC;
						v52 = 0i64;
						for (k = 0x101010101010101i64 * v51; (unsigned int)v52 < v50; v52 = (unsigned int)(v52 + 8))
							*(_QWORD*)&v28[v52] = k;
					}
					else
					{
						//++dword_14D40B2B8;
						if (v50)
						{
							v54 = v29 - v28;
							v55 = v50;
							do
							{
								*v28 = v28[v54];
								++v28;
								--v55;
							} while (v55);
						}
					}
				}
				else
				{
					//++dword_14D40B2AC;
					for (m = 0i64; (unsigned int)m < (unsigned int)v48; m = (unsigned int)(m + 8))
						*(_QWORD*)&v28[m] = *(_QWORD*)&v29[m];
				}
			}
			else
			{
				outBufBytePos += v20;
				*(_QWORD*)v28 = *(_QWORD*)v29;
				*((_QWORD*)v28 + 1) = *((_QWORD*)v29 + 1);
			}
			inputBuf = v74;
		}
		if (inBufBytePos >= qword70)
			break;
	LABEL_29:
		outputBuf = v70;
		v13 = (*(_QWORD*)&inputBuf[inBufBytePos & decoder->inputMask] << (64 - (unsigned __int8)currentBit)) | v19;
	}
	if (outBufBytePos != decoder->decompressedStreamSize)
		goto LABEL_25;
	decompSize = decoder->decompSize;
	if (outBufBytePos == decompSize)
	{
		result = true;
		goto LABEL_69;
	}
	inputInvMask = decoder->inputInvMask;
	headerOffset = decoder->headerOffset;
	v33 = inputInvMask & -(__int64)inBufBytePos;
	v19 >>= 1;
	++currentBit;
	if (headerOffset > v33)
	{
		inBufBytePos += v33;
		v34 = decoder->qword70;
		if (inBufBytePos > v34)
			decoder->qword70 = inputInvMask + v34 + 1;
	}
	v35 = inBufBytePos & decoder->inputMask;
	inBufBytePos += headerOffset;
	v36 = outBufBytePos + decoder->outputInvMask + 1;
	v37 = *(_QWORD*)&inputBuf[v35] & ((1i64 << (8 * (unsigned __int8)headerOffset)) - 1);
	v38 = v37 + decoder->bufferSizeNeeded;
	v39 = v37 + decoder->compressedStreamSize;
	decoder->bufferSizeNeeded = v38;
	decoder->compressedStreamSize = v39;
	if (v36 >= decompSize)
	{
		v36 = decompSize;
		decoder->compressedStreamSize = headerOffset + v39;
	}
	decoder->decompressedStreamSize = v36;
	if (inLen >= v38 && outLen >= v36)
	{
	LABEL_25:
		qword70 = decoder->qword70;
		if (inBufBytePos >= qword70)
		{
			inBufBytePos = ~decoder->inputInvMask & (inBufBytePos + 7);
			qword70 += decoder->inputInvMask + 1;
			decoder->qword70 = qword70;
		}
		if (decoder->compressedStreamSize < qword70)
			qword70 = decoder->compressedStreamSize;
		goto LABEL_29;
	}
	v69 = decoder->qword70;
	if (inBufBytePos >= v69)
	{
		inBufBytePos = ~inputInvMask & (inBufBytePos + 7);
		decoder->qword70 = v69 + inputInvMask + 1;
	}
	decoder->dword6C = v71;
	result = false;
	decoder->currentByte = v19;
	decoder->currentBit = currentBit;
LABEL_69:
	decoder->outBufBytePos = outBufBytePos;
	decoder->inBufBytePos = inBufBytePos;
	return result;
}

//-----------------------------------------------------------------------------
// initializes the ZStd decoder
//-----------------------------------------------------------------------------
static size_t Pak_ZStdDecoderInit(PakDecoder_s* const decoder, const uint8_t* frameHeader,
	const size_t dataSize, const size_t headerSize)
{
	ZSTD_DStream* const dctx = ZSTD_createDStream();
	assert(dctx);

	// failure
	if (!dctx)
		return NULL;

	decoder->zstreamContext = dctx;

	if (ZSTD_getFrameHeader(&dctx->fParams, frameHeader, dataSize) != 0)
	{
		ZSTD_freeDStream(decoder->zstreamContext);
		decoder->zstreamContext = nullptr;

		return NULL; // content size error
	}

	// ideally the frame header of the block gets parsed first, the length
	// thereof is returned by initDStream and thus being processed first
	// before moving on to actual data
	decoder->frameHeaderSize = ZSTD_initDStream(dctx);

	// we need at least this many bytes of streamed data to process the frame
	// header of the compressed block
	decoder->bufferSizeNeeded = decoder->inBufBytePos + decoder->frameHeaderSize;

	// must include header size
	decoder->decompSize = dctx->fParams.frameContentSize + headerSize;
	return decoder->decompSize;
}

//-----------------------------------------------------------------------------
// decodes the ZStd data stream up to available buffer or data, whichever ends
// first
//-----------------------------------------------------------------------------
static bool Pak_ZStdStreamDecode(PakDecoder_s* const decoder, const PakRingBufferFrame_s& outFrame, const PakRingBufferFrame_s& inFrame)
{
	ZSTD_outBuffer outBuffer = {
		&decoder->outputBuf[outFrame.bufIndex],
		outFrame.frameLen, NULL
	};

	ZSTD_inBuffer inBuffer = {
		&decoder->inputBuf[inFrame.bufIndex],
		inFrame.frameLen, NULL
	};

	ZSTD_DStream* const dctx = decoder->zstreamContext;
	const size_t ret = ZSTD_decompressStream(dctx, &outBuffer, &inBuffer);

	if (ZSTD_isError(ret))
	{
		// NOTE: obtained here and not in the error formatter as we could check
		// the error string during the assertion
		const char* const decodeError = ZSTD_getErrorName(ret);
		assert(0);

		Error(eDLL_T::RTECH, EXIT_FAILURE, "%s: decode error: %s\n", __FUNCTION__, decodeError);
		return false;
	}

	// advance buffer io positions, required so the main parser could already
	// start parsing the headers while the rest is getting decoded still
	decoder->outBufBytePos += outBuffer.pos;
	decoder->inBufBytePos += inBuffer.pos;

	// on the next call, we need at least this amount of data streamed in order
	// to decode the rest of the pak file, as this is where reading has stopped
	// this value may equal the currently streamed input size, as its possible
	// this function is getting called to flush the remainder decoded data into
	// the out buffer which got truncated off on the call prior due to wrapping
	//
	// if the input stream has fully decoded, this should equal the size of the
	// encoded pak file
	decoder->bufferSizeNeeded = decoder->inBufBytePos + ZSTD_nextSrcSizeToDecompress(dctx);

	const bool decoded = ret == NULL;

	// zstd decoder no longer necessary at this point, deallocate
	if (decoded)
	{
		ZSTD_freeDStream(dctx);
		decoder->zstreamContext = nullptr;
	}

	return decoded;
}

//-----------------------------------------------------------------------------
// initializes the decoder
//-----------------------------------------------------------------------------
size_t Pak_InitDecoder(PakDecoder_s* const decoder, const uint8_t* const inputBuf, uint8_t* const outputBuf,
	const uint64_t inputMask, const uint64_t outputMask, const size_t dataSize, const size_t dataOffset,
	const size_t headerSize, const PakDecodeMode_e decodeMode)
{
	// buffer size must be power of two as we index into buffers using a bit
	// mask rather than a modulo, the mask provided must be bufferSize-1
	assert(IsPowerOfTwo(inputMask + 1));
	assert(IsPowerOfTwo(outputMask + 1));

	// the absolute start address of the input and output buffers
	decoder->inputBuf = inputBuf;
	decoder->outputBuf = outputBuf;

	// the actual file size, which consists of dataOffset (anything up to the
	// frame header, like the file header) and the actual encoded data itself
	decoder->fileSize = dataOffset + dataSize;
	decoder->decodeMode = decodeMode;

	// buffer masks, which essentially gets used to index into the input and
	// output buffers, similar to 'idx % bufSize', where bufSize = bufMask+1
	decoder->inputMask = inputMask;
	decoder->outputMask = outputMask;

	// the current positions in the input and output buffers; if we deal with
	// paks that are patched, the input buffer position during the init and
	// decode call on subsequent patches may not be at the start of the buffer,
	// they will end where the previous 'to patch' pak had finished streaming
	// and decoding
	decoder->inBufBytePos = dataOffset + headerSize;
	decoder->outBufBytePos = headerSize;

	// if we use the default RTech decoder, return from here as the stuff below
	// is handled by the RTech decoder internally
	if (decodeMode == PakDecodeMode_e::MODE_RTECH)
		return Pak_RTechDecoderInit(decoder, inputBuf, inputMask, dataSize, dataOffset, headerSize);

	// NOTE: on RTech encoded paks this data is parsed out of the frame header,
	// but for ZStd encoded paks we are always limiting this to the ring buffer
	// size
	decoder->outputInvMask = PAK_DECODE_OUT_RING_BUFFER_MASK;

	// this points to the first byte of the frame header, takes dataOffset
	// into account which is the offset in the ring buffer to the patched
	// data as we parse it contiguously after the base pak data, which
	// might have ended somewhere in the middle of the ring buffer
	const uint8_t* const frameHeaderData = &inputBuf[inputMask & (dataOffset + headerSize)];

	const size_t decodeSize = Pak_ZStdDecoderInit(decoder, frameHeaderData, dataSize, headerSize);
	assert(decodeSize);

	return decodeSize;
}

//-----------------------------------------------------------------------------
// decodes streamed input pak data; on patched pak files, inLen will continue
// from where the base pak had ended as patch pak files are considered part of
// the pak file that's currently getting loaded
//-----------------------------------------------------------------------------
bool Pak_StreamToBufferDecode(PakDecoder_s* const decoder, const size_t inLen, const size_t outLen, const PakDecodeMode_e decodeMode)
{
	if (!Pak_HasEnoughStreamedDataForDecode(decoder, inLen))
		return false;

	if (!Pak_HasEnoughDecodeBufferAvailable(decoder, outLen))
		return false;

	if (decodeMode == PakDecodeMode_e::MODE_RTECH)
		return Pak_RTechStreamDecode(decoder, inLen, outLen);

	// must have a decoder at this point
	//
	// also, input seek pos may not exceed inLen as we can't read past
	// currently streamed data; this should've been checked before reaching
	// this position in code
	assert(decoder->zstreamContext && decoder->inBufBytePos <= inLen);

	const PakRingBufferFrame_s outFrame = Pak_DetermineRingBufferFrame(decoder->outputMask, decoder->outBufBytePos , outLen);
	const PakRingBufferFrame_s inFrame  = Pak_DetermineRingBufferFrame(decoder->inputMask, decoder->inBufBytePos, inLen);

	return Pak_ZStdStreamDecode(decoder, outFrame, inFrame);
}

//-----------------------------------------------------------------------------
// decodes buffered input pak data
//-----------------------------------------------------------------------------
bool Pak_BufferToBufferDecode(uint8_t* const inBuf, uint8_t* const outBuf, const size_t pakSize, const PakDecodeMode_e decodeMode)
{
	assert(decodeMode != PakDecodeMode_e::MODE_DISABLED);

	PakDecoder_s decoder{};
	const size_t decompressedSize = Pak_InitDecoder(&decoder, inBuf, outBuf, UINT64_MAX, UINT64_MAX, pakSize, NULL, sizeof(PakFileHeader_s), decodeMode);

	PakFileHeader_s* const inHeader = reinterpret_cast<PakFileHeader_s*>(inBuf);

	if (decompressedSize != inHeader->decompressedSize)
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s: decompressed size: '%zu' expected: '%zu'!\n",
			__FUNCTION__, decompressedSize, inHeader->decompressedSize);

		return false;
	}

	// we should always have enough buffer room at this point
	if (!Pak_StreamToBufferDecode(&decoder, inHeader->compressedSize, inHeader->decompressedSize, decodeMode))
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s: decompression failed!\n",
			__FUNCTION__);

		return false;
	}

	PakFileHeader_s* const outHeader = reinterpret_cast<PakFileHeader_s*>(outBuf);

	// copy the header over to the decoded buffer
	*outHeader = *inHeader;

	// remove compress flags
	outHeader->flags &= ~PAK_HEADER_FLAGS_COMPRESSED;
	outHeader->flags &= ~PAK_HEADER_FLAGS_ZSTREAM_ENCODED;

	// equal compressed size with decompressed
	outHeader->compressedSize = outHeader->decompressedSize;

	return true;
}

//-----------------------------------------------------------------------------
// decodes the pak file from file name
//-----------------------------------------------------------------------------
bool Pak_DecodePakFile(const char* const inPakFile, const char* const outPakFile)
{
	// if this path doesn't exist, we must create it first before trying to
	// open the out file
	if (!Pak_CreateOverridePath())
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s: failed to create output path for pak file '%s'!\n",
			__FUNCTION__, outPakFile);

		return false;
	}

	CIOStream inPakStream;

	if (!inPakStream.Open(inPakFile, CIOStream::Mode_e::Read))
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s: failed to open pak file '%s' for read!\n",
			__FUNCTION__, inPakFile);

		return false;
	}

	CIOStream outPakStream;

	if (!outPakStream.Open(outPakFile, CIOStream::Mode_e::Write))
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s: failed to open pak file '%s' for write!\n",
			__FUNCTION__, outPakFile);

		return false;
	}

	const size_t fileSize = inPakStream.GetSize();

	if (fileSize <= sizeof(PakFileHeader_s))
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s: pak '%s' appears truncated!\n",
			__FUNCTION__, inPakFile);

		return false;
	}

	std::unique_ptr<uint8_t[]> inPakBufContainer(new uint8_t[fileSize]);
	uint8_t* const inPakBuf = inPakBufContainer.get();

	inPakStream.Read(inPakBuf, fileSize);
	inPakStream.Close();

	PakFileHeader_s* const inHeader = reinterpret_cast<PakFileHeader_s*>(inPakBuf);

	if (inHeader->magic != PAK_HEADER_MAGIC || inHeader->version != PAK_HEADER_VERSION)
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s: pak '%s' has incompatible or invalid header!\n",
			__FUNCTION__, inPakFile);

		return false;
	}

	const PakDecodeMode_e decodeMode = inHeader->GetCompressionMode();

	if (decodeMode == PakDecodeMode_e::MODE_DISABLED)
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s: pak '%s' is already decompressed!\n",
			__FUNCTION__, inPakFile);

		return false;
	}

	if (inHeader->compressedSize != fileSize)
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s: pak '%s' appears truncated or corrupt; compressed size: '%zu' expected: '%zu'!\n",
			__FUNCTION__, inPakFile, fileSize, inHeader->compressedSize);

		return false;
	}

	Pak_ShowHeaderDetails(inHeader);

	std::unique_ptr<uint8_t[]> outPakBufContainer(new uint8_t[inHeader->decompressedSize]);
	uint8_t* const outPakBuf = outPakBufContainer.get();

	if (!Pak_BufferToBufferDecode(inPakBuf, outPakBuf, fileSize, decodeMode))
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s: failed to decompress pak file '%s'!\n",
			__FUNCTION__, inPakFile);

		return false;
	}

	const PakFileHeader_s* const outHeader = reinterpret_cast<PakFileHeader_s*>(outPakBuf);

	// NOTE: if the paks this particular pak patches have different sizes than
	// current sizes in the patch header, the runtime will crash!
	if (outHeader->patchIndex && !Pak_UpdatePatchHeaders(outPakBuf, outPakFile))
	{
		Warning(eDLL_T::RTECH, "%s: pak '%s' is a patch pak, but the pak(s) it patches weren't found; patch headers not updated!\n",
			__FUNCTION__, inPakFile);
	}

	outPakStream.Write(outPakBuf, outHeader->decompressedSize);

	Msg(eDLL_T::RTECH, "Decompressed pak file to: '%s'\n", outPakFile);
	return true;
}
