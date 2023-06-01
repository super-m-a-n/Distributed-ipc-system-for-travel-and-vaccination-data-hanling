# Systems Programming Project 2

## About
This project implements a distributed system which utilizies inter process communication to handle / accept / reject various user travel requests and queries based on their vaccination records. We have one ```travelMonitor``` parent process which uses named pipes to communicate with a number of child ```Monitor``` processes. Furtherly, various system calls/signals and low-level I/O are used for the communication. The vaccination data found in folder ```/input_dir``` are produced using a bash script ```create_infiles.sh```. Detailed information about the project's specifications and the user requests on the data can be found in the project's pdf file : hw2-spring-2021.pdf
