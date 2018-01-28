# CPLUSPLUS_STUDY_NOTES
SimpleConnectionPoolDemo1.cpp is a simple demo of a minimal implementation of a connection pool of connections. 
The program does a lazy initialization of a ConnectionPool class that pools a fixed number of connections and serves new connection requests from this pool.
The ConnectionPool does not initialize all the connections in its pool from the start, but only creates and adds new connections to the pool when the current number of available connections in the pool is zero.
The client gets a very basic interface to work with.

What is currently missing ?
Exception Handling will be added soon.
More modularization of the code to separate the different classes into their own header files.
Add interfaces wherever possible.
Create a separate client program to test the Connection Pool implementation.
