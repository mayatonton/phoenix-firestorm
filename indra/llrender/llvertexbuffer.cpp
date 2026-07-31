/**
 * @file llvertexbuffer.cpp
 * @brief LLVertexBuffer implementation
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include <set>
#include <map>
#include <cstring>
#include "llfasttimer.h"
#include "llsys.h"
#include "llvertexbuffer.h"
#include "llglheaders.h"
#include "llrender.h"
#include "llvector4a.h"
#include "llshadermgr.h"
#include "llglslshader.h"
#include "llmemory.h"
#include "llvkloader.h"
#include "llvkcontract.h"
#include "llvkuboreg.h"
#include "llrendertarget.h"
#include <glm/gtc/type_ptr.hpp>

U32 nhpo2(U32 v)
{
    U32 r = 1;
    while (r < v) {
        r *= 2;
    }
    return r;
}

U32 wpo2(U32 i)
{
    llassert(i > 0);
    llassert(nhpo2(i) == i);

    U32 r = 0;

    while (i >>= 1) ++r;

    return r;
}

struct CompareMappedRegion
{
    bool operator()(const LLVertexBuffer::MappedRegion& lhs, const LLVertexBuffer::MappedRegion& rhs)
    {
        return lhs.mStart < rhs.mStart;
    }
};


#define ANALYZE_VBO_POOL 0

class LLVBOPool
{
    public:
    virtual ~LLVBOPool() = default;
    virtual void allocate(GLenum type, U32 size, U8*& data) = 0;
    virtual void free(GLenum type, U32 size, U8* data) = 0;
    virtual U64 getVramBytesUsed() = 0;
};

class LLDefaultVBOPool final : public LLVBOPool
{
public:
    typedef std::chrono::steady_clock::time_point Time;
    struct Entry
    {
        U8* mData;
        Time mAge;
    };

    ~LLDefaultVBOPool() override
    {
        clear();
    }

    typedef std::unordered_map<U32, std::list<Entry>> Pool;

    Pool mVBOPool;
    Pool mIBOPool;

    U32 mTouchCount = 0;

    U64 mDistributed = 0;
    U64 mAllocated = 0;
    U64 mReserved = 0;
    U32 mMisses = 0;
    U32 mHits = 0;

    U64 getVramBytesUsed() override
    {
        return mAllocated + mReserved;
    }

    void adjustSize(U32& size)
    {
        U32 block_size = llmax(nhpo2(size) / 8, (U32) 16);
        size += block_size - (size % block_size);
    }

    void allocate(GLenum type, U32 size, U8*& data) override
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
        llassert(type == GL_ARRAY_BUFFER || type == GL_ELEMENT_ARRAY_BUFFER);
        llassert(data == nullptr);  // non null data indicates a buffer that wasn't freed
        llassert(size >= 2);  // any buffer size smaller than a single index is nonsensical

        mDistributed += size;
        adjustSize(size);
        mAllocated += size;

        auto& pool = type == GL_ELEMENT_ARRAY_BUFFER ? mIBOPool : mVBOPool;

        Pool::iterator iter = pool.find(size);
        if (iter == pool.end())
        { // cache miss, allocate a new buffer
            LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("vbo pool miss");
            LL_PROFILE_GPU_ZONE("vbo alloc");

            mMisses++;

            data = (U8*)ll_aligned_malloc_16(size);
        }
        else
        {
            mHits++;
            llassert(mReserved >= size); // assert if accounting gets messed up
            mReserved -= size;

            std::list<Entry>& entries = iter->second;
            Entry& entry = entries.back();
            data = entry.mData;

            entries.pop_back();
            if (entries.empty())
            {
                pool.erase(iter);
            }
        }

        clean();
    }

    void free(GLenum type, U32 size, U8* data) override
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
        llassert(type == GL_ARRAY_BUFFER || type == GL_ELEMENT_ARRAY_BUFFER);
        llassert(size >= 2);
        llassert(data != nullptr);

        clean();

        llassert(mDistributed >= size);
        mDistributed -= size;
        adjustSize(size);
        llassert(mAllocated >= size);
        mAllocated -= size;
        mReserved += size;

        auto& pool = type == GL_ELEMENT_ARRAY_BUFFER ? mIBOPool : mVBOPool;

        Pool::iterator iter = pool.find(size);

        if (iter == pool.end())
        {
            std::list<Entry> newlist;
            newlist.push_front({ data, std::chrono::steady_clock::now() });
            pool[size] = newlist;
        }
        else
        {
            iter->second.push_front({ data, std::chrono::steady_clock::now() });
        }

    }

    void clean()
    {
        mTouchCount++;
        if (mTouchCount < 1024) // clean every 1k touches
        {
            return;
        }
        mTouchCount = 0;

        LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;

        std::unordered_map<U32, std::list<Entry>>* pools[] = { &mVBOPool, &mIBOPool };

        using namespace std::chrono_literals;

        Time cutoff = std::chrono::steady_clock::now() - 5s;

        for (auto* pool : pools)
        {
            for (Pool::iterator iter = pool->begin(); iter != pool->end(); )
            {
                auto& entries = iter->second;

                while (!entries.empty() && entries.back().mAge < cutoff)
                {
                    LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("vbo cache timeout");
                    auto& entry = entries.back();
                    ll_aligned_free_16(entry.mData);
                    llassert(mReserved >= iter->first);
                    mReserved -= iter->first;
                    entries.pop_back();

                }

                if (entries.empty())
                {
                    iter = pool->erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
        }

#if 0
        LL_INFOS() << llformat("(%d/%d)/%d MB (distributed/allocated)/total in VBO Pool. Overhead: %d percent. Hit rate: %d percent",
            mDistributed / 1000000,
            mAllocated / 1000000,
            (mAllocated + mReserved) / 1000000, // total bytes
            ((mAllocated+mReserved-mDistributed)*100)/llmax(mDistributed, (U64) 1), // overhead percent
            (mHits*100)/llmax(mMisses+mHits, (U32)1)) // hit rate percent
            << LL_ENDL;
#endif
    }

    void clear()
    {
        for (auto& entries : mIBOPool)
        {
            for (auto& entry : entries.second)
            {
                ll_aligned_free_16(entry.mData);
            }
        }

        for (auto& entries : mVBOPool)
        {
            for (auto& entry : entries.second)
            {
                ll_aligned_free_16(entry.mData);
            }
        }

        mReserved = 0;

        mIBOPool.clear();
        mVBOPool.clear();
    }
};

static LLVBOPool* sVBOPool = nullptr;

void LLVertexBufferData::drawWithMatrix()
{
    if (!mVB)
    {
        llassert(false);
        return;
    }

    if (mImageGL)
    {
        gGL.getTexUnit(0)->bind(mImageGL);
    }
    else
    {
        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    }

    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.pushMatrix();
    gGL.loadMatrix(glm::value_ptr(mModelView));
    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.pushMatrix();
    gGL.loadMatrix(glm::value_ptr(mProjection));
    gGL.matrixMode(LLRender::MM_TEXTURE0);
    gGL.pushMatrix();
    gGL.loadMatrix(glm::value_ptr(mTexture0));

    mVB->setBuffer();
    mVB->drawArrays(mMode, 0, mCount);

    gGL.popMatrix();
    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.popMatrix();
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.popMatrix();
}

void LLVertexBufferData::draw()
{
    if (!mVB)
    {
        llassert(false);
        return;
    }

    if (mImageGL)
    {
        gGL.getTexUnit(0)->bind(mImageGL);
    }
    else
    {
        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    }

    mVB->setBuffer();
    mVB->drawArrays(mMode, 0, mCount);
}


U64 LLVertexBuffer::getBytesAllocated()
{
    return sVBOPool ? sVBOPool->getVramBytesUsed() : 0;
}

//
U32 LLVertexBuffer::sVertexCount = 0;
std::atomic<U32> LLVertexBuffer::sVkDrawCallCount{0};


const U32 LLVertexBuffer::sTypeSize[LLVertexBuffer::TYPE_MAX] =
{
    sizeof(LLVector4), // TYPE_VERTEX,
    sizeof(LLVector4), // TYPE_NORMAL,
    sizeof(LLVector2), // TYPE_TEXCOORD0,
    sizeof(LLVector2), // TYPE_TEXCOORD1,
    sizeof(LLVector2), // TYPE_TEXCOORD2,
    sizeof(LLVector2), // TYPE_TEXCOORD3,
    sizeof(LLColor4U), // TYPE_COLOR,
    sizeof(LLColor4U), // TYPE_EMISSIVE, only alpha is used currently
    sizeof(LLVector4), // TYPE_TANGENT,
    sizeof(F32),       // TYPE_WEIGHT,
    sizeof(LLVector4), // TYPE_WEIGHT4,
    sizeof(LLVector4), // TYPE_CLOTHWEIGHT,
    sizeof(U64),       // TYPE_JOINT,
    sizeof(LLVector4), // TYPE_TEXTURE_INDEX (actually exists as position.w), no extra data, but stride is 16 bytes
};

static const std::string vb_type_name[] =
{
    "TYPE_VERTEX",
    "TYPE_NORMAL",
    "TYPE_TEXCOORD0",
    "TYPE_TEXCOORD1",
    "TYPE_TEXCOORD2",
    "TYPE_TEXCOORD3",
    "TYPE_COLOR",
    "TYPE_EMISSIVE",
    "TYPE_TANGENT",
    "TYPE_WEIGHT",
    "TYPE_WEIGHT4",
    "TYPE_CLOTHWEIGHT",
    "TYPE_JOINT"
    "TYPE_TEXTURE_INDEX",
    "TYPE_MAX",
    "TYPE_INDEX",
};

const U32 LLVertexBuffer::sGLMode[LLRender::NUM_MODES] =
{
    GL_TRIANGLES,
    GL_TRIANGLE_STRIP,
    GL_TRIANGLE_FAN,
    GL_POINTS,
    GL_LINES,
    GL_LINE_STRIP,
    GL_LINE_LOOP,
};

void LLVertexBuffer::drawArrays(U32 mode, const std::vector<LLVector3>& pos)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
    gGL.begin(mode);
    for (auto& v : pos)
    {
        gGL.vertex3fv(v.mV);
    }
    gGL.end();
    gGL.flush();
}

void LLVertexBuffer::drawElements(U32 mode, const LLVector4a* pos, const LLVector2* tc, U32 num_indices, const U16* indicesp)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
    llassert(LLGLSLShader::sCurBoundShaderPtr != NULL);

    // <FS:Beq> FIRE-29679 trap empty calls that cause crashes when rezzing in OpenSim.
    if(pos == nullptr || indicesp == nullptr )
    {
        LL_WARNS() << "Called drawElements with null pos or null indices" << LL_ENDL;
        return;
    }
    // </FS:Beq>

    // <FS:Ansariel> Crash fix due to invalid calls to drawElements by Drake Arconis
    if (num_indices == 0)
    {
        LL_WARNS() << "Called drawElements with 0 indices" << LL_ENDL;
        return;
    }
    // </FS:Ansariel>


    gGL.syncMatrices();

    unbind();

    gGL.begin(mode);

    if (tc != nullptr)
    {
        for (U32 i = 0; i < num_indices; ++i)
        {
            U16 idx = indicesp[i];
            gGL.texCoord2fv(tc[idx].mV);
            gGL.vertex3fv(pos[idx].getF32ptr());
        }
    }
    else
    {
        for (U32 i = 0; i < num_indices; ++i)
        {
            U16 idx = indicesp[i];
            gGL.vertex3fv(pos[idx].getF32ptr());
        }
    }
    gGL.end();
    gGL.flush();
}

bool LLVertexBuffer::validateRange(U32 start, U32 end, U32 count, U32 indices_offset) const
{
    if (!gDebugGL)
    {
        return true;
    }

    llassert(start < mNumVerts);
    llassert(end < mNumVerts);

    if (start >= mNumVerts ||
        end >= mNumVerts)
    {
        LL_ERRS() << "Bad vertex buffer draw range: [" << start << ", " << end << "] vs " << mNumVerts << LL_ENDL;
    }

    if (indices_offset >= mNumIndices ||
        indices_offset + count > mNumIndices)
    {
        LL_ERRS() << "Bad index buffer draw range: [" << indices_offset << ", " << indices_offset+count << "]" << LL_ENDL;
    }

    {
#if 0  // not a reliable test for VBOs that are not backed by a CPU buffer
        U16* idx = (U16*) mMappedIndexData+indices_offset;
        for (U32 i = 0; i < count; ++i)
        {
            llassert(idx[i] >= start);
            llassert(idx[i] <= end);

            if (idx[i] < start || idx[i] > end)
            {
                LL_ERRS() << "Index out of range: " << idx[i] << " not in [" << start << ", " << end << "]" << LL_ENDL;
            }
        }

        LLVector4a* v = (LLVector4a*)mMappedData;

        for (U32 i = start; i <= end; ++i)
        {
            if (!v[i].isFinite3())
            {
                LL_ERRS() << "Non-finite vertex position data detected." << LL_ENDL;
            }
        }

        LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;

        if (shader && shader->mFeatures.mIndexedTextureChannels > 1)
        {
            LLVector4a* v = (LLVector4a*) mMappedData;

            for (U32 i = start; i < end; i++)
            {
                U32 idx = (U32) (v[i][3]+0.25f);
                if (idx >= (U32)shader->mFeatures.mIndexedTextureChannels)
                {
                    LL_ERRS() << "Bad texture index found in vertex data stream." << LL_ENDL;
                }
            }
        }
#endif
    }

    return true;
}

#if LL_PROFILER_ENABLE_RENDER_DOC
void LLVertexBuffer::setLabel(const char* label) {
}
#endif

void LLVertexBuffer::clone(LLVertexBuffer& target) const
{
    target.mTypeMask = mTypeMask;
    target.mIndicesType = mIndicesType;
    target.mIndicesStride = mIndicesStride;
    if (target.getNumVerts() != getNumVerts() ||
        target.getNumIndices() != getNumIndices())
    {
        target.allocateBuffer(getNumVerts(), getNumIndices());
    }
}

void LLVertexBuffer::drawRange(U32 mode, U32 start, U32 end, U32 count, U32 indices_offset, U32 draw_data_slot) const
{
    llassert(validateRange(start, end, count, indices_offset));
    gGL.syncMatrices();
    bool vk_fired = false;

    if (LLVKLoader::shouldUseVulkanRender()
        && LLGLSLShader::sCurBoundShaderPtr != nullptr)
    {
        VkCommandBuffer cmd = VK_NULL_HANDLE;
        if (LLVKLoader::beginShaderDrawOrSkip(LLGLSLShader::sCurBoundShaderPtr, mode, cmd))
        {
            const U32 fi = (draw_data_slot == LLVKLoader::PERDRAW_SLOT_INHERIT) ? 0u : draw_data_slot;
            if (LLGLSLShader::sCurBoundShaderPtr->mVkUsesHeapSet
                || LLGLSLShader::sCurBoundShaderPtr->mVkUsesSkinSet)
            {
                LLVKContract::checkDrawDataIDAtFire(fi);
            }
            vkCmdDrawIndexed(cmd, count, 1, mVkIndexSlice.offset / mIndicesStride + indices_offset, (S32)mVkVertexSlice.first, fi);
            ++sVkDrawCallCount;
            vk_fired = true;
            LLVKContract::drawFired();
        }
    }

    if (LLVKLoader::shouldUseVulkanRender())
    {
        LLGLSLShader* sh = LLGLSLShader::sCurBoundShaderPtr;
        LLVKContract::checkPerDrawIDFreshnessAtFire(
            vk_fired,
            sh != nullptr && sh->mVkUsesSkinSet,
            sh != nullptr ? sh->mName.c_str() : nullptr);
    }

    if (!vk_fired)
    {
        if (LLVKLoader::shouldUseVulkanRender()
            && LLGLSLShader::sCurBoundShaderPtr == nullptr)
        {
            LLVKContract::drawSkipped(LLVKContract::C_NO_SHADER_OR_LAYOUT,
                                      std::string("(no-shader)"));
        }
    }
}

void LLVertexBuffer::drawRangeFast(U32 mode, U32 start, U32 end, U32 count, U32 indices_offset, U32 draw_data_slot) const
{
    if (LLVKLoader::shouldUseVulkanRender())
    {
        bool vk_fired = false;
        if (LLGLSLShader::sCurBoundShaderPtr != nullptr)
        {
            VkCommandBuffer cmd = VK_NULL_HANDLE;
            if (LLVKLoader::beginShaderDrawOrSkip(LLGLSLShader::sCurBoundShaderPtr, mode, cmd))
            {
                const U32 fi = (draw_data_slot == LLVKLoader::PERDRAW_SLOT_INHERIT) ? 0u : draw_data_slot;
                if (LLGLSLShader::sCurBoundShaderPtr->mVkUsesHeapSet
                    || LLGLSLShader::sCurBoundShaderPtr->mVkUsesSkinSet)
                {
                    LLVKContract::checkDrawDataIDAtFire(fi);
                }
                vkCmdDrawIndexed(cmd, count, 1, mVkIndexSlice.offset / mIndicesStride + indices_offset, (S32)mVkVertexSlice.first, fi);
                ++sVkDrawCallCount;
                vk_fired = true;
                LLVKContract::drawFired();
            }
        }
        {
            LLGLSLShader* sh = LLGLSLShader::sCurBoundShaderPtr;
            LLVKContract::checkPerDrawIDFreshnessAtFire(
                vk_fired,
                sh != nullptr && sh->mVkUsesSkinSet,
                sh != nullptr ? sh->mName.c_str() : nullptr);
        }
        if (!vk_fired && LLGLSLShader::sCurBoundShaderPtr == nullptr)
        {
            LLVKContract::drawSkipped(LLVKContract::C_NO_SHADER_OR_LAYOUT,
                                      std::string("(no-shader)"));
        }
        return;
    }
}


void LLVertexBuffer::draw(U32 mode, U32 count, U32 indices_offset, U32 draw_data_slot) const
{
    drawRange(mode, 0, mNumVerts-1, count, indices_offset, draw_data_slot);
}


void LLVertexBuffer::drawArrays(U32 mode, U32 first, U32 count, U32 draw_data_slot) const
{
    llassert(first + count <= mNumVerts);

    gGL.syncMatrices();
    bool vk_fired = false;

    if (LLVKLoader::shouldUseVulkanRender()
        && LLGLSLShader::sCurBoundShaderPtr != nullptr)
    {
        VkCommandBuffer cmd = VK_NULL_HANDLE;
        if (LLVKLoader::beginShaderDrawOrSkip(LLGLSLShader::sCurBoundShaderPtr, mode, cmd))
        {
            const U32 fi = (draw_data_slot == LLVKLoader::PERDRAW_SLOT_INHERIT) ? 0u : draw_data_slot;
            if (LLGLSLShader::sCurBoundShaderPtr->mVkUsesHeapSet
                || LLGLSLShader::sCurBoundShaderPtr->mVkUsesSkinSet)
            {
                LLVKContract::checkDrawDataIDAtFire(fi);
            }
            vkCmdDraw(cmd, count, 1, mVkVertexSlice.first + first, fi);
            ++sVkDrawCallCount;
            vk_fired = true;
            LLVKContract::drawFired();
        }
    }

    if (LLVKLoader::shouldUseVulkanRender())
    {
        LLGLSLShader* sh = LLGLSLShader::sCurBoundShaderPtr;
        LLVKContract::checkPerDrawIDFreshnessAtFire(
            vk_fired,
            sh != nullptr && sh->mVkUsesSkinSet,
            sh != nullptr ? sh->mName.c_str() : nullptr);
    }

    if (!vk_fired)
    {
        if (LLVKLoader::shouldUseVulkanRender()
            && LLGLSLShader::sCurBoundShaderPtr == nullptr)
        {
            LLVKContract::drawSkipped(LLVKContract::C_NO_SHADER_OR_LAYOUT,
                                      std::string("(no-shader)"));
        }
    }
}

void LLVertexBuffer::initClass(LLWindow* window)
{
    llassert(sVBOPool == nullptr);
    sVBOPool = new LLDefaultVBOPool();

    LLVKLoader::megabufInit(sTypeSize, TYPE_TEXTURE_INDEX);
}

void LLVertexBuffer::unbind()
{
}

void LLVertexBuffer::cleanupClass()
{
    unbind();

    delete sVBOPool;
    sVBOPool = nullptr;

    LLVKLoader::megabufShutdown();
}


LLVertexBuffer::LLVertexBuffer(U32 typemask)
:   LLRefCount(),
    mTypeMask(typemask)
{
    //zero out offsets
    for (U32 i = 0; i < TYPE_MAX; i++)
    {
        mOffsets[i] = 0;
    }
}

// list of mapped buffers
// NOTE: must not be LLPointer<LLVertexBuffer> to avoid breaking non-ref-counted LLVertexBuffer instances
static std::vector<LLVertexBuffer*> sMappedBuffers;

void LLVertexBuffer::flushBuffers()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
    // must only be called from main thread
    for (auto& buffer : sMappedBuffers)
    {
        buffer->_unmapBuffer();
        buffer->mMapped = false;
    }

    sMappedBuffers.resize(0);
}

U32 LLVertexBuffer::calcOffsets(const U32& typemask, U32* offsets, U32 num_vertices)
{
    U32 offset = 0;
    for (U32 i=0; i<TYPE_TEXTURE_INDEX; i++)
    {
        U32 mask = 1<<i;
        if (typemask & mask)
        {
            if (offsets && LLVertexBuffer::sTypeSize[i])
            {
                offsets[i] = offset;
                offset += LLVertexBuffer::sTypeSize[i]*num_vertices;
                offset = (offset + 0xF) & ~0xF;
            }
        }
    }

    offsets[TYPE_TEXTURE_INDEX] = offsets[TYPE_VERTEX] + 12;

    return offset;
}

U32 LLVertexBuffer::calcVertexSize(const U32& typemask)
{
    U32 size = 0;
    for (U32 i = 0; i < TYPE_TEXTURE_INDEX; i++)
    {
        U32 mask = 1<<i;
        if (typemask & mask)
        {
            size += LLVertexBuffer::sTypeSize[i];
        }
    }

    return size;
}

// protected, use unref()
LLVertexBuffer::~LLVertexBuffer()
{
    if (mMapped)
    { // is on the mapped buffer list but doesn't need to be flushed
        mMapped = false;
        unmapBuffer();
    }

    destroyGLBuffer();
    destroyGLIndices();

    if (mMappedData)
    {
        LL_ERRS() << "Failed to clear vertex buffer's vertices" << LL_ENDL;
    }
    if (mMappedIndexData)
    {
        LL_ERRS() << "Failed to clear vertex buffer's indices" << LL_ENDL;
    }
};


void LLVertexBuffer::genBuffer(U32 size)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
    llassert(mSize == 0);
    llassert(mMappedData == nullptr);

    mSize = size;

    if (mSize > 0 && mVkVertexSlice.buffer == VK_NULL_HANDLE)
    {
        LLVKLoader::megabufAcquireVertex(mTypeMask, mNumVerts, mVkVertexSlice);
    }
}

void LLVertexBuffer::genIndices(U32 size)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
    llassert(mIndicesSize == 0);
    llassert(mMappedIndexData == nullptr);

    mIndicesSize = size;

    if (mIndicesSize > 0 && mVkIndexSlice.buffer == VK_NULL_HANDLE)
    {
        LLVKLoader::megabufAcquireIndex(mIndicesSize, mVkIndexSlice);
    }
}

bool LLVertexBuffer::createGLBuffer(U32 size)
{
    if (mSize > 0)
    {
        destroyGLBuffer();
    }

    if (size == 0)
    {
        return true;
    }

    genBuffer(size);
    return true;
}

bool LLVertexBuffer::createGLIndices(U32 size)
{
    if (mIndicesSize > 0)
    {
        destroyGLIndices();
    }

    if (size == 0)
    {
        return true;
    }

    genIndices(size);
    return true;
}

void LLVertexBuffer::releaseVertexStaging()
{
    if (mMappedData)
    {
        if (sVBOPool)
        {
            sVBOPool->free(GL_ARRAY_BUFFER, mSize, mMappedData);
        }
        mMappedData = nullptr;
    }
    mMappedVertexRegions.clear();
}

void LLVertexBuffer::releaseIndexStaging()
{
    if (mMappedIndexData)
    {
        if (sVBOPool)
        {
            sVBOPool->free(GL_ELEMENT_ARRAY_BUFFER, mIndicesSize, mMappedIndexData);
        }
        mMappedIndexData = nullptr;
    }
    mMappedIndexRegions.clear();
}

void LLVertexBuffer::destroyGLBuffer()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
    releaseVertexStaging();
    mSize = 0;

    if (mVkVertexSlice.buffer != VK_NULL_HANDLE)
    {
        LLVKLoader::megabufReleaseVertex(mVkVertexSlice);
        mVkVertexSlice = LLVKLoader::MegaSliceV();
    }
}

void LLVertexBuffer::destroyGLIndices()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
    releaseIndexStaging();
    mIndicesSize = 0;

    if (mVkIndexSlice.buffer != VK_NULL_HANDLE)
    {
        LLVKLoader::megabufReleaseIndex(mVkIndexSlice);
        mVkIndexSlice = LLVKLoader::MegaSliceI();
    }
}

bool LLVertexBuffer::updateNumVerts(U32 nverts)
{
    llassert(nverts >= 0);

    bool success = true;

    U32 needed_size = calcOffsets(mTypeMask, mOffsets, nverts);

    mNumVerts = nverts;

    if (needed_size != mSize)
    {
        success &= createGLBuffer(needed_size);
    }

    llassert(mSize == needed_size);
    return success;
}

bool LLVertexBuffer::updateNumIndices(U32 nindices)
{
    llassert(nindices >= 0);

    bool success = true;

    U32 needed_size = sizeof(U16) * nindices;

    if (needed_size != mIndicesSize)
    {
        success &= createGLIndices(needed_size);
    }

    llassert(mIndicesSize == needed_size);
    mNumIndices = nindices;
    return success;
}

bool LLVertexBuffer::allocateBuffer(U32 nverts, U32 nindices)
{
    if (nverts < 0 || nindices < 0)
    {
        LL_ERRS() << "Bad vertex buffer allocation: " << nverts << " : " << nindices << LL_ENDL;
    }

    bool success = true;

    success &= updateNumVerts(nverts);
    success &= updateNumIndices(nindices);

    return success;
}


// if no gap between region and given range exists, expand region to cover given range and return true
// otherwise return false
bool expand_region(LLVertexBuffer::MappedRegion& region, U32 start, U32 end)
{

    if (end < region.mStart ||
        start > region.mEnd)
    { //gap exists, do not merge
        return false;
    }

    region.mStart = llmin(region.mStart, start);
    region.mEnd = llmax(region.mEnd, end);

    return true;
}


U8* LLVertexBuffer::ensureVertexStaging()
{
    if (mMappedData == nullptr && mSize > 0 && sVBOPool)
    {
        sVBOPool->allocate(GL_ARRAY_BUFFER, mSize, mMappedData);
    }
    return mMappedData;
}

U8* LLVertexBuffer::ensureIndexStaging()
{
    if (mMappedIndexData == nullptr && mIndicesSize > 0 && sVBOPool)
    {
        sVBOPool->allocate(GL_ELEMENT_ARRAY_BUFFER, mIndicesSize, mMappedIndexData);
    }
    return mMappedIndexData;
}

// Map for data access
U8* LLVertexBuffer::mapVertexBuffer(LLVertexBuffer::AttributeType type, U32 index, S32 count)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
    _mapBuffer();

    if (ensureVertexStaging() == nullptr)
    {
        return nullptr;
    }

    if (count == -1)
    {
        count = mNumVerts - index;
    }

    {
        U32 start = mOffsets[type] + sTypeSize[type] * index;
        U32 end = start + sTypeSize[type] * count-1;

        bool flagged = false;
        // flag region as mapped
        for (U32 i = 0; i < mMappedVertexRegions.size(); ++i)
        {
            MappedRegion& region = mMappedVertexRegions[i];
            if (expand_region(region, start, end))
            {
                flagged = true;
                break;
            }
        }

        if (!flagged)
        {
            //didn't expand an existing region, make a new one
            mMappedVertexRegions.push_back({ start, end });
        }
    }
    return mMappedData+mOffsets[type]+sTypeSize[type]*index;
}


U8* LLVertexBuffer::mapIndexBuffer(U32 index, S32 count)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
    _mapBuffer();

    if (ensureIndexStaging() == nullptr)
    {
        return nullptr;
    }

    if (count == -1)
    {
        count = mNumIndices-index;
    }

    {
        U32 start = sizeof(U16) * index;
        U32 end = start + sizeof(U16) * count-1;

        bool flagged = false;
        // flag region as mapped
        for (U32 i = 0; i < mMappedIndexRegions.size(); ++i)
        {
            MappedRegion& region = mMappedIndexRegions[i];
            if (expand_region(region, start, end))
            {
                flagged = true;
                break;
            }
        }

        if (!flagged)
        {
            //didn't expand an existing region, make a new one
            mMappedIndexRegions.push_back({ start, end });
        }
    }

    return mMappedIndexData + sizeof(U16)*index;
}

void LLVertexBuffer::zeroVertexData()
{
    if (mSize == 0)
    {
        return;
    }
    _mapBuffer();
    if (ensureVertexStaging() == nullptr)
    {
        return;
    }
    memset(mMappedData, 0, mSize);
    mMappedVertexRegions.clear();
    mMappedVertexRegions.push_back({ 0, mSize - 1 });
}

void LLVertexBuffer::zeroIndexData()
{
    if (mIndicesSize == 0)
    {
        return;
    }
    _mapBuffer();
    if (ensureIndexStaging() == nullptr)
    {
        return;
    }
    memset(mMappedIndexData, 0, mIndicesSize);
    mMappedIndexRegions.clear();
    mMappedIndexRegions.push_back({ 0, mIndicesSize - 1 });
}

// flush the given byte range
//  start -- first byte to copy
//  end -- last byte to copy (NOT last byte + 1)
//  data -- data to be flushed
//  dst -- mMappedData or mMappedIndexData
void LLVertexBuffer::flush_vbo(GLenum target, U32 start, U32 end, void* data, U8* dst)
{
    if (end == 0)
    {
        return;
    }
    if (target == GL_ARRAY_BUFFER && mVkVertexSlice.mapped != nullptr)
    {
        const U8* src = (const U8*)data;
        for (U32 i = 0; i < TYPE_TEXTURE_INDEX; ++i)
        {
            if (!(mTypeMask & (1u << i)))
            {
                continue;
            }
            const U32 block_start = mOffsets[i];
            const U32 block_end   = block_start + sTypeSize[i] * mNumVerts - 1;
            const U32 s = llmax(start, block_start);
            const U32 e = llmin(end, block_end);
            if (s > e)
            {
                continue;
            }
            U8* out = mVkVertexSlice.mapped
                    + mVkVertexSlice.region_offsets[i]
                    + (size_t)mVkVertexSlice.first * sTypeSize[i]
                    + (s - block_start);
            std::memcpy(out, src + (s - start), e - s + 1);
        }
    }
    else if (target == GL_ELEMENT_ARRAY_BUFFER && mVkIndexSlice.mapped != nullptr)
    {
        std::memcpy(mVkIndexSlice.mapped + mVkIndexSlice.offset + start, data, end - start + 1);
    }
}

void LLVertexBuffer::unmapBuffer()
{
    flushBuffers();
}

void LLVertexBuffer::_mapBuffer()
{
    if (!mMapped)
    {
        mMapped = true;
        sMappedBuffers.push_back(this);
    }
}

void LLVertexBuffer::_unmapBuffer()
{
    if (!mMapped)
    {
        return;
    }

    struct SortMappedRegion
    {
        bool operator()(const MappedRegion& lhs, const MappedRegion& rhs)
        {
            return lhs.mStart < rhs.mStart;
        }
    };

    {
        if (!mMappedVertexRegions.empty())
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("unmapBuffer - vertex");

            U32 start = 0;
            U32 end = 0;

            std::sort(mMappedVertexRegions.begin(), mMappedVertexRegions.end(), SortMappedRegion());

            for (U32 i = 0; i < mMappedVertexRegions.size(); ++i)
            {
                const MappedRegion& region = mMappedVertexRegions[i];
                if (region.mStart == end + 1)
                {
                    end = region.mEnd;
                }
                else
                {
                    flush_vbo(GL_ARRAY_BUFFER, start, end, (U8*)mMappedData + start, mMappedData);
                    start = region.mStart;
                    end = region.mEnd;
                }
            }

            flush_vbo(GL_ARRAY_BUFFER, start, end, (U8*)mMappedData + start, mMappedData);
            mMappedVertexRegions.clear();
        }

        if (!mMappedIndexRegions.empty())
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("unmapBuffer - index");

            U32 start = 0;
            U32 end = 0;

            std::sort(mMappedIndexRegions.begin(), mMappedIndexRegions.end(), SortMappedRegion());

            for (U32 i = 0; i < mMappedIndexRegions.size(); ++i)
            {
                const MappedRegion& region = mMappedIndexRegions[i];
                if (region.mStart == end + 1)
                {
                    end = region.mEnd;
                }
                else
                {
                    flush_vbo(GL_ELEMENT_ARRAY_BUFFER, start, end, (U8*)mMappedIndexData + start, mMappedIndexData);
                    start = region.mStart;
                    end = region.mEnd;
                }
            }

            flush_vbo(GL_ELEMENT_ARRAY_BUFFER, start, end, (U8*)mMappedIndexData + start, mMappedIndexData);
            mMappedIndexRegions.clear();
        }
    }

    if (!mStagingPersistent)
    {
        releaseVertexStaging();
        releaseIndexStaging();
    }
}


template <class T,LLVertexBuffer::AttributeType type> struct VertexBufferStrider
{
    typedef LLStrider<T> strider_t;
    static bool get(LLVertexBuffer& vbo,
                    strider_t& strider,
                    S32 index, S32 count)
    {
        if (type == LLVertexBuffer::TYPE_INDEX)
        {
            U8* ptr = vbo.mapIndexBuffer(index, count);

            if (ptr == NULL)
            {
                LL_WARNS() << "mapIndexBuffer failed!" << LL_ENDL;
                return false;
            }

            strider = (T*)ptr;
            strider.setStride(0);
            return true;
        }
        else if (vbo.hasDataType(type))
        {
            U32 stride = LLVertexBuffer::sTypeSize[type];

            U8* ptr = vbo.mapVertexBuffer(type, index, count);

            if (ptr == NULL)
            {
                LL_WARNS() << "mapVertexBuffer failed!" << LL_ENDL;
                return false;
            }

            strider = (T*)ptr;
            strider.setStride(stride);
            return true;
        }
        else
        {
            LL_ERRS() << "VertexBufferStrider could not find valid vertex data." << LL_ENDL;
        }
        return false;
    }
};

bool LLVertexBuffer::getVertexStrider(LLStrider<LLVector3>& strider, U32 index, S32 count)
{
    return VertexBufferStrider<LLVector3,TYPE_VERTEX>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getVertexStrider(LLStrider<LLVector4a>& strider, U32 index, S32 count)
{
    return VertexBufferStrider<LLVector4a,TYPE_VERTEX>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getIndexStrider(LLStrider<U16>& strider, U32 index, S32 count)
{
    llassert(mIndicesStride == 2); // cannot access 32-bit indices with U16 strider
    llassert(mIndicesType == GL_UNSIGNED_SHORT);
    return VertexBufferStrider<U16,TYPE_INDEX>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getTexCoord0Strider(LLStrider<LLVector2>& strider, U32 index, S32 count)
{
    return VertexBufferStrider<LLVector2,TYPE_TEXCOORD0>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getTexCoord1Strider(LLStrider<LLVector2>& strider, U32 index, S32 count)
{
    return VertexBufferStrider<LLVector2,TYPE_TEXCOORD1>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getTexCoord2Strider(LLStrider<LLVector2>& strider, U32 index, S32 count)
{
    return VertexBufferStrider<LLVector2,TYPE_TEXCOORD2>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getNormalStrider(LLStrider<LLVector3>& strider, U32 index, S32 count)
{
    return VertexBufferStrider<LLVector3,TYPE_NORMAL>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getNormalStrider(LLStrider<LLVector4a>& strider, U32 index, S32 count)
{
    return VertexBufferStrider<LLVector4a, TYPE_NORMAL>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getTangentStrider(LLStrider<LLVector3>& strider, U32 index, S32 count)
{
    return VertexBufferStrider<LLVector3,TYPE_TANGENT>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getTangentStrider(LLStrider<LLVector4a>& strider, U32 index, S32 count)
{
    return VertexBufferStrider<LLVector4a,TYPE_TANGENT>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getColorStrider(LLStrider<LLColor4U>& strider, U32 index, S32 count)
{
    return VertexBufferStrider<LLColor4U,TYPE_COLOR>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getEmissiveStrider(LLStrider<LLColor4U>& strider, U32 index, S32 count)
{
    return VertexBufferStrider<LLColor4U,TYPE_EMISSIVE>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getWeightStrider(LLStrider<F32>& strider, U32 index, S32 count)
{
    return VertexBufferStrider<F32,TYPE_WEIGHT>::get(*this, strider, index, count);
}

// <FS:Ansariel> Vectorized Weight4Strider and ClothWeightStrider by Drake Arconis
//bool LLVertexBuffer::getWeight4Strider(LLStrider<LLVector4>& strider, U32 index, S32 count)
//{
//  return VertexBufferStrider<LLVector4,TYPE_WEIGHT4>::get(*this, strider, index, count);
//}
//
//bool LLVertexBuffer::getClothWeightStrider(LLStrider<LLVector4>& strider, U32 index, S32 count)
//{
//  return VertexBufferStrider<LLVector4,TYPE_CLOTHWEIGHT>::get(*this, strider, index, count);
//}
bool LLVertexBuffer::getWeight4Strider(LLStrider<LLVector4a>& strider, U32 index, S32 count)
{
    return VertexBufferStrider<LLVector4a,TYPE_WEIGHT4>::get(*this, strider, index, count);
}

bool LLVertexBuffer::getClothWeightStrider(LLStrider<LLVector4a>& strider, U32 index, S32 count)
{
    return VertexBufferStrider<LLVector4a,TYPE_CLOTHWEIGHT>::get(*this, strider, index, count);
}
// </FS:Ansariel>



// Set for rendering
void LLVertexBuffer::setBuffer()
{

    if (mMapped)
    {
        LL_WARNS_ONCE() << "Missing call to unmapBuffer or flushBuffers" << LL_ENDL;
        _unmapBuffer();
    }

    // no data may be pending
    llassert(mMappedVertexRegions.empty());
    llassert(mMappedIndexRegions.empty());

    // a shader must be bound
    llassert(LLGLSLShader::sCurBoundShaderPtr);

    U32 data_mask = LLGLSLShader::sCurBoundShaderPtr->mVkAttributeMask;

    // this Vertex Buffer must provide all necessary attributes for currently bound shader
    llassert_msg((data_mask & mTypeMask) == data_mask,
        "Attribute mask mismatch! mTypeMask should be a superset of data_mask.  data_mask: 0x"
                << std::hex << data_mask << " mTypeMask: 0x" << mTypeMask << " Missing: 0x" << (data_mask & ~mTypeMask) <<  std::dec);

    if (LLVKLoader::shouldUseVulkanRender() && mVkVertexSlice.buffer != VK_NULL_HANDLE
        && LLGLSLShader::sCurBoundShaderPtr->mVkPipelineLayout != VK_NULL_HANDLE)
    {
        const U32 vk_data_mask = LLGLSLShader::sCurBoundShaderPtr->mVkAttributeMask;

        llassert_msg((vk_data_mask & mTypeMask) == vk_data_mask,
            "VK attribute mask mismatch! mTypeMask should be a superset of vk_data_mask.  vk_data_mask: 0x"
                    << std::hex << vk_data_mask << " mTypeMask: 0x" << mTypeMask << " Missing: 0x" << (vk_data_mask & ~mTypeMask) << std::dec);

        VkCommandBuffer cmd = LLVKLoader::getCurrentCommandBuffer();
        if (cmd != VK_NULL_HANDLE)
        {
            const U32* region = mVkVertexSlice.region_offsets;
            for (U32 type = 0; type < TYPE_MAX; ++type)
            {
                if (!(vk_data_mask & (1u << type)))
                    continue;
                VkDeviceSize buf_offset = (type == TYPE_TEXTURE_INDEX)
                                              ? ((VkDeviceSize)region[TYPE_VERTEX] + 12)
                                              : region[type];
                LLVKLoader::bindVertexBufferVk(cmd, mVkVertexSlice.buffer, buf_offset, type);
            }
            if (mVkIndexSlice.buffer != VK_NULL_HANDLE)
            {
                VkIndexType index_type = (mIndicesType == GL_UNSIGNED_INT)
                                             ? VK_INDEX_TYPE_UINT32
                                             : VK_INDEX_TYPE_UINT16;
                LLVKLoader::bindIndexBufferVk(cmd, mVkIndexSlice.buffer, 0, index_type);
            }
        }
    }

}

void LLVertexBuffer::setPositionData(const LLVector4a* data)
{
    flush_vbo(GL_ARRAY_BUFFER, 0, sizeof(LLVector4a) * getNumVerts()-1, (U8*) data, mMappedData);
}

void LLVertexBuffer::setTexCoord0Data(const LLVector2* data)
{
    flush_vbo(GL_ARRAY_BUFFER, mOffsets[TYPE_TEXCOORD0], mOffsets[TYPE_TEXCOORD0] + sTypeSize[TYPE_TEXCOORD0] * getNumVerts() - 1, (U8*)data, mMappedData);
}

void LLVertexBuffer::setTexCoord1Data(const LLVector2* data)
{
    flush_vbo(GL_ARRAY_BUFFER, mOffsets[TYPE_TEXCOORD1], mOffsets[TYPE_TEXCOORD1] + sTypeSize[TYPE_TEXCOORD1] * getNumVerts() - 1, (U8*)data, mMappedData);
}

void LLVertexBuffer::setColorData(const LLColor4U* data)
{
    flush_vbo(GL_ARRAY_BUFFER, mOffsets[TYPE_COLOR], mOffsets[TYPE_COLOR] + sTypeSize[TYPE_COLOR] * getNumVerts() - 1, (U8*) data, mMappedData);
}

void LLVertexBuffer::setNormalData(const LLVector4a* data)
{
    flush_vbo(GL_ARRAY_BUFFER, mOffsets[TYPE_NORMAL], mOffsets[TYPE_NORMAL] + sTypeSize[TYPE_NORMAL] * getNumVerts() - 1, (U8*) data, mMappedData);
}

void LLVertexBuffer::setTangentData(const LLVector4a* data)
{
    flush_vbo(GL_ARRAY_BUFFER, mOffsets[TYPE_TANGENT], mOffsets[TYPE_TANGENT] + sTypeSize[TYPE_TANGENT] * getNumVerts() - 1, (U8*) data, mMappedData);
}

void LLVertexBuffer::setWeight4Data(const LLVector4a* data)
{
    flush_vbo(GL_ARRAY_BUFFER, mOffsets[TYPE_WEIGHT4], mOffsets[TYPE_WEIGHT4] + sTypeSize[TYPE_WEIGHT4] * getNumVerts() - 1, (U8*) data, mMappedData);
}

void LLVertexBuffer::setJointData(const U64* data)
{
    flush_vbo(GL_ARRAY_BUFFER, mOffsets[TYPE_JOINT], mOffsets[TYPE_JOINT] + sTypeSize[TYPE_JOINT] * getNumVerts() - 1, (U8*) data, mMappedData);
}

void LLVertexBuffer::setIndexData(const U16* data)
{
    flush_vbo(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(U16) * getNumIndices() - 1, (U8*) data, mMappedIndexData);
}

void LLVertexBuffer::setIndexData(const U32* data)
{
    if (mIndicesType != GL_UNSIGNED_INT)
    { // HACK -- vertex buffers are initialized as 16-bit indices, but can be switched to 32-bit indices
        mIndicesType = GL_UNSIGNED_INT;
        mIndicesStride = 4;
        mNumIndices /= 2;
    }
    flush_vbo(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(U32) * getNumIndices() - 1, (U8*)data, mMappedIndexData);
}

void LLVertexBuffer::setPositionData(const LLVector4a* data, U32 offset, U32 count)
{
    flush_vbo(GL_ARRAY_BUFFER, offset * sizeof(LLVector4a), (offset + count) * sizeof(LLVector4a) - 1, (U8*)data, mMappedData);
}

void LLVertexBuffer::setNormalData(const LLVector4a* data, U32 offset, U32 count)
{
    flush_vbo(GL_ARRAY_BUFFER, mOffsets[TYPE_NORMAL] + offset * sTypeSize[TYPE_NORMAL], mOffsets[TYPE_NORMAL] + (offset + count) * sTypeSize[TYPE_NORMAL] - 1, (U8*)data, mMappedData);
}

void LLVertexBuffer::setTexCoord0Data(const LLVector2* data, U32 offset, U32 count)
{
    flush_vbo(GL_ARRAY_BUFFER, mOffsets[TYPE_TEXCOORD0] + offset * sTypeSize[TYPE_TEXCOORD0], mOffsets[TYPE_TEXCOORD0] + (offset + count) * sTypeSize[TYPE_TEXCOORD0] - 1, (U8*)data, mMappedData);
}

void LLVertexBuffer::setTexCoord1Data(const LLVector2* data, U32 offset, U32 count)
{
    flush_vbo(GL_ARRAY_BUFFER, mOffsets[TYPE_TEXCOORD1] + offset * sTypeSize[TYPE_TEXCOORD1], mOffsets[TYPE_TEXCOORD1] + (offset + count) * sTypeSize[TYPE_TEXCOORD1] - 1, (U8*)data, mMappedData);
}

void LLVertexBuffer::setColorData(const LLColor4U* data, U32 offset, U32 count)
{
    flush_vbo(GL_ARRAY_BUFFER, mOffsets[TYPE_COLOR] + offset * sTypeSize[TYPE_COLOR], mOffsets[TYPE_COLOR] + (offset + count) * sTypeSize[TYPE_COLOR] - 1, (U8*)data, mMappedData);
}

void LLVertexBuffer::setTangentData(const LLVector4a* data, U32 offset, U32 count)
{
    flush_vbo(GL_ARRAY_BUFFER, mOffsets[TYPE_TANGENT] + offset * sTypeSize[TYPE_TANGENT], mOffsets[TYPE_TANGENT] + (offset + count) * sTypeSize[TYPE_TANGENT] - 1, (U8*)data, mMappedData);
}

void LLVertexBuffer::setWeight4Data(const LLVector4a* data, U32 offset, U32 count)
{
    flush_vbo(GL_ARRAY_BUFFER, mOffsets[TYPE_WEIGHT4] + offset * sTypeSize[TYPE_WEIGHT4], mOffsets[TYPE_WEIGHT4] + (offset + count) * sTypeSize[TYPE_WEIGHT4] - 1, (U8*)data, mMappedData);
}

void LLVertexBuffer::setJointData(const U64* data, U32 offset, U32 count)
{
    flush_vbo(GL_ARRAY_BUFFER, mOffsets[TYPE_JOINT] + offset * sTypeSize[TYPE_JOINT], mOffsets[TYPE_JOINT] + (offset + count) * sTypeSize[TYPE_JOINT] - 1, (U8*)data, mMappedData);
}

void LLVertexBuffer::setIndexData(const U16* data, U32 offset, U32 count)
{
    flush_vbo(GL_ELEMENT_ARRAY_BUFFER, offset * sizeof(U16), (offset + count) * sizeof(U16) - 1, (U8*)data, mMappedIndexData);
}

void LLVertexBuffer::setIndexData(const U32* data, U32 offset, U32 count)
{
    if (mIndicesType != GL_UNSIGNED_INT)
    { // HACK -- vertex buffers are initialized as 16-bit indices, but can be switched to 32-bit indices
        mIndicesType = GL_UNSIGNED_INT;
        mIndicesStride = 4;
        mNumIndices /= 2;
    }
    flush_vbo(GL_ELEMENT_ARRAY_BUFFER, offset * sizeof(U32), (offset + count) * sizeof(U32) - 1, (U8*)data, mMappedIndexData);
}




