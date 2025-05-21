# Trivia Project in C

**Author:** Jake Paccione

## Overview

Trivia is a game application ran through the terminal that implements low and high level File I/O, sockets, and multiplexed I/O using `select()`, allowing for an established server-client connection. A central server accepts up to three simultaneous client connections, reads a configurable question file, and steps through a series of questions—broadcasting each to all players, collecting the first response, updating scores, and announcing winners at the end.

## Features

- **Concurrent Multiplayer:**  
  Supports up to 3 players connecting over TCP; server tracks each socket in a `select()` loop.

- **
