// Author Name: Jacob Rice
// Date: April 25th, 2022
// x500: rice0296

## Summary
Overall, I would say that my project turned out almost exactly as I would have wanted it to. Nearly
all of the rubric criteria was met (as explained in the bullet points below). My project heavily uses 
threads, mutexes, queues, and semaphores which I learned about in previous classes, and as a result, the 
server and client can do many things concurrently. The code has extensive error checking that should 
prevent crashes. Deadlock situations have also been debugged and removed. I was able to test the code
by running the server on my personal server and connecting to it thru clients running on the server,
my windows pc using WSL, and a friend (who is not is CSCI 4211) from outside my local network who ran the 
executable file on their machine (no source code was shared, just the client exe file and a script of 
commands), and the program worked as expected in all tested situations. I tested up to 10 clients at a time.

## Assumptions/Decisions Made
When a user is logged in one an existing client, and a new login request (with valid credentials) for that
same user comes from a new client, the first client is logged out (a message is sent to notify this client) 
and the client who sent the most recent login request is logged in. (Only 1 client can be logged into an 
account at a time).

The server does not have a set limit as to when to stop accepting new connections. It is unknown how many
clients the server could handle, but no more than 10 at a time were tested.

Both the client and server can be gracefully exited using Ctrl+C (SIGINT)

The server saves credentials to server_data.data upon server shutdown and every ~10 minutes.

The server can handle and detect packets that are not correctly formatted message_t's (This was tested when I 
accidentally tried to connect a Minecraft client to my chat server, and when old versions of the client were tested 
with new versions of the server and the protocol differed)

Overall, the communication between client and server is very insecure, as everything other than the password
is sent as plain text. It would be very easy for someone to launch attacks such as a replay attack on the server.

## Build Instructions
To build the whole project, run the "make" command inside the main folder

To rebuild the whole project, run "make clean; make" inside the main folder

To build just the server exe, run "make server"
To build just the client exe, run "make client"

To rebuild just the server, run "make cleans; make server"
To rebuild just the client, run "make cleanc; make client"


## Rubric and Completion of each Criteria
User registration and login/logout 2                    - Complete, including error checking

Sending and receiving messages 8                        - Complete, including error checking

Sending and receiving files (no concurrency) 4          - Complete

Sending and receiving files (with concurrency) 4        - Messages are queued so they do not block the 
                                                          sending of another message, but only 1 file 
                                                          is sent over the network at a time (only 
                                                          criteria not fully working)

List all users 1                                        - To Do

Error handling 4                                        - Fully working, the server and client can handle
                                                          unexpected networking errors and other errors

System performance 3                                    - The server is highly multithreaded, and will
                                                          have high performance on multi-core CPUs

Documentation 1                                         - The code is well documented

Multiple users sending files + messages concurrently 6  - Each client connection gets 2 threads on the
                                                          server, one for reading and one for sending.
                                                          The threads can run concurrently.

Add-on feature 4                                        - Offline message queuing: messages sent to
                                                          everyone and DM's to a specific person are
                                                          buffered on the server if the person is offline

                                                          Minor add-on: Passwords are hashed before they
                                                          are sent over the network and stored in the
                                                          server datafile