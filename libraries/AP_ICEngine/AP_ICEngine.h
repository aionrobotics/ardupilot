/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
  control of internal combustion engines (starter, ignition and choke)
 */
#pragma once

#include <AP_HAL/AP_HAL.h>
#include <AP_RPM/AP_RPM.h>

#define AP_ICENGINE_OPTIONS_MASK_ARMING_REQUIRED_IGNITION       (1<<0)
#define AP_ICENGINE_OPTIONS_MASK_ARMING_REQUIRED_START          (1<<1)
#define AP_ICENGINE_OPTIONS_MASK_DEFAULT                        (0)


class AP_ICEngine {
public:
    // constructor
    AP_ICEngine();

    /* Do not allow copies */
    AP_ICEngine(const AP_ICEngine &other) = delete;
    AP_ICEngine &operator=(const AP_ICEngine&) = delete;

    static const struct AP_Param::GroupInfo var_info[];

    // update engine state. Should be called at 10Hz or more
    void update(void);

    void init(const bool force_outut);

    // check for throttle override
    bool throttle_override(int8_t &percent);

    // check for brake override
    bool brake_override(float &percentage);

    enum ICE_State {
        ICE_OFF=0,
        ICE_START_HEIGHT_DELAY=1,
        ICE_START_DELAY=2,
        ICE_STARTING=3,
        ICE_RUNNING=4
    };

    // get current engine control state
    ICE_State get_state(void) const { return state; }

    // handle DO_ENGINE_CONTROL messages via MAVLink or mission
    bool engine_control(float start_control, float cold_start, float height_delay);
    
    bool handle_message(const mavlink_command_long_t &packt);
    bool handle_set_ice_transmission_state(const mavlink_command_long_t &packet);

    // Engine temperature status
    bool get_temperature(float& value) const;
    bool too_hot() const { return temperature.is_healthy() && temperature.too_hot(); }
    bool too_cold() const { return temperature.is_healthy() && temperature.too_cold(); }
    void send_temp();

    static AP_ICEngine *get_singleton() { return _singleton; }

private:
    static AP_ICEngine *_singleton;

    enum ICE_State state;

    enum MAV_ICE_TRANSMISSION_GEAR_STATE gear_state = MAV_ICE_TRANSMISSION_GEAR_STATE_UNKNOWN;
    uint16_t gear_pwm;

    // engine temperature for feedback
    struct {
        AP_Int8 pin;
        int8_t pin_prev; // check for changes at runtime
        AP_Float scaler;
        AP_Int16 min;
        AP_Int16 max;
        AP_Int8 ratiometric;
        AP_Float offset;
        AP_Int8 function;
        AP_Float too_hot_throttle_reduction_factor;

        AP_HAL::AnalogSource *source;
        float value;
        uint32_t last_sample_ms;
        uint32_t last_send_ms;

        bool is_healthy() const { return (pin > 0 && last_sample_ms && (AP_HAL::millis() - last_sample_ms < 1000)); }
        bool too_hot() const {  return (min < max) && (value > max); } // note, min == max will return false.
        bool too_cold() const { return (min < max) && (value < min); } // note, min == max will return false.
    } temperature;

    enum Temperature_Function {
        FUNCTION_LINEAR    = 0,
        FUNCTION_INVERTED  = 1,
        FUNCTION_HYPERBOLA = 2
    };

    void update_temperature();

    void set_output_channels();

    void determine_state();

    void set_outputs_off() const;

    // bitmask options
    AP_Int32 options;

    AP_Int8 resarts_allowed;

    // enable library
    AP_Int8 enable;

    // channel for pilot to command engine start, 0 for none
    AP_Int8 start_chan;

    // which RPM instance to use
    AP_Int8 rpm_instance;
    
    // time to run starter for (seconds)
    AP_Float starter_time;

    // delay between start attempts (seconds)
    AP_Float starter_delay;
    
    // RPM above which engine is considered to be running
    AP_Int32 rpm_threshold_running;
    
    // RPM above which engine is considered to be running and remaining starting time should be skipped
    AP_Int32 rpm_threshold_starting;

    // time when we started the starter
    uint32_t starter_start_time_ms;

    // time when we last ran the starter
    uint32_t starter_last_run_ms;

    // throttle percentage for engine start
    AP_Int8 start_percent;

    // throttle percentage for engine idle
    AP_Int8 idle_percent;

    // Time to wait after applying acceessory before applying starter
    AP_Int16 power_up_time;
    uint32_t engine_power_up_wait_ms;

    // height when we enter ICE_START_HEIGHT_DELAY
    float initial_height;

    // height change required to start engine
    float height_required;

    // we are waiting for valid height data
    bool height_pending:1;

    // timestamp for periodic gcs msg regarding throttle_override
    uint32_t throttle_overrde_msg_last_ms;

    // store the previous throttle
    int8_t throttle_prev;

    // keep track of how many times we attempted to start. This will get conmpared to resarts_allowed
    uint8_t starting_attempts;

    // to know if we're running for the first time
    bool run_once;

    AP_Int8 master_output_enable_pin;
};


namespace AP {
    AP_ICEngine *ice();
};
