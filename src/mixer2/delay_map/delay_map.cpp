/* delay_map
 * Lucas Walter 2019
 * This file is a Frei0r plugin.
 * Use an input image and a scale factor to index into a deque of buffered
 * images.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <algorithm>
#include <assert.h>
#include <chrono>
#include <deque>
#include <iostream>
// #include <opencv2/core.hpp>
// #include <opencv2/imgproc/imgproc.hpp>
#include <stdlib.h>
#include <vector>

// #define LOG(msg) std::cout << __FUNCTION__ << ":" << __LINE__ << " " << msg << "\n"
#define LOG(msg) do {} while(0)
extern "C" {
#include "frei0r.h"
}

typedef struct delay_map_instance {
  const size_t width_ = 8;
  const size_t height_ = 8;
  const size_t num_chan_ = 4;
  const size_t max_size_ = 180;
  // scale 0 - 255 values to indices into the queue_
  double scale_ = 1.0;
  double offset_ = 0.0;
  bool use_color_ = true;
  bool invert_ = false;
  // std::deque<std::vector<unsigned char>> queue_;
  std::vector<unsigned char> queue_;
  size_t queue_start_ = 0;

  delay_map_instance(size_t width, size_t height) :
    width_(width),
    height_(height)
  {
    queue_.resize(width_ * height_ * num_chan_ * max_size_);
    std::cout << __FUNCTION__ << __LINE__ << " delay map "
        << width_ << " x " << height_ << " " << max_size_ << " " << num_chan_
        << " " << queue_.size()
        << " " << this << "\n";
  }

  void update(unsigned char *in, unsigned char *map_raw,
              unsigned char *dst_raw) {
#if 0
    LOG(width_ << " " << height_
        << " " << (void*)in
        << " " << (void*)map_raw
        << " " << (void*)dst_raw << std::endl);
#endif
    if ((width_ == 0) || (height_ == 0)) {
      return;
    }
    if ((in == nullptr) || (map_raw == nullptr) || (dst_raw == nullptr)) {
      return;
    }
    // TODO(lucasw) there is something wrong where if the plugin is started
    // with an active map it produces all black.

    LOG(queue_start_ << " " << scale_ << " " << offset_ << " " << invert_);
    const size_t im_sz = width_ * height_ * 4;
    const size_t im_ind = queue_start_ * im_sz;
    std::copy(in, in + im_sz, &queue_[im_ind]);

    // TODO(lucasw) probably should round here and in the main loop, but what performance cost?
    if (false) {
    // if (scale_ <= 0.0) {
      std::copy(in, in + im_sz, dst_raw);
      return;
    }

    const int invert_scale = (invert_ >= 0.5) ? 1 : -1;
    LOG(max_size_ << " " << invert_scale);

    for (size_t y = 0; y < height_; ++y) {
      for (size_t x = 0; x < width_; ++x) {
        const size_t pix_ind = y * width_ * num_chan_ + x * num_chan_;
        for (size_t i = 0; i < 4; ++i) {
          const size_t pix_chan_ind = pix_ind + i;
          // TODO(lucasw) try interpolation with a float src_im_ind
          // black means the most recent image, white the earliest
          const int src_im_ind_raw = map_raw[pix_chan_ind];
          const int src_im_ind_offset = invert_scale * (max_size_ *
              (scale_ * src_im_ind_raw / 255.0 + offset_));
          const int src_im_ind_pre = (static_cast<int>(queue_start_) + src_im_ind_offset);
          int max_ssize = static_cast<int>(max_size_);
          const int src_im_ind =  (max_ssize + src_im_ind_pre) % max_ssize;
          const size_t combined_ind = src_im_ind * im_sz + pix_chan_ind;

#if 0
          if ((y == 0) && (x == 0) && (i == 0)) {
            std::cout << queue_start_ << " " << combined_ind
              << ", src_im_ind_pre " << src_im_ind_pre
              << ", src_im_ind " << src_im_ind
              << ", src_im_ind_offset " << src_im_ind_offset
              << ", " << src_im_ind_raw << " " << scale_
              << " " << offset_ << " " << invert_scale << "\n";
          }
#endif
#if 1
          // if (combined_ind >= queue_.size()) {
          if (combined_ind >= queue_.size()) {
            std::cerr << "x " << x << ", y " << y
                << ", queue_start_ " << queue_start_
                << ", src_im_ind_raw " << src_im_ind_raw
                << ", src_im_ind " << src_im_ind
                << ", im_sz " << im_sz
                << ", pix_chan_ind " << pix_chan_ind
                << ", combined_ind " << combined_ind
                << ", queue size " << queue_.size() << "\n";
            return;
          }
#endif
          dst_raw[pix_chan_ind] = queue_[combined_ind];
        }
      }
    }
    queue_start_++;
    queue_start_ %= max_size_;
  }
} delay_map_instance_t;

int f0r_init() { return 1; }

void f0r_deinit() {}

void f0r_get_plugin_info(f0r_plugin_info_t *inverterInfo) {
  inverterInfo->name = "delay_map";
  inverterInfo->author = "Lucas Walter";
  inverterInfo->plugin_type = F0R_PLUGIN_TYPE_MIXER2;
  inverterInfo->color_model = F0R_COLOR_MODEL_BGRA8888;
  inverterInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  inverterInfo->major_version = 0;
  inverterInfo->minor_version = 2;
  inverterInfo->num_params = 3;
  inverterInfo->explanation =
      "Uses an image input as a per pixel map of delay times in output";
}

void f0r_get_param_info(f0r_param_info_t *info, int param_index) {
  switch (param_index) {
  case 0:
    info->name = "scale";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "scale";
    break;
  case 1:
    info->name = "offset";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "offset";
    break;
  case 2:
    info->name = "invert";
    info->type = F0R_PARAM_BOOL;
    info->explanation = "invert";
    break;
  case 3:
    info->name = "use color";
    info->type = F0R_PARAM_BOOL;
    info->explanation = "use color";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
  LOG("construct");
  delay_map_instance_t *inst = new delay_map_instance(width, height);
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance) {
  // TODO(lucasw) close the display?
  LOG("destruct");
  delay_map_instance_t *inst =
      reinterpret_cast<delay_map_instance_t *>(instance);
  delete inst;
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param,
                         int param_index) {
  assert(instance);
  delay_map_instance_t *inst = (delay_map_instance_t *)instance;
  LOG(inst << " " << param << " " << param_index);

  switch (param_index) {
  case 0: {
    inst->scale_ = *(double *)param;
    std::clamp(inst->scale_, 0.0, 1.0);
    break;
  }
  case 1: {
    inst->offset_ = *(double *)param;
    std::clamp(inst->offset_, 0.0, 1.0);
    break;
  }
  case 2: {
    inst->invert_ = *(bool *)param;
    break;
  }
  }
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param,
                         int param_index) {
  assert(instance);
  delay_map_instance_t *inst = (delay_map_instance_t *)instance;
  LOG(inst << " " << param << " " << param_index);

  switch (param_index) {
  case 0:
    *(f0r_param_double *)param = inst->scale_;
    break;
  case 1:
    *(f0r_param_double *)param = inst->offset_;
    break;
  case 2:
    *(f0r_param_bool *)param = inst->invert_;
    break;
  }
}

void f0r_update2(f0r_instance_t instance, double time, const uint32_t *inframe1,
                 const uint32_t *inframe2, const uint32_t *inframe3,
                 uint32_t *outframe) {
  assert(instance);
  delay_map_instance_t *inst = (delay_map_instance_t *)instance;
  LOG(inst << " " << inframe1 << " " << inframe2);
  unsigned char *src = (unsigned char *)inframe1;
  unsigned char *map = (unsigned char *)inframe2;
  unsigned char *dst = (unsigned char *)outframe;

  const clock_t begin_time = clock();
  // this is taking 10-15ms with a 640x480 image
  auto t_start = std::chrono::high_resolution_clock::now();
  inst->update(src, map, dst);
  auto t_end = std::chrono::high_resolution_clock::now();
  double elapsed_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
  LOG("update time ms: " << elapsed_time_ms);
}
