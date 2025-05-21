# Trivia Project in C

**Author:** Jake Paccione

## Overview

Trivia is a game application ran through the terminal that implements low and high level File I/O, sockets, and multiplexed I/O using `select()`, allowing for an established server-client connection. A central server accepts up to three simultaneous client connections, reads a configurable question file, and steps through a series of questions—broadcasting each to all players, collecting the first response, updating scores, and announcing winners at the end.

## Features

- **Concurrent Multiplayer:**  
  Supports up to 3 players connecting over TCP; server tracks each socket in a `select()` loop.

- **Dynamic Question Loading:**  
  Parses prompts, options, and answers from a plain-text file (`-f questions.txt`), allowing easy quiz customization.

- **Multiplexed I/O:**  
  Uses `select()` on both sockets and STDIN in the client to interleave network reads and user input without blocking.

- **Robust Server-Client Protocol:**  
  Cleanly handles new connections, unexpected disconnects, invalid input, and end-of-game teardown.

- **Command-Line Configuration:**  
  `-f`: question file  
  `-i`: IP address to bind/connect  
  `-p`: port number  
  `-h`: help message

- **Scorekeeping & Results:**  
  Maintains per-player scores (increment/decrement on right/wrong answers), announces overall winner(s) at game end.

- **Build Automation:**  
  Simple Makefile with `all` and `clean` targets; compiles with `-g -Wall` for warnings.

## Build & Run  

```bash
# Build both server and client
make

# Start the server (listen on all interfaces, port 25555):
./server -f questions.txt -i 0.0.0.0 -p 25555

# On each client machine, connect using the server's LAN IP (or host's IPv4 for online play):  
# Note: If running through a VM, ensure the host's network adapter is set to “Bridged”  
./client -i 10.0.2.15 -p 25555
```


