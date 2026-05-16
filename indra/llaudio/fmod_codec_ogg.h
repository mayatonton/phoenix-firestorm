/**
 * @file fmod_codec_ogg.h
 * @brief AYAstorm FMOD codec plugin for Ogg Opus and Ogg Vorbis.
 *
 * Bundled libfmod lacks Opus decode support. A priority-0 Opus probe also
 * cannot safely hand non-seekable Ogg Vorbis streams back to FMOD after reading
 * from them, so this codec handles both Ogg Opus and Ogg Vorbis in one pass.
 */

#ifndef LL_FMOD_CODEC_OGG_H
#define LL_FMOD_CODEC_OGG_H

// fmod.h pulls in fmod_common.h first, which defines F_CALL and the typedefs
// used by fmod_codec.h. Including fmod_codec.h alone would fail with
// "F_CALL was not declared" / "FMOD_TIMEUNIT does not name a type".
#include "fmodstudio/fmod.h"
#include "fmodstudio/fmod_codec.h"

extern "C" FMOD_CODEC_DESCRIPTION* F_CALL FMODGetCodecDescriptionOgg();

#endif // LL_FMOD_CODEC_OGG_H
