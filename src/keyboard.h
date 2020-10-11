#pragma once

#include <algorithm>
#include <iostream>
#include <cstdint>
#include <map>
#include <thread>

#include <SDL2/SDL.h>

struct KeyboardState {
 public:
  KeyboardState() {
    std::fill(octave.begin(), octave.end(), 0);
    FillKeysMap();
  }

  void ProcessEvent(const SDL_Event& event) {
    uint8_t value = 0;
    if (event.type == SDL_KEYDOWN) {
      value = 1;
    }

    auto sym = event.key.keysym.sym;
    if (auto it = key_to_note.find(sym); it != key_to_note.end()) {
      octave[it->second] = value;
      std::cout << "Key: " << int(it->second) << ' ' << int(value) << std::endl;
    }
  }

  std::array<uint8_t, 12> octave;
 private:
  void FillKeysMap() {
    std::vector<int> keys = {SDLK_z, SDLK_s, SDLK_x, SDLK_d, SDLK_c, SDLK_v, SDLK_g, SDLK_b, SDLK_h, SDLK_n, SDLK_j, SDLK_m};
    for (std::size_t i = 0; i < keys.size(); ++i) {
      key_to_note[keys[i]] = i;
    }
  }
  std::map<int, uint8_t> key_to_note;
};
