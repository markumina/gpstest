# Quick ESP-IDF GPS parser

## Based on:
- Given dataset, which I placed in gps_data.h (was going to toss it in an nvm file, but no time).
- Given instructions:
```
   For this project, you will write a simple program in C that:
   a) Reads this data set
   b) Separates the individual walks
   c) Shows summary information for each walk
```
## Hardware Used:
Esp32 DevKitc_v4, running ESP-IDF.

## Thoughts/Notes:
Very quick implementation of a parser. There are probably hundreds of libraries for this,
but I wanted to mess with the data from scratch/strings, just to gain a bit of intuition about GPS data.

I made the quick assumption that trips are broken up by a pause in travel time (1 hour pause).

If I were to take this farther, I might offer some additional insight into what constitutes
a trip - a pause may not be enough (maybe if you begin from the same point, you discount the pause).

In any case, lots of interesting behaviors can be witnessed from a small dataset, I've found, as
I thought about it while parsing - main reason why I wanted to parse it so manually.

In real life, I would have structurized things, spell-checked this README,
made flow of control in the code clearer, etc... (but time).

## Output (as tagged for 0.0.2):
This output has not been checked for much accuracy, but I'm super out of time on this test.
I am interested in discussion about the results, and had fun working with the data.

```
Length of GPS data: 54124



******* Trip 1 Results *******

Trip started @ . . . . . . . . . . . .  2018-03-13 21:15:01 +0000

Trip miles walked . . . . . . . . . . . 0.359271

Trip origin (long./lat.) . . . . . . .  -73.984138, 40.702454

Trip destination (long./lat.) . . . . . -73.984169, 40.702374



******* Trip 2 Results *******

Trip started @ . . . . . . . . . . . .  2018-03-13 23:50:12 +0000

Trip miles walked . . . . . . . . . . . 2.998486

Trip origin (long./lat.) . . . . . . .  -73.986771, 40.736820

Trip destination (long./lat.) . . . . . -73.986694, 40.736847



******* Trip 3 Results *******

Trip started @ . . . . . . . . . . . .  2018-03-14 11:39:44 +0000

Trip miles walked . . . . . . . . . . . 0.417593

Trip origin (long./lat.) . . . . . . .  -73.986778, 40.736866

Trip destination (long./lat.) . . . . . -73.986923, 40.736935



******* Trip 4 Results *******

Trip started @ . . . . . . . . . . . .  2018-03-14 12:59:28 +0000

Trip miles walked . . . . . . . . . . . 0.962967

Trip origin (long./lat.) . . . . . . .  -73.987099, 40.737034

Trip destination (long./lat.) . . . . . -73.990479, 40.735126



******* Trip 5 Results *******

Trip started @ . . . . . . . . . . . .  2018-03-14 21:59:52 +0000

Trip miles walked . . . . . . . . . . . 1.432216

Trip origin (long./lat.) . . . . . . .  -73.989983, 40.737698

Trip destination (long./lat.) . . . . . -73.986755, 40.736843



******* Additional Results (pertaining to total dataset) *******

Total number of walks . . . . . . . . . . . . .  5

Total miles walked . . . . . . . . . . . . . . . 6.514759

Total origin/dest straight line distance . . . . 2.388467


***NOTE:

 -Total number of walks based on 3600 second travel break.


End output
```



