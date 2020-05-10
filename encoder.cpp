#include "encoder.h"

namespace encoder {

#define RAD_PER_REV      (6.2831853f)
#define START            ((uint8_t)0)
#define END              ((uint8_t)1)

Encoder::Encoder(PinName pin, PinMode mode, int pulses_per_rev, \
                 uint8_t window_size):interrupt_in_(pin, mode) {
  period_.meas = new int[window_size];
  window_size_ = window_size;
  pulses_per_rev_ = pulses_per_rev;
  period_.meas_type = START;
  last_speed_ = 0;
  t_.start();
  interrupt_in_.rise(Callback<void()>(this, &Encoder::MeasurePeriod));
  interrupt_in_.enable_irq();
}

Encoder::~Encoder(void) {
  delete [] period_.meas;
}

void Encoder::MeasurePeriod(void) {
  if (START == period_.meas_type) {
    period_.start = t_.read_us();
    period_.meas_type = END;
  } else {
    period_.meas[period_.num_meas] = t_.read_us() - period_.start;
    period_.num_meas = period_.num_meas < (window_size_ - 1) ? \
                       period_.num_meas + 1 : 0;
    period_.meas_type = START;
  }
}

void Encoder::ResetPeriodMeas(void) {
  period_.meas_type = START;
  period_.num_meas = 0;
}

float Encoder::GetWheelSpeed(void) {
  float period_sum, period_avg; 
  float speed = 0;

  interrupt_in_.disable_irq();

  /* Compute the average period since last call to ResetPeriodMeas */
  for (uint8_t i=0; i < MIN(period_.num_meas, window_size_); i++) {
    period_sum += period_.meas[i];
  }

  if (0 != period_.num_meas) {  //  Protect against divide by 0
    period_avg = period_sum/period_.num_meas;
    speed = RAD_PER_REV/(pulses_per_rev_*period_avg*1e-6);
  }

  ResetPeriodMeas();

  interrupt_in_.enable_irq();

  return(speed);
}

void Encoder::ZeroWheelSpeed(void) {
  interrupt_in_.disable_irq();
  ResetPeriodMeas();
  interrupt_in_.enable_irq();
}

}   // namespace encoder

