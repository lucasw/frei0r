/* screencap
 * binarymillenium 2007
 * This file is a Frei0r plugin.
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

#include <X11/Xutil.h>
#include <assert.h>
#include <iostream>
#include <stdlib.h>

extern "C" {
#include "frei0r.h"
}

void XImage2Frei0rImage(XImage &x_image, Display &x_display, Screen &x_screen,
                        unsigned char *dst) {
  XColor color;

  const auto x_depth = x_screen.depths->depth;
  const int wd = x_image.width;
  const int ht = x_image.height;

  // std::cout << "x_image " << wd << " x " << ht << "\n";
  if (x_depth == 24) {
    // the depth is 24, but it still 32 bits with alpha?
    const int frame_size = wd * ht * 4;
    std::copy(x_image.data, x_image.data + frame_size, dst);
#if 0
  } else if (x_depth == 24) {
    for (unsigned int y = 0; y < ht; y++) {
      for (unsigned int x = 0; x < wd; x++) {
        const auto ind_x = (y * wd * 3) + x;
        const auto ind_dst = (y * wd * 4) + x;
        for (unsigned int z = 0; z < 3; ++z) {
          dst[ind_dst + z] = x_image.data[ind_x + z];
        }
        dst[ind_dst + 3] = 255;
      }
    }
#endif
  } else {
    // Extremely slow alternative for non 24/32-bit depth
    std::cerr << "need to implement non-32/24 bit color depth: " << x_depth
              << "\n";
#if 0
    Colormap colmap = DefaultColormap(&x_display, DefaultScreen(&x_display));
    for (unsigned int y = 0; y < x_image.height; y++) {
      for (unsigned int x = 0; x < x_image.width; x++) {
        color.pixel = XGetPixel(&x_image, x, y);
        XQueryColor(&x_display, colmap, &color);
        // cv::Vec4b col = cv::Vec4b(color.blue, color.green, color.red,0);
        // tmp.at<cv::Vec4b> (y,x) = col;
      }
    }
#endif
  }
  return;
}

typedef struct screencap_instance {
  f0r_param_position_t tl_;
  unsigned int x;
  unsigned int y;
  unsigned int width;
  unsigned int height;

  int screen_w_;
  int screen_h_;

  // X resources
  Display *display_;
  Screen *screen_;
  XImage *x_image_sample_;

} screencap_instance_t;

/* Clamps a int32-range int between 0 and 255 inclusive. */
unsigned char CLAMP0255(int32_t a) {
  return (unsigned char)((((-a) >> 31) & a) // 0 if the number was negative
                         | (255 - a) >>
                               31); // -1 if the number was greater than 255
}

int f0r_init() { return 1; }

void f0r_deinit() {}

void f0r_get_plugin_info(f0r_plugin_info_t *inverterInfo) {
  inverterInfo->name = "screencap";
  inverterInfo->author = "binarymillenium";
  inverterInfo->plugin_type = F0R_PLUGIN_TYPE_SOURCE;
  inverterInfo->color_model = F0R_COLOR_MODEL_BGRA8888;
  inverterInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  inverterInfo->major_version = 0;
  inverterInfo->minor_version = 2;
  inverterInfo->num_params = 1;
  inverterInfo->explanation = "grabs an image of the desktop";
}

void f0r_get_param_info(f0r_param_info_t *info, int param_index) {
  switch (param_index) {
  case 0:
    info->name = "xy offset";
    info->type = F0R_PARAM_POSITION;
    info->explanation = "xy offset";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
  screencap_instance_t *inst =
      (screencap_instance_t *)malloc(sizeof(screencap_instance_t));

  std::cout << width << " x " << height << "\n";
  inst->width = width;
  inst->height = height;

  inst->x = 0;
  inst->y = 0;

  inst->display_ = XOpenDisplay(NULL); // Open first (-best) display
  if (inst->display_ == NULL) {
    std::cerr << "bad display\n";
    return nullptr;
  }

  inst->screen_ = DefaultScreenOfDisplay(inst->display_);
  if (inst->screen_ == NULL) {
    std::cerr << "bad screen\n";
    return nullptr;
  }

  Window wid = DefaultRootWindow(inst->display_);
  if (0 > wid) {
    std::cerr << "Failed to obtain the root windows Id "
                 "of the default screen of given display.\n";
    return nullptr;
  }

  XWindowAttributes xwAttr;
  Status ret = XGetWindowAttributes(inst->display_, wid, &xwAttr);
  inst->screen_w_ = xwAttr.width;
  inst->screen_h_ = xwAttr.height;

  std::cout << "full screen w x h " << inst->screen_w_ << " " << inst->screen_h_
            << ", depth " << inst->screen_->depths->depth << "\n";

  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance) {
  // TODO(lucasw) close the display?
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param,
                         int param_index) {
  assert(instance);
  screencap_instance_t *inst = (screencap_instance_t *)instance;

  switch (param_index) {
  case 0:
    {
      inst->tl_ = *((f0r_param_position_t*)param);
      float val;
      val = inst->tl_.x;
      if (val < 0.0) val = 0.0;
      if (val > 1.0) val = 1.0;
      inst->tl_.x = val;
      inst->x =
          (unsigned int)((inst->screen_w_ - inst->width) * val);
      val = inst->tl_.y;
      if (val < 0.0) val = 0.0;
      if (val > 1.0) val = 1.0;
      inst->tl_.y = val;
      inst->y =
          (unsigned int)((inst->screen_h_ - inst->height) * val);
      break;
    }
  }
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param,
                         int param_index) {
  assert(instance);
  screencap_instance_t *inst = (screencap_instance_t *)instance;

  switch (param_index) {
  case 0:
    *(f0r_param_position_t*)param = inst->tl_;
    break;
  }
}

void f0r_update(f0r_instance_t instance, double time, const uint32_t *inframe,
                uint32_t *outframe) {
  assert(instance);
  screencap_instance_t *inst = (screencap_instance_t *)instance;

  unsigned char *dst = (unsigned char *)outframe;

  // grab the image
  inst->x_image_sample_ =
      XGetImage(inst->display_, DefaultRootWindow(inst->display_), inst->x,
                inst->y, inst->width, inst->height, AllPlanes, ZPixmap);

  XImage2Frei0rImage(*inst->x_image_sample_, *inst->display_, *inst->screen_,
                     dst);

  XDestroyImage(inst->x_image_sample_);
#if 0
  for (unsigned i = 0; (i < inst->height); i++) {
    for (unsigned j = 0; (j < inst->width); j++) {

      int ind = i * inst->width + j;

      bool flip = false;

      dst[2] = flip ? 255 - p.Red() : p.Red();
      dst[1] = flip ? 255 - p.Green() : p.Green();
      dst[0] = flip ? 255 - p.Blue() : p.Blue();

      dst += 4;
    }
  }
#endif
}
