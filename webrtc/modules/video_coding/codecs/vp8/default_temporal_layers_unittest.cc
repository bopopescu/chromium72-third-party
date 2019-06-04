/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "modules/video_coding/codecs/vp8/default_temporal_layers.h"
#include "modules/video_coding/codecs/vp8/libvpx_vp8_encoder.h"
#include "modules/video_coding/include/video_codec_interface.h"
#include "modules/video_coding/utility/simulcast_rate_allocator.h"
#include "test/field_trial.h"
#include "test/gtest.h"

namespace webrtc {
namespace test {
namespace {
enum {
  kTemporalUpdateLast = VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_UPD_ARF |
                        VP8_EFLAG_NO_REF_GF | VP8_EFLAG_NO_REF_ARF,
  kTemporalUpdateGoldenWithoutDependency =
      VP8_EFLAG_NO_REF_GF | VP8_EFLAG_NO_REF_ARF | VP8_EFLAG_NO_UPD_ARF |
      VP8_EFLAG_NO_UPD_LAST,
  kTemporalUpdateGolden =
      VP8_EFLAG_NO_REF_ARF | VP8_EFLAG_NO_UPD_ARF | VP8_EFLAG_NO_UPD_LAST,
  kTemporalUpdateAltrefWithoutDependency =
      VP8_EFLAG_NO_REF_ARF | VP8_EFLAG_NO_REF_GF | VP8_EFLAG_NO_UPD_GF |
      VP8_EFLAG_NO_UPD_LAST,
  kTemporalUpdateAltref = VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_UPD_LAST,
  kTemporalUpdateNone = VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_UPD_ARF |
                        VP8_EFLAG_NO_UPD_LAST | VP8_EFLAG_NO_UPD_ENTROPY,
  kTemporalUpdateNoneNoRefAltRef =
      VP8_EFLAG_NO_REF_ARF | VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_UPD_ARF |
      VP8_EFLAG_NO_UPD_LAST | VP8_EFLAG_NO_UPD_ENTROPY,
  kTemporalUpdateNoneNoRefGolden =
      VP8_EFLAG_NO_REF_GF | VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_UPD_ARF |
      VP8_EFLAG_NO_UPD_LAST | VP8_EFLAG_NO_UPD_ENTROPY,
  kTemporalUpdateNoneNoRefGoldenAltRef =
      VP8_EFLAG_NO_REF_GF | VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_REF_ARF |
      VP8_EFLAG_NO_UPD_ARF | VP8_EFLAG_NO_UPD_LAST | VP8_EFLAG_NO_UPD_ENTROPY,
  kTemporalUpdateGoldenWithoutDependencyRefAltRef =
      VP8_EFLAG_NO_REF_GF | VP8_EFLAG_NO_UPD_ARF | VP8_EFLAG_NO_UPD_LAST,
  kTemporalUpdateGoldenRefAltRef = VP8_EFLAG_NO_UPD_ARF | VP8_EFLAG_NO_UPD_LAST,
  kTemporalUpdateLastRefAltRef =
      VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_UPD_ARF | VP8_EFLAG_NO_REF_GF,
  kTemporalUpdateLastAndGoldenRefAltRef =
      VP8_EFLAG_NO_UPD_ARF | VP8_EFLAG_NO_REF_GF,
};

std::vector<uint32_t> GetTemporalLayerRates(int target_bitrate_kbps,
                                            int framerate_fps,
                                            int num_temporal_layers) {
  VideoCodec codec;
  codec.codecType = VideoCodecType::kVideoCodecVP8;
  codec.numberOfSimulcastStreams = 1;
  codec.maxBitrate = target_bitrate_kbps;
  codec.maxFramerate = framerate_fps;
  codec.simulcastStream[0].targetBitrate = target_bitrate_kbps;
  codec.simulcastStream[0].maxBitrate = target_bitrate_kbps;
  codec.simulcastStream[0].numberOfTemporalLayers = num_temporal_layers;
  codec.simulcastStream[0].active = true;
  SimulcastRateAllocator allocator(codec);
  return allocator.GetAllocation(target_bitrate_kbps, framerate_fps)
      .GetTemporalLayerAllocation(0);
}

constexpr int kDefaultBitrateBps = 500;
constexpr int kDefaultFramerate = 30;
constexpr int kDefaultBytesPerFrame =
    (kDefaultBitrateBps / 8) / kDefaultFramerate;
constexpr int kDefaultQp = 2;
}  // namespace

using BufferFlags = Vp8TemporalLayers::BufferFlags;

TEST(TemporalLayersTest, 2Layers) {
  constexpr int kNumLayers = 2;
  DefaultTemporalLayers tl(kNumLayers);
  DefaultTemporalLayersChecker checker(kNumLayers);
  Vp8EncoderConfig cfg;
  CodecSpecificInfoVP8 vp8_info;
  tl.OnRatesUpdated(GetTemporalLayerRates(kDefaultBytesPerFrame,
                                          kDefaultFramerate, kNumLayers),
                    kDefaultFramerate);
  tl.UpdateConfiguration(&cfg);

  int expected_flags[16] = {
      kTemporalUpdateLastRefAltRef,
      kTemporalUpdateGoldenWithoutDependencyRefAltRef,
      kTemporalUpdateLastRefAltRef,
      kTemporalUpdateNone,
      kTemporalUpdateLastRefAltRef,
      kTemporalUpdateGoldenWithoutDependencyRefAltRef,
      kTemporalUpdateLastRefAltRef,
      kTemporalUpdateNone,
      kTemporalUpdateLastRefAltRef,
      kTemporalUpdateGoldenWithoutDependencyRefAltRef,
      kTemporalUpdateLastRefAltRef,
      kTemporalUpdateNone,
      kTemporalUpdateLastRefAltRef,
      kTemporalUpdateGoldenWithoutDependencyRefAltRef,
      kTemporalUpdateLastRefAltRef,
      kTemporalUpdateNone,
  };
  int expected_temporal_idx[16] = {0, 1, 0, 1, 0, 1, 0, 1,
                                   0, 1, 0, 1, 0, 1, 0, 1};

  bool expected_layer_sync[16] = {false, true,  false, false, false, true,
                                  false, false, false, true,  false, false,
                                  false, true,  false, false};

  uint32_t timestamp = 0;
  for (int i = 0; i < 16; ++i) {
    Vp8TemporalLayers::FrameConfig tl_config = tl.UpdateLayerConfig(timestamp);
    EXPECT_EQ(expected_flags[i], LibvpxVp8Encoder::EncodeFlags(tl_config)) << i;
    tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, i == 0, kDefaultQp,
                    &vp8_info);
    EXPECT_TRUE(checker.CheckTemporalConfig(i == 0, tl_config));
    EXPECT_EQ(expected_temporal_idx[i], vp8_info.temporalIdx);
    EXPECT_EQ(expected_temporal_idx[i], tl_config.packetizer_temporal_idx);
    EXPECT_EQ(expected_temporal_idx[i], tl_config.encoder_layer_id);
    EXPECT_EQ(i == 0 || expected_layer_sync[i], vp8_info.layerSync);
    EXPECT_EQ(expected_layer_sync[i], tl_config.layer_sync);
    timestamp += 3000;
  }
}

TEST(TemporalLayersTest, 3Layers) {
  constexpr int kNumLayers = 3;
  DefaultTemporalLayers tl(kNumLayers);
  DefaultTemporalLayersChecker checker(kNumLayers);
  Vp8EncoderConfig cfg;
  CodecSpecificInfoVP8 vp8_info;
  tl.OnRatesUpdated(GetTemporalLayerRates(kDefaultBytesPerFrame,
                                          kDefaultFramerate, kNumLayers),
                    kDefaultFramerate);
  tl.UpdateConfiguration(&cfg);

  int expected_flags[16] = {
      kTemporalUpdateLastRefAltRef,
      kTemporalUpdateNoneNoRefGolden,
      kTemporalUpdateGoldenWithoutDependencyRefAltRef,
      kTemporalUpdateNone,
      kTemporalUpdateLastRefAltRef,
      kTemporalUpdateNone,
      kTemporalUpdateGoldenRefAltRef,
      kTemporalUpdateNone,
      kTemporalUpdateLastRefAltRef,
      kTemporalUpdateNoneNoRefGolden,
      kTemporalUpdateGoldenWithoutDependencyRefAltRef,
      kTemporalUpdateNone,
      kTemporalUpdateLastRefAltRef,
      kTemporalUpdateNone,
      kTemporalUpdateGoldenRefAltRef,
      kTemporalUpdateNone,
  };
  int expected_temporal_idx[16] = {0, 2, 1, 2, 0, 2, 1, 2,
                                   0, 2, 1, 2, 0, 2, 1, 2};

  bool expected_layer_sync[16] = {false, true,  true,  false, false, false,
                                  false, false, false, true,  true,  false,
                                  false, false, false, false};

  unsigned int timestamp = 0;
  for (int i = 0; i < 16; ++i) {
    Vp8TemporalLayers::FrameConfig tl_config = tl.UpdateLayerConfig(timestamp);
    EXPECT_EQ(expected_flags[i], LibvpxVp8Encoder::EncodeFlags(tl_config)) << i;
    tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, i == 0, kDefaultQp,
                    &vp8_info);
    EXPECT_TRUE(checker.CheckTemporalConfig(i == 0, tl_config));
    EXPECT_EQ(expected_temporal_idx[i], vp8_info.temporalIdx);
    EXPECT_EQ(expected_temporal_idx[i], tl_config.packetizer_temporal_idx);
    EXPECT_EQ(expected_temporal_idx[i], tl_config.encoder_layer_id);
    EXPECT_EQ(i == 0 || expected_layer_sync[i], vp8_info.layerSync);
    EXPECT_EQ(expected_layer_sync[i], tl_config.layer_sync);
    timestamp += 3000;
  }
}

