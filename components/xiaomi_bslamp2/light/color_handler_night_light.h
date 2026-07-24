#pragma once

#include "../common.h"
#include "../light_hal.h"
#include "color_handler.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace xiaomi {
namespace bslamp2 {

struct NightLightCalibration {
  float red;
  float green;
  float blue;
};

static const NightLightCalibration DEFAULT_NIGHT_LIGHT_CALIBRATION{0.968f, 0.968f, 0.972f};

/**
 * This class can handle the GPIO outputs for the night light mode.
 *
 * At the lowest brightness setting, the light will switch to night light
 * mode. In the Yeelight integration in Home Assistant, this feature is
 * exposed trough a separate switch. I have found that the switch is both
 * confusing and made me run into issues when automating the lights.
 * Using the lowest brightness for triggering the night light feels a lot
 * more natural.
 *
 * Note that if a switch is still the preferred way to handle the night
 * light mode toggle, then this still could be implemented through the
 * device's yaml configuration.
 */
class ColorHandlerNightLight : public ColorHandler {
 public:
  void set_color_temperature_calibration(NightLightCalibration calibration) {
    color_temperature_calibration_.red = clamp(calibration.red, 0.0f, 1.0f);
    color_temperature_calibration_.green = clamp(calibration.green, 0.0f, 1.0f);
    color_temperature_calibration_.blue = clamp(calibration.blue, 0.0f, 1.0f);
  }

  void set_color_temperature_calibration(float red, float green, float blue) {
    set_color_temperature_calibration({red, green, blue});
  }

  /**
   * Sets a brightness multiplier for the RGB night light. The multiplier is
   * applied in "drive space" (1.0 - duty), where a larger drive means a
   * brighter channel. A value of 1.0 keeps the original (measured) brightness,
   * values below 1.0 make the RGB night light dimmer and values above 1.0
   * make it brighter. This allows dimming the RGB night light well below the
   * default floor, e.g. for a very faint red glow in a nursery.
   */
  void set_rgb_brightness(float brightness) {
    rgb_brightness_ = clamp(brightness, 0.0f, 4.0f);
  }

  bool set_light_color_values(light::LightColorValues v) {
    light_mode = LIGHT_MODE_NIGHT;

    // Note: I do not check for a brightness at or below 0.01 (1%) here,
    // because the lowest brightness setting from Home Assistant turns
    // up as 0.011765 in here (which is 3/255 and not 1/100).
    if (v.get_brightness() >= 0.012f)
      return false;

    // This night light mode is activated when white light is selected.
    // Based on measurements using the original device firmware, so it
    // matches the night light of the original firmware.
    if (v.get_color_mode() == light::ColorMode::COLOR_TEMPERATURE) {
      red = color_temperature_calibration_.red;
      green = color_temperature_calibration_.green;
      blue = color_temperature_calibration_.blue;
      white = 0.0f;
    }
    // In RGB mode, the selected color is used to give the night light a
    // specific color, instead of the default. This is a nice extra for
    // this firmware, as the original firmware does not support it.
    else {
      // Compute the measured duty cycles, then apply the RGB brightness
      // multiplier in drive space (1.0 - duty). Scaling the drive keeps the
      // color balance intact while making the glow dimmer or brighter. The
      // result is clamped to a valid duty cycle range.
      const float m = rgb_brightness_;
      auto blue_scale = (v.get_red() + v.get_green()) / 2.0f;
      auto blue_max = std::lerp(0.9640f, 0.9720f, blue_scale);
      red = clamp(1.0f - (1.0f - std::lerp(0.9997f, 0.9680f, v.get_red())) * m, 0.0f, 1.0f);
      green = clamp(1.0f - (1.0f - std::lerp(0.9997f, 0.9680f, v.get_green())) * m, 0.0f, 1.0f);
      blue = clamp(1.0f - (1.0f - std::lerp(0.9997f, blue_max, v.get_blue())) * m, 0.0f, 1.0f);
      white = 0.0f;
    }

    return true;
  }

 protected:
  NightLightCalibration color_temperature_calibration_{DEFAULT_NIGHT_LIGHT_CALIBRATION};
  float rgb_brightness_{1.0f};
};

}  // namespace bslamp2
}  // namespace xiaomi
}  // namespace esphome
