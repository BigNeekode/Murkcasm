# Microcosm VST

A modular granular delay/looper plugin inspired by Hologram Microcosm. Built with JUCE and C++17.

## Features

### Modular Processing Chain (Helix-Style)
The plugin uses a modular architecture where you can freely add, remove, and reorder processing modules:

```
Input -> [Module 1] -> [Module 2] -> ... -> [Module N] -> Output
```

### Available Modules

**Granular Source:**
- **Granular** - Core granular synthesis engine with scatter, pitch shift, and looping

**Activity Modes:**
- **Chop** - Tempo-synced slicing with stutter options
- **Stretch** - Time stretching without pitch change (overlap-add)
- **Reverse** - Backwards playback with crossfading
- **Scatter** - Random jump cuts through the buffer
- **Glitch** - Stutter/repeat effects with probability

**Effects:**
- **Reverb** - Algorithmic reverb (JUCE DSP)
- **Delay** - Stereo delay with feedback and ping-pong
- **Filter** - Multi-mode filter (LP/HP/BP)
- **Chorus** - Stereo chorus effect
- **Bitcrusher** - Sample rate and bit depth reduction
- **EQ** - 4-band parametric EQ with sweepable frequencies and Q

## Building

### Requirements
- CMake 3.16+
- C++17 compiler
- JUCE 8.0+ (fetched automatically)

### Build Commands

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

The VST3 will be in `build/Microcosm_artefacts/Release/VST3/`

## Architecture

```
Source/
├── PluginProcessor.h/cpp      # Main plugin host
├── PluginEditor.h/cpp         # UI with tabbed interface
├── ProcessingModule.h/cpp     # Base class and chain management
├── GranularEngine.h/cpp       # Granular synthesis source
├── CircularBuffer.h/cpp       # Loop recording buffer
├── ActivityModules.h/cpp      # All activity modes (Chop, Stretch, etc)
├── EffectModules.h/cpp        # All effect modules (Reverb, Delay, etc)
├── EQModule.h/cpp             # 4-band parametric EQ
├── ModulationSystem.h/cpp     # LFOs, env followers, random generators
├── ModulationMatrix.h/cpp     # Routing matrix connecting sources to targets
└── ModulationUI.h/cpp         # Visual modulation editor
```

## Module Parameters

### Granular
| Parameter | Range | Description |
|-----------|-------|-------------|
| Grain Size | 10-500ms | Length of each grain |
| Density | 1-50 | Overlapping grains per block |
| Scatter | 0-100% | Randomness of start position |
| Pitch | ±12st | Playback pitch shift |
| Loop Length | 0.1-60s | Recording buffer size |
| Loop | On/Off | Enable recording |
| Overdub | On/Off | Mix new audio with loop |
| Feedback | 0-100% | Loop decay amount |

### Activity Modes
Each mode has unique parameters. See source code for details.

## Roadmap

- [x] Phase 1: Granular engine scaffold
- [x] Phase 2: Modular architecture with Activity Modes
- [x] Phase 3: Parameter modulation system
- [ ] Phase 4: Preset system
- [ ] Phase 5: UI polish and visualization

### Phase 3: Modulation System

Microcosm now features a comprehensive modulation matrix allowing any parameter to be dynamically controlled:

**Modulation Sources:**
| Source | Description | Use Case |
|--------|-------------|----------|
| LFO Sine | Smooth periodic oscillation | Pulsing grain density |
| LFO Triangle | Linear ramp up/down | Sawtooth filter sweeps |
| LFO Random | Smooth random values | Organic, evolving textures |
| Envelope Follower | Audio-reactive modulation | Ducking reverb with transients |
| Random Step | Stepped random values | Glitchy stuttering |
| Perlin Noise | Organic natural patterns | Fluid, liquid modulation |

**Modulation Matrix:**
```
┌─────────────────────────────────────────┐
│  SOURCE    AMOUNT    →    TARGET       │
├─────────────────────────────────────────┤
│  LFO Sine   30%     →    Scatter       │
│  EnvFollow  50%     →    Filter Cutoff │
│  Perlin     20%     →    Delay Time    │
└─────────────────────────────────────────┘
```

Access via the "Modulation" tab in the plugin UI. Add sources, create routings, and adjust modulation amounts in real-time.

## License

MIT License - See LICENSE file