TEST(TemporalLayersTest, Alternative3Layers) {
  constexpr int kNumLayers = 3;
  ScopedFieldTrials field_trial("WebRTC-UseShortVP8TL3Pattern/Enabled/");
  DefaultTemporalLayers tl(kNumLayers);
  DefaultTemporalLayersChecker checker(kNumLayers);
  Vp8EncoderConfig cfg;
  CodecSpecificInfoVP8 vp8_info;
  tl.OnRatesUpdated(GetTemporalLayerRates(kDefaultBytesPerFrame,
                                          kDefaultFramerate, kNumLayers),
                    kDefaultFramerate);
  tl.UpdateConfiguration(&cfg);

  int expected_flags[8] = {kTemporalUpdateLast,
                           kTemporalUpdateAltrefWithoutDependency,
                           kTemporalUpdateGoldenWithoutDependency,
                           kTemporalUpdateNone,
                           kTemporalUpdateLast,
                           kTemporalUpdateAltrefWithoutDependency,
                           kTemporalUpdateGoldenWithoutDependency,
                           kTemporalUpdateNone};
  int expected_temporal_idx[8] = {0, 2, 1, 2, 0, 2, 1, 2};

  bool expected_layer_sync[8] = {false, true, true, false,
                                 false, true, true, false};

  unsigned int timestamp = 0;
  for (int i = 0; i < 8; ++i) {
    Vp8TemporalLayers::FrameConfig tl_config = tl.UpdateLayerConfig(timestamp);
    EXPECT_EQ(expected_flags[i], LibvpxVp8Encoder::EncodeFlags(tl_config)) << i;
    tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, i == 0, kDefaultQp,
                    &vp8_info);
    EXPECT_TRUE(checker.CheckTemporalConfig(i == 0, tl_config));
    EXPECT_EQ(expected_temporal_idx[i], vp8_info.temporalIdx);
    EXPECT_EQ(expected_temporal_idx[i], tl_config.packetizer_temporal_idx);
    EXPECT_EQ(expected_temporal_idx[i], tl_config.encoder_layer_id);
    EXPECT_EQ(i == 0 || expected_layer_sync[i], vp8_info.layerSync);
    EXPECT_EQ(expected_layer_sync[i], tl_config.layer_sync);
    timestamp += 3000;
  }
}

