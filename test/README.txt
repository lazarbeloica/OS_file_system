In this folder is given a example of the way the file system should be used.
A user should include the fs.h and file.h from h folder to use file system funcionalities.
Since the file system does not know how to comunicate with the disk the partition clas will be 
used as a simulation of the disk.

Use p1.ini or p2.ini to configure disk1.dat or disk2.dat sizes (these two dat simulate partitions).
The partition interfaces is given in part.h file (use library with the part.h).

If you wish to use a test which is provided in this folder please choose one of the test files, copy it to the 
program location and rename it to the ulaz.dat. Output of the program will be izlaz1.dat and izlaz2.dat which will 
be cpies of the ulaz.dat.

All operations are thread safe.
