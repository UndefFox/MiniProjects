#include "particleengine.h"

#include <cstdlib>
#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <immintrin.h>

#include "avx2prng.h"


namespace {
    constexpr auto CORDS_PER_LINE = sizeof(__m256i) / sizeof(ParticleEngine::cords_t);
    constexpr auto CHARS_PER_LINE = sizeof(__m128i) / sizeof(char);
}

void ParticleEngine::addArchetype(unsigned char symbol, unsigned int count, int speed) {
    if (symbol > 127)
        throw std::invalid_argument("Extended ASCII isn't supported!");

    archetypes.emplace_back(symbol, speed, count);
}

void ParticleEngine::resizeImage(unsigned int width, unsigned int height) {
    this->width = width;
    this->height = height;

    AVX2_PRNG_Generator gen{};

    for (auto& arche : archetypes) {
        for (int i = 0; i < arche.particles.x.size(); i += CORDS_PER_LINE) {
            _mm256_store_si256((__m256i*) &arche.particles.x[i], gen.newRanged16(width));
        }
        for (int i = 0; i < arche.particles.y.size(); i += CORDS_PER_LINE) {
            _mm256_store_si256((__m256i*) &arche.particles.y[i], gen.newRanged16(height));
        }
    }

    // -1 to avoid autoscrolling by going to the next line
    const auto buffer_size = width * height;
    buffer.reserve((buffer_size / CHARS_PER_LINE + 1) * CHARS_PER_LINE);
    buffer.resize(buffer_size);
    std::memset(buffer.data(), ' ', buffer.size());
}

void ParticleEngine::waitForFrameDelay() const {
    std::this_thread::sleep_for(std::chrono::milliseconds{1000 / fps});
}

void ParticleEngine::setFPS(unsigned int newFPS) {
    if (newFPS == 0)
        throw std::invalid_argument("fps can't be 0!");

    fps = newFPS;
}

void ParticleEngine::tick() {
    __m256i heightV = _mm256_set1_epi16(height);
    __m256i widthV = _mm256_set1_epi32(width);
    AVX2_PRNG_Generator gen{};

    std::memset(buffer.data(), ' ', buffer.size());

    for (auto& arche : archetypes) {
        const __m256i speedV = _mm256_set1_epi16(arche.speed);

        cords_t* x = arche.particles.x.data();
        cords_t* y = arche.particles.y.data();

        bool flag = true;
        const auto particle_count = arche.particles.size();
        for (int i = 0; flag ;) {
            __m256i xV = _mm256_load_si256((const __m256i*) x);
            __m256i yV = _mm256_load_si256((const __m256i*) y);

            yV = _mm256_add_epi16(yV, speedV);

            const __m256i mask = _mm256_cmpgt_epi16(heightV, yV);
            const __m256i not_mask = _mm256_andnot_si256(mask, _mm256_set1_epi16(-1));

            if (!_mm256_testz_si256(not_mask, not_mask)) {
                yV = _mm256_and_si256(yV, mask);

                const __m256i newXV = gen.newRanged16(width);

                xV = _mm256_or_si256(
                    _mm256_and_si256(not_mask, newXV),
                    _mm256_and_si256(mask, xV)
                );

                _mm256_store_si256((__m256i*) x, xV);
            }

            _mm256_store_si256((__m256i*) y, yV);

            alignas(32) uint32_t cords[16];


            __m256i yH = _mm256_cvtepu16_epi32(_mm256_extracti128_si256(yV, 0));
            __m256i xH = _mm256_cvtepu16_epi32(_mm256_extracti128_si256(xV, 0));
            __m256i cordsV = _mm256_mullo_epi32(yH, widthV);
            cordsV = _mm256_add_epi32(cordsV, xH);
            _mm256_store_si256((__m256i*) (cords), cordsV);

            yH = _mm256_cvtepu16_epi32(_mm256_extracti128_si256(yV, 1));
            xH = _mm256_cvtepu16_epi32(_mm256_extracti128_si256(xV, 1));
            cordsV = _mm256_mullo_epi32(yH, widthV);
            cordsV = _mm256_add_epi32(cordsV, xH);
            _mm256_store_si256((__m256i*) (cords + 8), cordsV);


            for (auto p : cords) {
                if (i++ == particle_count) {
                    flag = false;
                    break;
                }
                else {
                    buffer[p] = arche.symbol;
                }
            }

            x += CORDS_PER_LINE;
            y += CORDS_PER_LINE;
        }
    }
}

void ParticleEngine::display() const {
    std::basic_string_view view = buffer;

    for (int i = 0; view.size() - width > i; i += width) {
        std::cout << view.substr(i, width) << '\n';
    }
    std::cout << view.substr(view.size() - width, width) << std::flush;
}

ParticleEngine::ParticleArchetype::ParticleArchetype(unsigned char symbol, cords_t speed, std::size_t count) :
    symbol(symbol),
    speed(speed),
    particles({}, {}, 0)
{
    particles.reserve((count / CORDS_PER_LINE + 1) * CORDS_PER_LINE);
    particles.resize(count);
}