TEST(TemporalLayersTest, SearchOrder) {
  constexpr int kNumLayers = 3;
  ScopedFieldTrials field_trial("WebRTC-UseShortVP8TL3Pattern/Enabled/");
  DefaultTemporalLayers tl(kNumLayers);
  DefaultTemporalLayersChecker checker(kNumLayers);
  Vp8EncoderConfig cfg;
  CodecSpecificInfoVP8 vp8_info;
  tl.OnRatesUpdated(GetTemporalLayerRates(kDefaultBytesPerFrame,
                                          kDefaultFramerate, kNumLayers),
                    kDefaultFramerate);
  tl.UpdateConfiguration(&cfg);

  // Use a repeating pattern of tl 0, 2, 1, 2.
  // Tl 0, 1, 2 update last, golden, altref respectively.

  // Start with a key-frame. tl_config flags can be ignored.
  uint32_t timestamp = 0;
  Vp8TemporalLayers::FrameConfig tl_config = tl.UpdateLayerConfig(timestamp);
  tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, true, kDefaultQp,
                  &vp8_info);

  // TL2 frame. First one only references TL0. Updates altref.
  tl_config = tl.UpdateLayerConfig(++timestamp);
  tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, false, kDefaultQp,
                  &vp8_info);
  EXPECT_EQ(tl_config.first_reference, Vp8BufferReference::kLast);
  EXPECT_EQ(tl_config.second_reference, Vp8BufferReference::kNone);

  // TL1 frame. Can only reference TL0. Updated golden.
  tl_config = tl.UpdateLayerConfig(++timestamp);
  tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, false, kDefaultQp,
                  &vp8_info);
  EXPECT_EQ(tl_config.first_reference, Vp8BufferReference::kLast);
  EXPECT_EQ(tl_config.second_reference, Vp8BufferReference::kNone);

  // TL2 frame. Can reference all three buffers. Golden was the last to be
  // updated, the next to last was altref.
  tl_config = tl.UpdateLayerConfig(++timestamp);
  tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, false, kDefaultQp,
                  &vp8_info);
  EXPECT_EQ(tl_config.first_reference, Vp8BufferReference::kGolden);
  EXPECT_EQ(tl_config.second_reference, Vp8BufferReference::kAltref);
}

