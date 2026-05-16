/**
 * @file llpluginaudio_test.cpp
 * @brief Tests for media plugin audio ring helpers.
 */

#include "linden_common.h"

#include "llpluginaudio.h"
#include "../test/lltut.h"

namespace tut
{
    struct llpluginaudio_data
    {
    };

    typedef test_group<llpluginaudio_data> llpluginaudio_group;
    typedef llpluginaudio_group::object object;
    llpluginaudio_group llpluginaudio("llpluginaudio");

    template<> template<>
    void object::test<1>()
    {
        set_test_name("ring allocation includes sentinel frame");

        constexpr std::uint32_t capacity = 48000 * 2;
        ensure_equals("ring total frames",
                      ll_plugin_audio_ring_total_frames(capacity),
                      capacity + 1);
        ensure_equals("ring sample count",
                      ll_plugin_audio_ring_sample_count(capacity),
                      static_cast<std::size_t>(capacity + 1) *
                          LL_PLUGIN_AUDIO_RING_MAX_CHANNELS);
        ensure_equals("ring sample bytes",
                      ll_plugin_audio_ring_sample_bytes(capacity),
                      static_cast<std::size_t>(capacity + 1) *
                          LL_PLUGIN_AUDIO_RING_MAX_CHANNELS *
                          sizeof(float));
    }

    template<> template<>
    void object::test<2>()
    {
        set_test_name("shared memory size covers highest ring index");

        constexpr std::uint32_t capacity = 16;
        const std::size_t total_size =
            ll_plugin_audio_ring_shared_memory_size(capacity);
        const std::size_t highest_sample_offset =
            sizeof(LLPluginAudioRingHeader) +
            (static_cast<std::size_t>(capacity) *
             LL_PLUGIN_AUDIO_RING_MAX_CHANNELS * sizeof(float));
        const std::size_t highest_frame_end =
            highest_sample_offset +
            (LL_PLUGIN_AUDIO_RING_MAX_CHANNELS * sizeof(float));

        ensure("index capacity fits in shared memory",
               highest_frame_end <= total_size);
    }

    template<> template<>
    void object::test<3>()
    {
        set_test_name("supported 3D media channel counts");

        ensure("1ch is supported",
               ll_plugin_audio_ring_supported_3d_channel_count(1));
        ensure("2ch is supported",
               ll_plugin_audio_ring_supported_3d_channel_count(2));
        ensure("6ch is supported",
               ll_plugin_audio_ring_supported_3d_channel_count(6));

        ensure("0ch is not supported",
               !ll_plugin_audio_ring_supported_3d_channel_count(0));
        ensure("3ch is not supported",
               !ll_plugin_audio_ring_supported_3d_channel_count(3));
        ensure("4ch is not supported",
               !ll_plugin_audio_ring_supported_3d_channel_count(4));
        ensure("5ch is not supported",
               !ll_plugin_audio_ring_supported_3d_channel_count(5));
        ensure("7ch is not supported",
               !ll_plugin_audio_ring_supported_3d_channel_count(7));
        ensure("8ch is not supported",
               !ll_plugin_audio_ring_supported_3d_channel_count(8));
    }

    template<> template<>
    void object::test<4>()
    {
        set_test_name("6ch media order matches 3D Stream routing order");

        ensure_equals("FL index",
                      static_cast<std::uint32_t>(LLPluginAudioSixChannel::FrontLeft),
                      0u);
        ensure_equals("FR index",
                      static_cast<std::uint32_t>(LLPluginAudioSixChannel::FrontRight),
                      1u);
        ensure_equals("C index",
                      static_cast<std::uint32_t>(LLPluginAudioSixChannel::Center),
                      2u);
        ensure_equals("LFE index",
                      static_cast<std::uint32_t>(LLPluginAudioSixChannel::Lfe),
                      3u);
        ensure_equals("SL index",
                      static_cast<std::uint32_t>(LLPluginAudioSixChannel::SurroundLeft),
                      4u);
        ensure_equals("SR index",
                      static_cast<std::uint32_t>(LLPluginAudioSixChannel::SurroundRight),
                      5u);
    }
}
