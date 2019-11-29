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

#include <assert.h>
#include <chrono>
#include <deque>
#include <iostream>
// #include <opencv2/core.hpp>
// #include <opencv2/imgproc/imgproc.hpp>
#include <stdlib.h>
#include <vector>

extern "C" {
#include "frei0r.h"
}

typedef struct delay_map_instance {
  size_t width_ = 8;
  size_t height_ = 8;
  // scale 0 - 255 values to indices into the queue_
  double scale_ = 1.0;
  double offset_ = 0.0;
  bool use_color_ = true;
  bool invert_ = false;
  double queue_length_scale_ = 0.25;
  // std::deque<cv::Mat> queue_;
  std::deque<std::vector<unsigned char>> queue_;

  void update(unsigned char *in, unsigned char *map_raw,
              unsigned char *dst_raw) {
#if 0
    std::cout << width_ << " " << height_
        << " " << (void*)in
        << " " << (void*)map_raw
        << " " << (void*)dst_raw << "\n";
#endif
    if ((width_ == 0) || (height_ == 0)) {
      return;
    }
    // TODO(lucasw) there is something wrong where if the plugin is started
    // with an active map it produces all black.

    // cv::Size sz = cv::Size(width_, height_);
    // cv::Mat image_in = cv::Mat(sz, CV_8UC4); // (void*)in);
    std::vector<unsigned char> image_in;
    queue_.push_back(image_in);
    queue_.back().resize(width_ * height_ * 4);
    // std::copy(in, in + width_ * height_ * 4, &image_in.data[0]);
    std::copy(in, in + width_ * height_ * 4, &queue_.back()[0]);
    size_t max_size = queue_length_scale_ * 480;
    if (max_size < 1) {
      max_size = 1;
    }
    while (queue_.size() > max_size) {
      queue_.pop_front();
    }
    if (false) {
    // if (queue_.size() != max_size) {
      std::cout << "queue size " << queue_.size() << " " << max_size
        << queue_length_scale_ << "\n";
    }

    for (size_t y = 0; y < height_; ++y) {
      for (size_t x = 0; x < width_; ++x) {
        for (size_t i = 0; i < 4; ++i) {
          const size_t ind = y * width_ * 4 + x * 4 + i;
          // TODO(lucasw) try interpolation with a float queue_ind
          int queue_ind = map_raw[ind];
          // scale by queue size rather than max_size
          queue_ind = queue_ind * (queue_.size() * (scale_ / 255.0 + offset_));
          if (queue_ind < 0) {
            queue_ind = 0;
          } else if (queue_ind >= queue_.size()) {
            queue_ind = queue_.size() - 1;
          }
          // TODO(lucasw) need invert toggle, but for now black means the most
          // recent image, white the earliest
          if (!invert_) {
            queue_ind = queue_.size() - 1 - queue_ind;
          }
          dst_raw[ind] = queue_[static_cast<size_t>(queue_ind)][ind];
        }
      }
    }
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
    info->name = "queue length";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "queue length";
    break;
  case 1:
    info->name = "scale";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "scale";
    break;
  case 2:
    info->name = "offset";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "offset";
    break;
  case 3:
    info->name = "invert";
    info->type = F0R_PARAM_BOOL;
    info->explanation = "invert";
    break;
  case 4:
    info->name = "use color";
    info->type = F0R_PARAM_BOOL;
    info->explanation = "use color";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
  delay_map_instance_t *inst = new delay_map_instance;
  inst->width_ = width;
  inst->height_ = height;
  std::cout << "delay map " << width << " x " << height << "\n";
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance) {
  // TODO(lucasw) close the display?
  delay_map_instance_t *inst =
      reinterpret_cast<delay_map_instance_t *>(instance);
  delete inst;
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param,
                         int param_index) {
  assert(instance);
  delay_map_instance_t *inst = (delay_map_instance_t *)instance;

  switch (param_index) {
  case 0: {
    inst->queue_length_scale_ = *(double *)param;
    break;
  }
  case 1: {
    inst->scale_ = *(double *)param;
    break;
  }
  case 2: {
    inst->offset_ = *(double *)param;
    break;
  }
  case 3: {
    inst->invert_ = *(bool *)param;
    break;
  }
  case 4: {
    break;
  }
  }
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param,
                         int param_index) {
  assert(instance);
  delay_map_instance_t *inst = (delay_map_instance_t *)instance;

  switch (param_index) {
  case 0:
    *(f0r_param_double *)param = inst->queue_length_scale_;
    break;
  case 1:
    *(f0r_param_double *)param = inst->scale_;
    break;
  case 2:
    *(f0r_param_double *)param = inst->offset_;
    break;
  case 3:
    *(f0r_param_bool *)param = inst->invert_;
    break;
  }
}

void f0r_update2(f0r_instance_t instance, double time, const uint32_t *inframe1,
                 const uint32_t *inframe2, const uint32_t *inframe3,
                 uint32_t *outframe) {
  assert(instance);
  delay_map_instance_t *inst = (delay_map_instance_t *)instance;
  unsigned char *src = (unsigned char *)inframe1;
  unsigned char *map = (unsigned char *)inframe2;
  unsigned char *dst = (unsigned char *)outframe;

  const clock_t begin_time = clock();
  // this is taking 10-15ms with a 640x480 image
  auto t_start = std::chrono::high_resolution_clock::now();
  inst->update(src, map, dst);
  auto t_end = std::chrono::high_resolution_clock::now();
  double elapsed_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
  std::cout << "update time: " << elapsed_time_ms << "\n";
}
