# Plastic-Parallel-Programming

In this project, we investigate the performance possibilities of plastic parallel programs, which adapt to the current state of the host system and explicitly cooperate to share and optimise the use of system resources.

In the first year of this project, we created an example of a parallel programming library which implemented these features for a single parallel programming pattern, that of map-array. We investigated the overhead incurred by such a system, concluding that it was insignificant, and we produced some promising preliminary results into its performance. 

In the second year of the project, we developed upon this by investigating a less predictable parallel pattern, that of stencil codes, with a focus on potential performance to be gained from plasticity when run in contention with other programs. We designed and implemented a synthetic test stencil program, along with testing scripts and data analysis applications. We then evaluated the synthetic test program with two variants running in contention in a variety of scenarios. These were ultimately designed to reveal the potential for performance gains attainable by a contention aware plastic parallel programming library, over and above the normal performance benefits attained by parallelism. The results of these experiments showed potential speedups from 1.03 to 2.44, compared to a non contention-aware system. We then investigated some more complex scenarios, involving three variants running in contention, which showed speedups from 1.13 to 2.43, compared to a non contention-aware system.