TEST(TemporalLayersTest, SearchOrderWithDrop) {
  constexpr int kNumLayers = 3;
  ScopedFieldTrials field_trial("WebRTC-UseShortVP8TL3Pattern/Enabled/");
  DefaultTemporalLayers tl(kNumLayers);
  DefaultTemporalLayersChecker checker(kNumLayers);
  Vp8EncoderConfig cfg;
  CodecSpecificInfoVP8 vp8_info;
  tl.OnRatesUpdated(GetTemporalLayerRates(kDefaultBytesPerFrame,
                                          kDefaultFramerate, kNumLayers),
                    kDefaultFramerate);
  tl.UpdateConfiguration(&cfg);

  // Use a repeating pattern of tl 0, 2, 1, 2.
  // Tl 0, 1, 2 update last, golden, altref respectively.

  // Start with a key-frame. tl_config flags can be ignored.
  uint32_t timestamp = 0;
  Vp8TemporalLayers::FrameConfig tl_config = tl.UpdateLayerConfig(timestamp);
  tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, true, kDefaultQp,
                  &vp8_info);

  // TL2 frame. First one only references TL0. Updates altref.
  tl_config = tl.UpdateLayerConfig(++timestamp);
  tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, false, kDefaultQp,
                  &vp8_info);
  EXPECT_EQ(tl_config.first_reference, Vp8BufferReference::kLast);
  EXPECT_EQ(tl_config.second_reference, Vp8BufferReference::kNone);

  // Dropped TL1 frame. Can only reference TL0. Should have updated golden.
  tl_config = tl.UpdateLayerConfig(++timestamp);
  tl.OnEncodeDone(timestamp, 0, false, 0, nullptr);

  // TL2 frame. Can normally reference all three buffers, but golden has not
  // been populated this cycle. Altref was last to be updated, before that last.
  tl_config = tl.UpdateLayerConfig(++timestamp);
  tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, false, kDefaultQp,
                  &vp8_info);
  EXPECT_EQ(tl_config.first_reference, Vp8BufferReference::kAltref);
  EXPECT_EQ(tl_config.second_reference, Vp8BufferReference::kLast);
}

TEST(TemporalLayersTest, 4Layers) {
  constexpr int kNumLayers = 4;
  DefaultTemporalLayers tl(kNumLayers);
  DefaultTemporalLayersChecker checker(kNumLayers);
  Vp8EncoderConfig cfg;
  CodecSpecificInfoVP8 vp8_info;
  tl.OnRatesUpdated(GetTemporalLayerRates(kDefaultBytesPerFrame,
                                          kDefaultFramerate, kNumLayers),
                    kDefaultFramerate);
  tl.UpdateConfiguration(&cfg);
  int expected_flags[16] = {
      kTemporalUpdateLast,
      kTemporalUpdateNoneNoRefGoldenAltRef,
      kTemporalUpdateAltrefWithoutDependency,
      kTemporalUpdateNoneNoRefGolden,
      kTemporalUpdateGoldenWithoutDependency,
      kTemporalUpdateNone,
      kTemporalUpdateAltref,
      kTemporalUpdateNone,
      kTemporalUpdateLast,
      kTemporalUpdateNone,
      kTemporalUpdateAltref,
      kTemporalUpdateNone,
      kTemporalUpdateGolden,
      kTemporalUpdateNone,
      kTemporalUpdateAltref,
      kTemporalUpdateNone,
  };
  int expected_temporal_idx[16] = {0, 3, 2, 3, 1, 3, 2, 3,
                                   0, 3, 2, 3, 1, 3, 2, 3};

  bool expected_layer_sync[16] = {false, true,  true,  false, true,  false,
                                  false, false, false, false, false, false,
                                  false, false, false, false};

  uint32_t timestamp = 0;
  for (int i = 0; i < 16; ++i) {
    Vp8TemporalLayers::FrameConfig tl_config = tl.UpdateLayerConfig(timestamp);
    EXPECT_EQ(expected_flags[i], LibvpxVp8Encoder::EncodeFlags(tl_config)) << i;
    tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, i == 0, kDefaultQp,
                    &vp8_info);
    EXPECT_TRUE(checker.CheckTemporalConfig(i == 0, tl_config));
    EXPECT_EQ(expected_temporal_idx[i], vp8_info.temporalIdx);
    EXPECT_EQ(expected_temporal_idx[i], tl_config.packetizer_temporal_idx);
    EXPECT_EQ(expected_temporal_idx[i], tl_config.encoder_layer_id);
    EXPECT_EQ(i == 0 || expected_layer_sync[i], vp8_info.layerSync);
    EXPECT_EQ(expected_layer_sync[i], tl_config.layer_sync);
    timestamp += 3000;
  }
}

