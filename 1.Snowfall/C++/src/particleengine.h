#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <new>


/** @brief Simple snowfall simulation.
 *
 *  Simulation done by simple set of particles variables with force applied to
 *  them with rendering to the buffer.
 */
class ParticleEngine {
public:
    typedef uint16_t cords_t;

private:
    template <typename T, int alligment>
    struct AlignedAllocator {
        typedef T value_type;

        inline T* allocate(std::size_t n) {
            return static_cast<T*>(::operator new(n * sizeof(T), std::align_val_t(alligment)));
        }

        inline void deallocate(T* p, std::size_t n) noexcept {
            ::operator delete(p, std::align_val_t(alligment));
        }

        template<typename U>
        struct rebind { using other = AlignedAllocator<U, alligment>; };
    };

	struct ParticleArchetype {
        struct Particls {
            std::vector<uint16_t, AlignedAllocator<uint16_t, 32>> x;
            std::vector<uint16_t, AlignedAllocator<uint16_t, 32>> y;
            std::size_t highestParticleIndex;

            inline void resize(std::size_t n) { x.resize(n); y.resize(n); }
            inline void reserve(std::size_t n) { x.reserve(n); y.reserve(n); }
            inline std::size_t size() { return x.size(); }
        } particles;

		unsigned char symbol;
        cords_t speed;

        ParticleArchetype(unsigned char symbol, cords_t speed, std::size_t count);
	};


private:
	unsigned int fps = 30;
    std::basic_string<char, std::char_traits<char>, AlignedAllocator<char, 32>> buffer;
	std::vector<ParticleArchetype> archetypes;
	unsigned int width;
	unsigned int height;

public:
	/** @brief Adds new particle type to the engine.
	 *
	 *  @p symbol Character to render this particle as.
	 *  @p count The amount of particles of this type that must be added.
	 *  @p speed What speed particle moves with vertically. The axis is oriented
	 *           downward.
	 */
	void addArchetype(unsigned char symbol, unsigned int count, int speed);

	/** @brief Changed the size of the drawing/simulation space.
	 *
	 *  The entire area is used for drawing with no margins/padding.
	 *
	 *  @p width Width of the space.
	 *  @p height Heigth of the space.
	 */
	void resizeImage(unsigned int width, unsigned int height);

	/** @brief Sleeps thread for the duration of one frame.
	 *
	 *  The framerate is set via ParticleEngine::fps variable. Doesn't account for
	 *  the delay introduced by outside factors.
	 */
	void waitForFrameDelay() const;

	/** @brief Sets ParticleEngine::fps variable.
	 *
	 *  @throws std::invalid_argument If @p newFPS is 0.
	 */
	void setFPS(unsigned int newFPS);



	/** @brief Progresses simulation by one frame.
	 */
	void tick();

	/** @brief Prints ParticleEngine::buffer into the default output.
	 *
	 *  The screen isn't cleared automatically.
	 */
	void display() const;
};
