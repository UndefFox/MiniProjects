#include "particleengine.h"

#include <cstdlib>
#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>
#include <stdexcept>



// How many symbols past render line to reserve for special symbols
#define BLANK_CHAR_COUNT 1


void ParticleEngine::addArchetype(unsigned char symbol, unsigned int count, int speed)
{
	if (symbol > 127)
		throw std::invalid_argument("Extended ASCII isn't supported!");

	archetypes.emplace_back(symbol, speed, count);
}

void ParticleEngine::resizeImage(unsigned int width, unsigned int height)
{
	height -= 1;

	this->width = width;
	this->height = height;

	for (auto& arche : archetypes) {
		for (auto& particle : arche.particles) {
			particle.x = std::rand() % width;
			particle.y = std::rand() % height;
		}
	}


	// -1 to avoid autoscrolling by going to the next line
	buffer.resize(coordinatesToBufferIndex(width - 1, height));
	std::memset(buffer.data(), ' ', buffer.size());
	for (int r = 0; r < height - 1; r++) {
		buffer[coordinatesToBufferIndex(width, r)] = '\n';
	}
}

void ParticleEngine::waitForFrameDelay() const {
	std::this_thread::sleep_for(std::chrono::milliseconds{1000 / fps});
}

void ParticleEngine::setFPS(unsigned int newFPS) {
	if (newFPS == 0)
		throw std::invalid_argument("fps can't be 0!");

	fps = newFPS;
}

void ParticleEngine::tick()
{
	for (auto& arche : archetypes) {
		for (auto& particle : arche.particles) {
			buffer[coordinatesToBufferIndex(particle.x, particle.y)] = ' ';

			particle.y += arche.speed;

			if (particle.y > height) {
				particle.x = std::rand() % width;
				particle.y = 0;
			}

			buffer[coordinatesToBufferIndex(particle.x, particle.y)] = arche.symbol;
		}
	}
}

void ParticleEngine::display() const
{
	std::cout << buffer << std::flush;
}

unsigned int ParticleEngine::coordinatesToBufferIndex(unsigned int x, unsigned int y) const {
	return x + y * (width + BLANK_CHAR_COUNT);
}

ParticleEngine::ParticleArchetype::ParticleArchetype(unsigned char symbol, int speed, unsigned int count) :
	symbol(symbol),
	speed(speed)
{
	particles.resize(count);
}