TEST(TemporalLayersTest, DoesNotReferenceDroppedFrames) {
  constexpr int kNumLayers = 3;
  // Use a repeating pattern of tl 0, 2, 1, 2.
  // Tl 0, 1, 2 update last, golden, altref respectively.
  ScopedFieldTrials field_trial("WebRTC-UseShortVP8TL3Pattern/Enabled/");
  DefaultTemporalLayers tl(kNumLayers);
  DefaultTemporalLayersChecker checker(kNumLayers);
  Vp8EncoderConfig cfg;
  CodecSpecificInfoVP8 vp8_info;
  tl.OnRatesUpdated(GetTemporalLayerRates(kDefaultBytesPerFrame,
                                          kDefaultFramerate, kNumLayers),
                    kDefaultFramerate);
  tl.UpdateConfiguration(&cfg);

  // Start with a keyframe.
  uint32_t timestamp = 0;
  Vp8TemporalLayers::FrameConfig tl_config = tl.UpdateLayerConfig(timestamp);
  tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, true, kDefaultQp,
                  &vp8_info);

  // Dropped TL2 frame.
  tl_config = tl.UpdateLayerConfig(++timestamp);
  tl.OnEncodeDone(timestamp, 0, false, 0, nullptr);

  // Dropped TL1 frame.
  tl_config = tl.UpdateLayerConfig(++timestamp);
  tl.OnEncodeDone(timestamp, 0, false, 0, nullptr);

  // TL2 frame. Can reference all three buffers, valid since golden and altref
  // both contain the last keyframe.
  tl_config = tl.UpdateLayerConfig(++timestamp);
  tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, false, kDefaultQp,
                  &vp8_info);
  EXPECT_TRUE(tl_config.last_buffer_flags & BufferFlags::kReference);
  EXPECT_TRUE(tl_config.golden_buffer_flags & BufferFlags::kReference);
  EXPECT_TRUE(tl_config.arf_buffer_flags & BufferFlags::kReference);

  // Restart of cycle!

  // TL0 base layer frame, updating and referencing last.
  tl_config = tl.UpdateLayerConfig(++timestamp);
  tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, false, kDefaultQp,
                  &vp8_info);

  // TL2 frame, updating altref.
  tl_config = tl.UpdateLayerConfig(++timestamp);
  tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, false, kDefaultQp,
                  &vp8_info);

  // TL1 frame, updating golden.
  tl_config = tl.UpdateLayerConfig(++timestamp);
  tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, false, kDefaultQp,
                  &vp8_info);

  // TL2 frame. Can still reference all buffer since they have been update this
  // cycle.
  tl_config = tl.UpdateLayerConfig(++timestamp);
  tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, false, kDefaultQp,
                  &vp8_info);
  EXPECT_TRUE(tl_config.last_buffer_flags & BufferFlags::kReference);
  EXPECT_TRUE(tl_config.golden_buffer_flags & BufferFlags::kReference);
  EXPECT_TRUE(tl_config.arf_buffer_flags & BufferFlags::kReference);

  // Restart of cycle!

  // TL0 base layer frame, updating and referencing last.
  tl_config = tl.UpdateLayerConfig(++timestamp);
  tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, false, kDefaultQp,
                  &vp8_info);

  // Dropped TL2 frame.
  tl_config = tl.UpdateLayerConfig(++timestamp);
  tl.OnEncodeDone(timestamp, 0, false, 0, nullptr);

  // Dropped TL1 frame.
  tl_config = tl.UpdateLayerConfig(++timestamp);
  tl.OnEncodeDone(timestamp, 0, false, 0, nullptr);

  // TL2 frame. This time golden and altref contain data from the previous cycle
  // and cannot be referenced.
  tl_config = tl.UpdateLayerConfig(++timestamp);
  tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, false, kDefaultQp,
                  &vp8_info);
  EXPECT_TRUE(tl_config.last_buffer_flags & BufferFlags::kReference);
  EXPECT_FALSE(tl_config.golden_buffer_flags & BufferFlags::kReference);
  EXPECT_FALSE(tl_config.arf_buffer_flags & BufferFlags::kReference);
}

