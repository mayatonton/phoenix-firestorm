/**
* @file llvkuboreg.h
* @brief AYAstorm r41 UBO offset/size single-source-of-truth verification (UBOReg)
*
* $LicenseInfo:firstyear=2026&license=viewerlgpl$
* AYAstorm Viewer Source Code
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
* $/LicenseInfo$
*/

#ifndef LL_LLVKUBOREG_H
#define LL_LLVKUBOREG_H

#include "stdtypes.h"

#include <cstddef>
#include <string>

class LLGLSLShader;
struct VkReflUboBlock;

namespace LLVkUboReg
{
    struct MemberEntry
    {
        const char* name;
        U32 offset;
        U32 size;
    };

    struct BlockEntry
    {
        S32 set;
        const char* block_name;
        U32 cpp_size;
        const MemberEntry* members;
        U32 member_count;
    };

    struct PcEntry
    {
        const char* name;
        U32 offset;
        U32 size;
    };

    struct LedgerEntry
    {
        const char* shader_name;
        const char* block_name;
        const char* kind;
        U32 expect_pipeline_size;
        U32 expect_block_size;
    };

    constexpr U32 PC_OFF_MODELVIEW            = 0;
    constexpr U32 PC_OFF_POINT_SIZE           = 64;
    constexpr U32 PC_OFF_LAST_OBJECT_MATRIX   = 64;
    constexpr U32 PC_OFF_MINIMUM_ALPHA        = 64;
    constexpr U32 PC_OFF_SSS_SKIN_FLAG        = 68;
    constexpr U32 PC_OFF_OBJECT_ALPHA         = 68;
    constexpr U32 PC_OFF_WATER_SIGN           = 72;
    constexpr U32 PC_OFF_PREVIEW_NEUTRAL_ATMOS = 76;
    constexpr U32 PC_OFF_EMISSIVE_COLOR       = 80;
    constexpr U32 PC_OFF_METALLIC             = 96;
    constexpr U32 PC_OFF_ROUGHNESS            = 100;
    constexpr U32 PC_OFF_GAUSSIAN_RES_SCALE   = 104;
    constexpr U32 PC_OFF_GAUSSIAN_DIRECTION   = 112;

    constexpr MemberEntry mat3Entry(const char* glsl_name, U32 off0, U32 off1, U32 off2, U32 col_size)
    {
        return (off1 == off0 + col_size && off2 == off1 + col_size)
            ? MemberEntry{ glsl_name, off0, 3 * col_size }
            : throw "UBOREG_M3: mat3 column fields are not adjacent";
    }

    typedef void (*VariantVerifier)(const LLGLSLShader& shader, const VkReflUboBlock& block);

    void registerVariantVerifier(const char* block_name, VariantVerifier fn);

    void verifyProgramLayout(LLGLSLShader& shader);
    void verifyPerProgramSize(LLGLSLShader& shader, U32 per_program_ubo_size);

    std::string normalizeMemberName(const std::string& block_name, const std::string& member_name);

    void reportDiff(const LLGLSLShader& shader, const VkReflUboBlock& block, const std::string& member,
                    const char* kind, U32 expect_off, U32 expect_size, U32 refl_off, U32 refl_size);
    void reportUnknown(const LLGLSLShader& shader, const VkReflUboBlock& block, const std::string& member,
                       const char* kind);
}

#define UBOREG_M(S, f) { #f, (U32)offsetof(LLVKLoader::S, f), (U32)sizeof(((LLVKLoader::S*)0)->f) }
#define UBOREG_MN(S, f, glsl_name) { glsl_name, (U32)offsetof(LLVKLoader::S, f), (U32)sizeof(((LLVKLoader::S*)0)->f) }
#define UBOREG_M3(S, f, glsl_name) \
    LLVkUboReg::mat3Entry(glsl_name, \
        (U32)offsetof(LLVKLoader::S, f##_col0), \
        (U32)offsetof(LLVKLoader::S, f##_col1), \
        (U32)offsetof(LLVKLoader::S, f##_col2), \
        (U32)sizeof(((LLVKLoader::S*)0)->f##_col0))

#endif // LL_LLVKUBOREG_H
