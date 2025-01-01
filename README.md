# WAV File Generator Documentation

This document explains the implementation of a WAV file generator that creates audio files containing a sine wave tone.

## Overview

The program generates CD-quality WAV files with the following specifications:
- Sample Rate: 44100 Hz
- Bit Depth: 16 bits
- Channels: 1 (Mono)
- Format: PCM (uncompressed)

## Key Components

### 1. WAV Format Namespace

The `wav` namespace contains all WAV file format specifications:
- Constants for audio quality parameters
- A packed struct `Header` that precisely matches the WAV file header format
- Size constants for various chunks of the WAV file

### 2. SineOscillator Class

Generates sine wave samples with the following features:
- Configurable frequency and amplitude
- Phase angle overflow protection
- SIMD optimization hints for x86_64 platforms
- Efficient sample generation using trigonometric functions

Implementation details:
- Uses constant angular velocity based on frequency
- Prevents phase accumulation errors through modulo operation
- Returns normalized samples in the range [-amplitude, +amplitude]

### 3. WavWriter Class

Handles WAV file creation with these optimizations:
- Pre-allocated sample buffer
- Chunked file writing for improved I/O performance
- Proper WAV header generation
- Exception-safe file operations

## Data Flow

1. **Sample Generation**:
    - SineOscillator produces floating-point samples
    - Samples are normalized to [-1.0, 1.0] range
    - Each sample represents amplitude at a point in time

2. **Sample Processing**:
    - Floating-point samples are converted to 16-bit integers
    - Conversion uses the maximum possible amplitude for 16-bit audio
    - Samples are stored in a pre-allocated buffer

3. **File Writing**:
    - WAV header is generated with correct file sizes
    - Header is written first (44 bytes)
    - Audio data is written in 8KB chunks for efficiency
    - File is properly closed and error-checked

## Error Handling

The program includes comprehensive error handling:
- File operation errors
- Memory allocation failures
- Buffer overflow protection
- Exception safety throughout the codebase

## Performance Considerations

1. **Memory Usage**:
    - Pre-allocated buffer prevents reallocations
    - Buffer size based on typical audio durations
    - Efficient memory-to-disk writes

2. **CPU Optimization**:
    - SIMD hints for modern processors
    - Optimized trigonometric calculations
    - Minimal branching in sample generation

3. **I/O Performance**:
    - Chunked writing reduces system calls
    - Binary file operations
    - Proper file buffer sizing

## Usage Example

```cpp
// Create a 440 Hz tone at 50% volume for 2 seconds
SineOscillator oscillator(440.0f, 0.5f);
WavWriter writer;

// Generate samples
for (int i = 0; i < 44100 * 2; ++i) {
    writer.addSample(oscillator.process());
}

// Write to file
writer.writeToFile("output.wav");
```