TEST(TemporalLayersTest, DoesNotReferenceUnlessGuaranteedToExist) {
  constexpr int kNumLayers = 3;
  // Use a repeating pattern of tl 0, 2, 1, 2.
  // Tl 0, 1 updates last, golden respectively. Altref is always last keyframe.
  DefaultTemporalLayers tl(kNumLayers);
  DefaultTemporalLayersChecker checker(kNumLayers);
  Vp8EncoderConfig cfg;
  CodecSpecificInfoVP8 vp8_info;
  tl.OnRatesUpdated(GetTemporalLayerRates(kDefaultBytesPerFrame,
                                          kDefaultFramerate, kNumLayers),
                    kDefaultFramerate);
  tl.UpdateConfiguration(&cfg);

  // Start with a keyframe.
  uint32_t timestamp = 0;
  Vp8TemporalLayers::FrameConfig tl_config = tl.UpdateLayerConfig(timestamp);
  tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, true, kDefaultQp,
                  &vp8_info);

  // Do a full cycle of the pattern.
  for (int i = 0; i < 7; ++i) {
    tl_config = tl.UpdateLayerConfig(++timestamp);
    tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, false, kDefaultQp,
                    &vp8_info);
  }

  // TL0 base layer frame, starting the cycle over.
  tl_config = tl.UpdateLayerConfig(++timestamp);
  tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, false, kDefaultQp,
                  &vp8_info);

  // TL2 frame.
  tl_config = tl.UpdateLayerConfig(++timestamp);
  tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, false, kDefaultQp,
                  &vp8_info);

  // Encoder has a hiccup and builds a queue, so frame encoding is delayed.
  // TL1 frame, updating golden.
  tl_config = tl.UpdateLayerConfig(++timestamp);

  // TL2 frame, that should be referencing golden, but we can't be certain it's
  // not going to be dropped, so that is not allowed.
  tl_config = tl.UpdateLayerConfig(timestamp + 1);
  EXPECT_TRUE(tl_config.last_buffer_flags & BufferFlags::kReference);
  EXPECT_FALSE(tl_config.golden_buffer_flags & BufferFlags::kReference);
  EXPECT_TRUE(tl_config.arf_buffer_flags & BufferFlags::kReference);

  // TL0 base layer frame.
  tl_config = tl.UpdateLayerConfig(timestamp + 2);

  // The previous four enqueued frames finally get encoded, and the updated
  // buffers are now OK to reference.
  // Enqueued TL1 frame ready.
  tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, false, kDefaultQp,
                  &vp8_info);
  // Enqueued TL2 frame.
  tl.OnEncodeDone(++timestamp, kDefaultBytesPerFrame, false, kDefaultQp,
                  &vp8_info);
  // Enqueued TL0 frame.
  tl.OnEncodeDone(++timestamp, kDefaultBytesPerFrame, false, kDefaultQp,
                  &vp8_info);

  // TL2 frame, all buffers are now in a known good state, OK to reference.
  tl_config = tl.UpdateLayerConfig(++timestamp + 1);
  EXPECT_TRUE(tl_config.last_buffer_flags & BufferFlags::kReference);
  EXPECT_TRUE(tl_config.golden_buffer_flags & BufferFlags::kReference);
  EXPECT_TRUE(tl_config.arf_buffer_flags & BufferFlags::kReference);
}

TEST(TemporalLayersTest, DoesNotReferenceUnlessGuaranteedToExistLongDelay) {
  constexpr int kNumLayers = 3;
  // Use a repeating pattern of tl 0, 2, 1, 2.
  // Tl 0, 1 updates last, golden, altref respectively.
  ScopedFieldTrials field_trial("WebRTC-UseShortVP8TL3Pattern/Enabled/");
  DefaultTemporalLayers tl(kNumLayers);
  DefaultTemporalLayersChecker checker(kNumLayers);
  Vp8EncoderConfig cfg;
  CodecSpecificInfoVP8 vp8_info;
  tl.OnRatesUpdated(GetTemporalLayerRates(kDefaultBytesPerFrame,
                                          kDefaultFramerate, kNumLayers),
                    kDefaultFramerate);
  tl.UpdateConfiguration(&cfg);

  // Start with a keyframe.
  uint32_t timestamp = 0;
  Vp8TemporalLayers::FrameConfig tl_config = tl.UpdateLayerConfig(timestamp);
  tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, true, kDefaultQp,
                  &vp8_info);

  // Do a full cycle of the pattern.
  for (int i = 0; i < 3; ++i) {
    tl_config = tl.UpdateLayerConfig(++timestamp);
    tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, false, kDefaultQp,
                    &vp8_info);
  }

  // TL0 base layer frame, starting the cycle over.
  tl_config = tl.UpdateLayerConfig(++timestamp);
  tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, false, kDefaultQp,
                  &vp8_info);

  // TL2 frame.
  tl_config = tl.UpdateLayerConfig(++timestamp);
  tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, false, kDefaultQp,
                  &vp8_info);

  // Encoder has a hiccup and builds a queue, so frame encoding is delayed.
  // Encoded, but delayed frames in TL 1, 2.
  tl_config = tl.UpdateLayerConfig(timestamp + 1);
  tl_config = tl.UpdateLayerConfig(timestamp + 2);

  // Restart of the pattern!

  // Encoded, but delayed frames in TL 2, 1.
  tl_config = tl.UpdateLayerConfig(timestamp + 3);
  tl_config = tl.UpdateLayerConfig(timestamp + 4);

  // TL1 frame from last cycle is ready.
  tl.OnEncodeDone(timestamp + 1, kDefaultBytesPerFrame, false, kDefaultQp,
                  &vp8_info);
  // TL2 frame from last cycle is ready.
  tl.OnEncodeDone(timestamp + 2, kDefaultBytesPerFrame, false, kDefaultQp,
                  &vp8_info);

  // TL2 frame, that should be referencing all buffers, but altref and golden
  // haven not been updated this cycle. (Don't be fooled by the late frames from
  // the last cycle!)
  tl_config = tl.UpdateLayerConfig(timestamp + 5);
  EXPECT_TRUE(tl_config.last_buffer_flags & BufferFlags::kReference);
  EXPECT_FALSE(tl_config.golden_buffer_flags & BufferFlags::kReference);
  EXPECT_FALSE(tl_config.arf_buffer_flags & BufferFlags::kReference);
}

