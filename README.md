<h1>Reprospection after 3 years</h1>

This is a programming excercise I did a few years ago back in 2012 as part of the interview process for a startup company. The excercise was to write an algorithm for four elevators, and the goal is to make sure each person's wait time (either how long they have to wait to get on to an elevator, or for them to get to their desired floor, I can't remember any more....) is as low as possible.

It is easy to see that there could be a pretty deterministic approach for this problem, however, I chose to go with a multithreaded approach, which at the time seemed like a good idea, but on hind sight I was over complicating things, and although it appeared to be a smart way of approaching this, it did not yield a better solution and gave you different results each time you run the program with the same input set.

But if you are interested, here is my attempt at this problem.


<h2>Description</h2>

The meat lies in the calculate_next_floor function.

So each time that funtion is called, it will go through two lists, the onboard guests list and the waiting guests list. And it will do a tally of the requests, to see how many guests onboard need to go up from the current floor and how many guests onboard need to go down the current floor, also how many guests are waiting above the current floor to go up, and how many down, and how many guests are waiting below the current floor to go down, and how many up. It is my design that the elevator adhere to its course of movement if needed, so if an elevator is going up, if there are requests that requires the elevator to go higher, no matter how many requests are below the current floor that the elevator is on, it will go up to the higher requests.

Originally I only used one elevator, and the average wait time for the two 10000 + entries test files is around 900, and the standard deviation is around 500. Then I used four threads for four elevators, the result didn't improve fourfold, instead the average wait time is at around 500, and the standard deviation is around 300.

A lot of improvements could be made. For example, the elevators do not take into account which floors those other elevators are going to, so the distribution of the requests is the only factor that affects the direction of the elevator, so if there are slighestly more requests coming from higher floors, then all the elevators are going up, leaving the guests waiting on the lower floors waiting for a long time, when we could split the elevators to make some of them answer the requests on the higher floors and some answer the requests coming from the lower floors.
