#pragma once

#include <cmath>

class Oscillator {
 public:
  Oscillator() = default;

  virtual float Value(float time) = 0;
};

class Sine : public Oscillator {
 public:
  Sine(float frequency, float phase, float amplitude)
    : freq_(frequency)
    , phase_(phase)
    , amp_(amplitude) {
  }

  float Value(float time) {
    return amp_ * sin(time * 2.0 * M_PI * freq_);
  }

 private:
  float freq_;
  float phase_;
  float amp_;
};

class Square : public Oscillator {
 public:
  Square(float frequency, float amplitude, float duty_cycle)
    : freq_(frequency)
    , amp_(amplitude)
    , ds_(duty_cycle) {
  }

  float Value(float time) {
    return amp_ * (sin(time * 2.0 * M_PI * freq_) > 0 ? 1.0 : -1.0);
  }

 private:
  float freq_;
  float amp_;
  float ds_;
};
