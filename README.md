# MINI PROJECTS

This repository is a collection of my own small, finished projects where I try out different concepts to better understand them in practice.

### Structure

The repository consists of a list of folders, each containing a different project. Inside a project directory, there will be one or several folders named after the language/framework used, with some consisting of several ones if the project has mixed codebase.

Each implementation has its own README.md file, which provides details about that specific implementation and highlights interesting aspects of the project.

### Projects

1. [Snowfall](#snowfall)
2. [Old Comutator](#oldcom)
3. [Preview Renderer](#previewrenderer)
4. [Fox Benchmark](#foxbenchmark)

---

## <a id="snowfall">Snowfall</a>

[This](https://github.com/UndefFox/MiniProjects/tree/master/1.Snowfall) project was my first experience writing a small console application to explore basic concepts: receiving UNIX signals and handling them properly, retrieving input parameters in the standard way (getopt), and processing them, with some simple use of SIMD (AVX2) to make it better handle larger number of particles.


## <a id="oldcom">Old Comutator</a>

[This](https://github.com/UndefFox/MiniProjects/tree/master/2.OldComutator) one was my first time implementing proper multi-threaded logic and working with sockets, combined into a piece of software that actually sends data over the network in an encrypted way.


## <a id="previewrenderer">Preview Renderer</a>

[This](https://github.com/UndefFox/MiniProjects/tree/master/3.PreviewRenderer) is my first steps in doing rendering on a GPU via a proper Vulkan. Mainly done to practice organizing bigger codebase with clear design. The code itself isn't more complex than [official guide](https://vulkan-tutorial.com/), but has a sprinkle of managing memory for offscreen rendering and nice math algorithms.

## <a id="foxbenchmark">Fox Benchmark</a>

[This assignment](https://github.com/UndefFox/MiniProjects/tree/master/4.CPUBenchmark)  was given to practice the basics of fast math implementation, but curiosity got the best of me, and I tried to achieve the maximum performance possible. Using a few research papers and help from other people, I've managed to achieve ~75% of the CBLAS library's performance while keeping the code relatively simple for a benchmark project.
