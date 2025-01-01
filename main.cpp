#include <cmath>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <array>
#include <cstdint>
#include <iostream>

// WAV format specifications and constants
namespace wav {
    constexpr uint32_t SAMPLE_RATE = 44100;    // CD-quality audio sample rate
    constexpr uint16_t BIT_DEPTH = 16;         // Bits per sample
    constexpr uint16_t NUM_CHANNELS = 1;       // Mono audio
    constexpr uint16_t AUDIO_FORMAT = 1;       // PCM format code

    // Standard WAV header sizes in bytes
    constexpr uint32_t FMT_CHUNK_SIZE = 16;    // Format chunk size
    constexpr uint32_t HEADER_SIZE = 44;       // Total header size

    // WAV file header structure following standard RIFF specification
    #pragma pack(push, 1)  // Ensure struct is packed without padding
    struct Header {
        // RIFF chunk descriptor
        std::array<char, 4> riffHeader;        // Contains "RIFF"
        uint32_t chunkSize;                    // Total file size - 8 bytes
        std::array<char, 4> waveHeader;        // Contains "WAVE"

        // Format sub-chunk
        std::array<char, 4> fmtHeader;         // Contains "fmt "
        uint32_t subchunk1Size;                // Format chunk size (16 for PCM)
        uint16_t audioFormat;                  // Audio format code (1 for PCM)
        uint16_t numChannels;                  // Mono = 1, Stereo = 2
        uint32_t sampleRate;                   // Samples per second
        uint32_t byteRate;                     // Bytes per second
        uint16_t blockAlign;                   // Bytes per sample * channels
        uint16_t bitsPerSample;                // Bits per sample

        // Data sub-chunk
        std::array<char, 4> dataHeader;        // Contains "data"
        uint32_t dataSize;                     // Size of actual audio data
    };
    #pragma pack(pop)
}

/**
 * @brief Generates sine wave samples for audio synthesis
 *
 * Implements a simple sine wave oscillator with frequency and amplitude control.
 * Uses optimized floating-point operations when available.
 */
class SineOscillator {
    const float frequency;                     // Oscillator frequency in Hz
    const float amplitude;                     // Output amplitude (0.0 to 1.0)
    float angle = 0.0f;                       // Current phase angle
    const float angleStep;                     // Phase increment per sample

    static constexpr float TWO_PI = 2.0f * static_cast<float>(M_PI);

public:
    /**
     * @param freq Frequency in Hz
     * @param amp Amplitude (0.0 to 1.0)
     */
    SineOscillator(float freq, float amp) :
        frequency(freq),
        amplitude(amp),
        angleStep(TWO_PI * freq / wav::SAMPLE_RATE) {}

    // Enable SIMD optimizations for x86_64 platforms
    #if defined(__x86_64__) || defined(_M_X64)
    __attribute__((optimize("O3")))
    #endif
    float process() noexcept {
        float sample = amplitude * std::sin(angle);
        angle += angleStep;
        if (angle >= TWO_PI) angle -= TWO_PI;  // Prevent overflow
        return sample;
    }
};

/**
 * @brief Handles WAV file creation and audio data writing
 *
 * Manages audio sample buffer and implements efficient file I/O
 * with proper WAV header generation.
 */
class WavWriter {
    std::vector<int16_t> buffer;              // Audio sample buffer
    const double maxAmplitude;                 // Maximum sample value

public:
    WavWriter() : maxAmplitude(std::pow(2.0, wav::BIT_DEPTH - 1) - 1) {
        buffer.reserve(wav::SAMPLE_RATE * 5);  // Pre-allocate 5 seconds
    }

    /**
     * @brief Converts and stores a floating-point audio sample
     * @param sample Float sample in range [-1.0, 1.0]
     */
    void addSample(float sample) {
        buffer.push_back(static_cast<int16_t>(sample * maxAmplitude));
    }

    /**
     * @brief Writes audio data to WAV file
     * @param filename Output file path
     * @throws std::runtime_error if file operations fail
     */
    void writeToFile(const std::string& filename) const {
        std::ofstream file(filename, std::ios::binary);
        if (!file) throw std::runtime_error("Could not open file: " + filename);

        // Create and initialize WAV header
        wav::Header header;
        header.dataSize = buffer.size() * sizeof(int16_t);
        header.chunkSize = wav::HEADER_SIZE + header.dataSize - 8;

        file.write(reinterpret_cast<const char*>(&header), sizeof(header));

        // Write audio data in optimized chunks
        const char* audioData = reinterpret_cast<const char*>(buffer.data());
        size_t remaining = header.dataSize;

        while (remaining > 0) {
            constexpr size_t CHUNK_SIZE = 8192;
            size_t writeSize = std::min(remaining, CHUNK_SIZE);
            file.write(audioData, writeSize);
            audioData += writeSize;
            remaining -= writeSize;
        }

        file.close();
    }
};

int main() {
    try {
        // Audio generation parameters
        constexpr int DURATION_SECONDS = 2;
        constexpr float FREQUENCY = 440.0f;    // A4 note
        constexpr float AMPLITUDE = 0.5f;      // 50% volume

        SineOscillator oscillator(FREQUENCY, AMPLITUDE);
        WavWriter writer;

        // Generate and store audio samples
        for (int i = 0; i < wav::SAMPLE_RATE * DURATION_SECONDS; ++i) {
            writer.addSample(oscillator.process());
        }

        writer.writeToFile("audio.wav");
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}