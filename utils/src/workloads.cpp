#include <stdio.h>

#include <workloads.hpp>

/*
 * Test user function
 */

int returnOne(int in1, deque<int> in2)
{
    return 1;
}



/*
 * Test user function
 */

int oneTouch(int in1, deque<int> in2)
{
    return in1 + in2[0];
}



/*
 * Test user function
 */

// int collatz(int weight, deque<int> seeds) 
// {
//     for (int i = 0; i < weight; i++)
//     {
//         int start = seeds[0];

//         if (start < 1) 
//         {
//             fprintf(stderr,"Error, cannot start collatz with %d\n", start);
//             return -1;
//         }

//         int count = 0;
//         while (start != 1) 
//         {
//             count++;
//             if (start % 2) 
//             {
//                 start = 3 * start + 1;
//             } 
//             else 
//             {
//                 start = start/2;
//             }
//         }
//     }
    
//     return 1;
// }