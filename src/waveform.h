#pragma once

#include <cmath>

class Oscillator {
 public:
  Oscillator(float f, float a) : freq_(f), amp_(a) {}

  void SetFreq(float f) {
    freq_ = f;
  }

  void SetAmp(float a) {
    amp_ = a;
  }

  virtual float Value(float time) = 0;
  float freq_;
  float amp_;
};

class Sine : public Oscillator {
 public:
  Sine(float frequency, float phase, float amplitude)
    : Oscillator(frequency, amplitude)
    , phase_(phase) {
  }

  float Value(float time) {
    return amp_ * sin(time * 2.0 * M_PI * freq_);
  }

 private:
  float phase_;
};

class Square : public Oscillator {
 public:
  Square(float frequency, float amplitude, float duty_cycle)
    : Oscillator(frequency, amplitude)
    , dc_(duty_cycle) {
  }

  float Value(float time) {
    float t = std::fmod(time,  1.0f / freq_) * freq_;
    return amp_ * (t > dc_ ? 1.0f : -1.0f);
  }

 private:
  float dc_;
};
