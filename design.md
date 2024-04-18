# Taint-Based Bounds Checking (Design)  
## Identification of Tainted Pointers  
1. Anything coming back from library calls  
2. We want to look at different APITypes (main, read, recv, strcpy, userdefined functions as well)  
3. Also interested in arguments that have user tainted data.  
4. Try to look at Value* and the users 
## Propagation  
1. Dataflow Analysis; propagagte tainted status down the CFG.
2. Also maybe propagagte the FAT status for Memory Writer. Or just make it general-purpose enough for this to happen.
## Memory Writer  
1. Given a tainted pointer list apply the Memory Writer Algorithm as defined so in the paper.  
2. Prune the list down to arrays and pointer dereferences (left-hand side of assignment)
## Actual Bounds Checking  
1. Given a list of places to put a bounds check, do so.  
2. Should be relatively straightforward (I hope)
