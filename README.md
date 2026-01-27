# Book_your_seats
# Multiplex Cinema Ticketing System — Client/Server in C

Multi-threaded TCP/IP client–server application written in C for managing ticket reservations in a multiplex cinema.  
Developed as a Bachelor’s degree university project in Electronic Engineering.

---

##  Overview

This project implements a networked booking system composed of:

- a **server** handling multiple concurrent clients
- a **terminal-based client** for user interaction

The server manages seat availability across multiple cinema rooms, supports administrator operations through password authentication, and guarantees consistency using mutex-protected shared state.

---

##  Features

- TCP/IP communication using POSIX sockets
- Multi-client handling via pthreads
- Maximum concurrent worker threads
- Thread-safe global state using mutexes
- Custom binary protocol with explicit length fields
- Booking, cancellation and seat tracking
- Administrator authentication with password file
- Password update with validation rules
- Graceful client disconnection and CTRL+C handling
- Reset and close of individual rooms or all rooms

---

##  Supported Commands

From the client terminal:

- `help` — show command list  
- `view [sala]` — show available seats  
- `book [sala] [posti]` — reserve seats  
- `cancel [sala]` — cancel reservation  
- `open [sala]` — open bookings (admin)  
- `close [sala]` — close bookings and reset room (admin)  
- `changepwd` — change administrator password  
- `quit` — disconnect

---

##  Architecture

### Server
- Listens on a TCP socket
- Spawns worker threads for each client
- Uses shared global state:
  - remaining seats per room
  - booking registry
  - booking enable/disable flags
- Synchronizes access using multiple mutexes
- Stores administrator password in a file

### Client
- CLI interface
- Parses user input
- Sends structured messages to server
- Handles server replies
- Supports SIGINT (CTRL+C)

---
### Protocol
Each request/response uses:

1. numeric fields in network byte order
2. message length headers
3. raw payload transmission

---

##  Technologies

- Language: C
- OS: Linux / POSIX
- Networking: BSD sockets
- Concurrency: pthreads
- Synchronization: mutex + condition variables
- Build: gcc / Makefile

---
## Project Context

Developed during the Bachelor’s degree in Electronic Engineering at the University of Pisa (Elaboration systems course).

---
## What I Learned

- socket programming in C
- designing simple application-layer protocols
- multi-threaded server architectures
- race-condition prevention with mutexes
- debugging concurrent systems
- input validation and error handling
- file-based credential management

---
 ## License

Educational project — feel free to reuse for learning purposes.

## Build & Run

Compile:

```bash
gcc -pthread server.c -o server
gcc client.c -o client
