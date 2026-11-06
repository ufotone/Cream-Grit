#include "unit.h"
#include "cream_unit.h"

static Delay s_delay_instance;
static unit_runtime_desc_t s_runtime_desc;

__unit_callback int8_t unit_init(const unit_runtime_desc_t * desc) {
  if (!desc)
    return k_unit_err_undef;

  if (desc->target != unit_header.target)
    return k_unit_err_target;

  if (!UNIT_API_IS_COMPAT(desc->api))
    return k_unit_err_api_version;

  s_runtime_desc = *desc;

  return s_delay_instance.Init(desc);
}

__unit_callback void unit_teardown() {
  s_delay_instance.Teardown();
}

__unit_callback void unit_reset() {
  s_delay_instance.Reset();
}

__unit_callback void unit_resume() {
  s_delay_instance.Resume();
}

__unit_callback void unit_suspend() {
  s_delay_instance.Suspend();
}

__unit_callback void unit_render(const float * in, float * out, uint32_t frames) {
  s_delay_instance.Process(in, out, frames);
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value) {
  s_delay_instance.setParameter(id, value);
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
  return s_delay_instance.getParameterValue(id);
}

__unit_callback const char * unit_get_param_str_value(uint8_t id, int32_t value) {
  return s_delay_instance.getParameterStrValue(id, value);
}

__unit_callback const uint8_t * unit_get_param_bmp_value(uint8_t id, int32_t value) {
  return s_delay_instance.getParameterBmpValue(id, value);
}

__unit_callback void unit_set_tempo(uint32_t tempo) {
  s_delay_instance.SetTempo(tempo);
}

__unit_callback void unit_load_preset(uint8_t idx) {
  s_delay_instance.LoadPreset(idx);
}

__unit_callback uint8_t unit_get_preset_index() {
  return s_delay_instance.getPresetIndex();
}

__unit_callback const char * unit_get_preset_name(uint8_t idx) {
  return Delay::getPresetName(idx);
}
