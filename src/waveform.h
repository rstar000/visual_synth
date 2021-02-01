#pragma once

#include <cmath>
#include "node.h"
#include "output.h"

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

  void SetDutyCycle(float dc) {
    dc_ = dc;
  }

 private:
  float dc_;
};


class OscillatorNode : public Node {
 public:
  OscillatorNode() {
    auto freq = std::make_shared<Input>("freq", Dt::kFloat, this, 440.0f);
    auto amp = std::make_shared<Input>("amp", Dt::kFloat, this, 0.5f);
    auto signal = std::make_shared<Output>("signal", Dt::kFloat, this, 0.0f);

    inputs = {freq, amp};
    outputs = {signal};
  }
};

class SineOscillatorNode : public OscillatorNode {
 public:
  SineOscillatorNode() 
    : OscillatorNode() 
    , osc_(std::make_shared<Sine>(440.0f, 0.0, 0.2))
    { }

 protected:
  void Process(float time) override {
    osc_->SetFreq(inputs[0]->GetValue<float>());
    osc_->SetAmp(inputs[1]->GetValue<float>());
    outputs[0]->SetValue<float>(osc_->Value(time));
  }
  std::shared_ptr<Sine> osc_;
};