TEST(TemporalLayersTest, KeyFrame) {
  constexpr int kNumLayers = 3;
  DefaultTemporalLayers tl(kNumLayers);
  DefaultTemporalLayersChecker checker(kNumLayers);
  Vp8EncoderConfig cfg;
  CodecSpecificInfoVP8 vp8_info;
  tl.OnRatesUpdated(GetTemporalLayerRates(kDefaultBytesPerFrame,
                                          kDefaultFramerate, kNumLayers),
                    kDefaultFramerate);
  tl.UpdateConfiguration(&cfg);

  int expected_flags[8] = {
      kTemporalUpdateLastRefAltRef,
      kTemporalUpdateNoneNoRefGolden,
      kTemporalUpdateGoldenWithoutDependencyRefAltRef,
      kTemporalUpdateNone,
      kTemporalUpdateLastRefAltRef,
      kTemporalUpdateNone,
      kTemporalUpdateGoldenRefAltRef,
      kTemporalUpdateNone,
  };
  int expected_temporal_idx[8] = {0, 2, 1, 2, 0, 2, 1, 2};
  bool expected_layer_sync[8] = {true,  true,  true,  false,
                                 false, false, false, false};

  uint32_t timestamp = 0;
  for (int i = 0; i < 7; ++i) {
    // Temporal pattern starts from 0 after key frame. Let the first |i| - 1
    // frames be delta frames, and the |i|th one key frame.
    for (int j = 1; j <= i; ++j) {
      // Since last frame was always a keyframe and thus index 0 in the pattern,
      // this loop starts at index 1.
      Vp8TemporalLayers::FrameConfig tl_config =
          tl.UpdateLayerConfig(timestamp);
      EXPECT_EQ(expected_flags[j], LibvpxVp8Encoder::EncodeFlags(tl_config))
          << j;
      tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, false, kDefaultQp,
                      &vp8_info);
      EXPECT_TRUE(checker.CheckTemporalConfig(false, tl_config));
      EXPECT_EQ(expected_temporal_idx[j], tl_config.packetizer_temporal_idx);
      EXPECT_EQ(expected_temporal_idx[j], tl_config.encoder_layer_id);
      EXPECT_EQ(expected_layer_sync[j], tl_config.layer_sync);
      timestamp += 3000;
    }

    Vp8TemporalLayers::FrameConfig tl_config = tl.UpdateLayerConfig(timestamp);
    tl.OnEncodeDone(timestamp, kDefaultBytesPerFrame, true, kDefaultQp,
                    &vp8_info);
    EXPECT_TRUE(vp8_info.layerSync) << "Key frame should be marked layer sync.";
    EXPECT_EQ(0, vp8_info.temporalIdx)
        << "Key frame should always be packetized as layer 0";
    EXPECT_TRUE(checker.CheckTemporalConfig(true, tl_config));
  }
}

class TemporalLayersReferenceTest : public ::testing::TestWithParam<int> {
 public:
  TemporalLayersReferenceTest()
      : timestamp_(1),
        last_sync_timestamp_(timestamp_),
        tl0_reference_(nullptr) {}
  virtual ~TemporalLayersReferenceTest() {}

 protected:
  static const int kMaxPatternLength = 32;

  struct BufferState {
    BufferState() : BufferState(-1, 0, false) {}
    BufferState(int temporal_idx, uint32_t timestamp, bool sync)
        : temporal_idx(temporal_idx), timestamp(timestamp), sync(sync) {}
    int temporal_idx;
    uint32_t timestamp;
    bool sync;
  };

