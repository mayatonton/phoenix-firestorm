#ifndef LL_VKTEXRESIDENCY_H
#define LL_VKTEXRESIDENCY_H

#include <atomic>

#include "stdtypes.h"
#include "volk.h"

struct VkBacking
{
    VkImage       image  = VK_NULL_HANDLE;
    VkImageView   view   = VK_NULL_HANDLE;
    void*         alloc  = nullptr;
    U32           width  = 0;
    U32           height = 0;
    U32           mips   = 1;
    VkFormat      format = VK_FORMAT_UNDEFINED;
    bool          owned  = true;

    bool valid() const { return view != VK_NULL_HANDLE; }
};

class VkTexResidency
{
public:
    bool        isLive() const { return mView.load(std::memory_order_acquire) != VK_NULL_HANDLE; }
    bool        hasBacking() const { return mCur.valid(); }
    bool        hasStaged() const { return mStagedValid; }
    VkImageView view()   const { return mView.load(std::memory_order_acquire); }
    U32         slot()   const { return mSlot.load(std::memory_order_relaxed); }
    VkImage     image()  const { return mCur.image; }
    U32         mips()   const { return mCur.mips; }
    U32         width()  const { return mCur.width; }
    U32         height() const { return mCur.height; }
    VkFormat    format() const { return mCur.format; }

    bool commit(const VkBacking& next, VkSampler sampler, bool want_slot);
    void resample(VkSampler s);
    void retire();
    U32  ensureSlot(VkSampler sampler);
    bool publishStaged(VkSampler sampler, bool want_slot);
    void discardStaged();

private:
    bool publish(VkImageView view, VkSampler sampler, const VkBacking* next, bool want_slot);

    std::atomic<U32>         mSlot{0xFFFFFFFFu};
    std::atomic<VkImageView> mView{VK_NULL_HANDLE};
    VkBacking                mCur;
    VkBacking                mStagedPrev;
    bool                     mStagedValid = false;
};

#endif
