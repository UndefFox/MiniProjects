#pragma once

#include <vector>
#include <string>


/** @brief Simple snowfall simulation.
 *
 *  Simulation done by simple set of particles variables with force applied to
 *  them with rendering to the buffer.
 */
class ParticleEngine {
private:
	struct Particle {
		unsigned int x;
		unsigned int y;
	};

	struct ParticleArchetype {
		std::vector<Particle> particles;
		unsigned char symbol;
		int speed;

		ParticleArchetype(unsigned char symbol, int speed, unsigned int count);
	};


private:
	unsigned int fps = 30;
	std::string buffer;
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

private:
	unsigned int coordinatesToBufferIndex(unsigned int x, unsigned int y) const;
};