  bool UpdateSyncRefState(const Vp8TemporalLayers::BufferFlags& flags,
                          BufferState* buffer_state) {
    if (flags & Vp8TemporalLayers::kReference) {
      if (buffer_state->temporal_idx == -1)
        return true;  // References key-frame.
      if (buffer_state->temporal_idx == 0) {
        // No more than one reference to TL0 frame.
        EXPECT_EQ(nullptr, tl0_reference_);
        tl0_reference_ = buffer_state;
        return true;
      }
      return false;  // References higher layer.
    }
    return true;  // No reference, does not affect sync frame status.
  }

  void ValidateReference(const Vp8TemporalLayers::BufferFlags& flags,
                         const BufferState& buffer_state,
                         int temporal_layer) {
    if (flags & Vp8TemporalLayers::kReference) {
      if (temporal_layer > 0 && buffer_state.timestamp > 0) {
        // Check that high layer reference does not go past last sync frame.
        EXPECT_GE(buffer_state.timestamp, last_sync_timestamp_);
      }
      // No reference to buffer in higher layer.
      EXPECT_LE(buffer_state.temporal_idx, temporal_layer);
    }
  }

  uint32_t timestamp_ = 1;
  uint32_t last_sync_timestamp_ = timestamp_;
  BufferState* tl0_reference_;

  BufferState last_state;
  BufferState golden_state;
  BufferState altref_state;
};

INSTANTIATE_TEST_CASE_P(DefaultTemporalLayersTest,
                        TemporalLayersReferenceTest,
                        ::testing::Range(1, kMaxTemporalStreams + 1));

TEST_P(TemporalLayersReferenceTest, ValidFrameConfigs) {
  const int num_layers = GetParam();
  DefaultTemporalLayers tl(num_layers);
  Vp8EncoderConfig cfg;
  CodecSpecificInfoVP8 vp8_specifics;
  tl.OnRatesUpdated(
      GetTemporalLayerRates(kDefaultBytesPerFrame, kDefaultFramerate, 1),
      kDefaultFramerate);
  tl.UpdateConfiguration(&cfg);

  // Run through the pattern and store the frame dependencies, plus keep track
  // of the buffer state; which buffers references which temporal layers (if
  // (any). If a given buffer is never updated, it is legal to reference it
  // even for sync frames. In order to be general, don't assume TL0 always
  // updates |last|.
  std::vector<Vp8TemporalLayers::FrameConfig> tl_configs(kMaxPatternLength);
  for (int i = 0; i < kMaxPatternLength; ++i) {
    Vp8TemporalLayers::FrameConfig tl_config = tl.UpdateLayerConfig(timestamp_);
    tl.OnEncodeDone(timestamp_, kDefaultBytesPerFrame, i == 0, kDefaultQp,
                    &vp8_specifics);
    ++timestamp_;
    EXPECT_FALSE(tl_config.drop_frame);
    tl_configs.push_back(tl_config);
    int temporal_idx = tl_config.encoder_layer_id;
    // For the default layers, always keep encoder and rtp layers in sync.
    EXPECT_EQ(tl_config.packetizer_temporal_idx, temporal_idx);

    // Determine if this frame is in a higher layer but references only TL0
    // or untouched buffers, if so verify it is marked as a layer sync.
    bool is_sync_frame = true;
    tl0_reference_ = nullptr;
    if (temporal_idx <= 0) {
      is_sync_frame = false;  // TL0 by definition not a sync frame.
    } else if (!UpdateSyncRefState(tl_config.last_buffer_flags, &last_state)) {
      is_sync_frame = false;
    } else if (!UpdateSyncRefState(tl_config.golden_buffer_flags,
                                   &golden_state)) {
      is_sync_frame = false;
    } else if (!UpdateSyncRefState(tl_config.arf_buffer_flags, &altref_state)) {
      is_sync_frame = false;
    }
    if (is_sync_frame) {
      // Cache timestamp for last found sync frame, so that we can verify no
      // references back past this frame.
      ASSERT_TRUE(tl0_reference_);
      last_sync_timestamp_ = tl0_reference_->timestamp;
    }
    EXPECT_EQ(tl_config.layer_sync, is_sync_frame);

    // Validate no reference from lower to high temporal layer, or backwards
    // past last reference frame.
    ValidateReference(tl_config.last_buffer_flags, last_state, temporal_idx);
    ValidateReference(tl_config.golden_buffer_flags, golden_state,
                      temporal_idx);
    ValidateReference(tl_config.arf_buffer_flags, altref_state, temporal_idx);

    // Update the current layer state.
    BufferState state = {temporal_idx, timestamp_, is_sync_frame};
    if (tl_config.last_buffer_flags & Vp8TemporalLayers::kUpdate)
      last_state = state;
    if (tl_config.golden_buffer_flags & Vp8TemporalLayers::kUpdate)
      golden_state = state;
    if (tl_config.arf_buffer_flags & Vp8TemporalLayers::kUpdate)
      altref_state = state;
  }
}
}  // namespace test
}  // namespace webrtc
