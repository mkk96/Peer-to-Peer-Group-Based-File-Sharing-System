# Mini-Torrent: Peer-to-Peer Group File Sharing

A BitTorrent-style, group-based peer-to-peer file sharing system in C++. A central tracker keeps the metadata, and peers download pieces of a file from other peers in parallel.

![C++](https://img.shields.io/badge/C%2B%2B-00599C?style=flat-square&logo=cplusplus&logoColor=white)
![Linux](https://img.shields.io/badge/platform-Linux-FCC624?style=flat-square&logo=linux&logoColor=black)
![Sockets](https://img.shields.io/badge/TCP-sockets-4E4E4E?style=flat-square)
![pthreads](https://img.shields.io/badge/multithreading-pthreads-2E7D32?style=flat-square)
![OpenSSL](https://img.shields.io/badge/OpenSSL-SHA1-721412?style=flat-square&logo=openssl&logoColor=white)

## Overview

The system has two programs:

- **Tracker** (`tracker.cpp`): a multi-threaded TCP server. It stores users, groups, membership and join requests, and for each shared file it records the file size, the SHA1 hash of every piece and the peers that have the file. It never stores or forwards file contents.
- **Client / peer** (`client.cpp`): a command-line client that talks to the tracker, plus a background server thread that serves file pieces to other peers.

Files are shared within **groups**. A user creates a group and becomes its owner. Other users ask to join, and the owner accepts. Only group members can upload to a group or download from it.

![Architecture](https://user-images.githubusercontent.com/48115585/139272264-5951ea87-0a67-4f53-a49a-c6003274e550.png)

### How a download works

1. The client asks the tracker for the file's size and the list of peers that have it, with their addresses.
2. The client asks each peer for its piece **bit vector**, which shows the pieces that peer holds.
3. The client walks through the pieces and assigns them to peers in round-robin order. Each piece is fetched on its own thread.
4. Each thread writes its piece at the correct offset in the destination file.
5. When the download finishes, the client hashes the file piece by piece and registers itself with the tracker as a new seeder. The tracker compares these hashes with the ones recorded at upload time and rejects the file if they differ.

## Features

- User accounts: create a user, log in, log out.
- Groups: create, join (by request), leave, accept join requests, list pending requests and list all groups.
- Share files within a group. The tracker stores per-piece SHA1 hashes, computed with OpenSSL.
- Parallel, multi-peer downloads with one thread per piece.
- Downloaded files become shareable from the downloading peer.
- One thread per connected peer on the tracker, with mutexes guarding updates to the shared user and group maps.

## Tech stack

- **Language:** C++
- **Networking:** BSD sockets (TCP, IPv4)
- **Concurrency:** POSIX threads (`pthread`) and mutexes
- **Hashing:** OpenSSL `SHA1` (`libcrypto`)
- **Platform:** Linux

## Project structure

```
Peer-to-Peer-Group-Based-File-Sharing-System/
├── tracker.cpp          # Tracker: users, groups, file metadata, peer lookup
├── client.cpp           # Peer: CLI, tracker protocol, piece server, parallel downloader
├── tracker_info.txt     # Tracker address(es), one IP:PORT per line
└── AOS_Assignment3.pdf  # Original assignment specification
```

## Getting started

### Prerequisites

- Linux
- `g++` and the OpenSSL development headers:

```bash
sudo apt-get install g++ libssl-dev
```

### Build

```bash
git clone https://github.com/mkk96/Peer-to-Peer-Group-Based-File-Sharing-System.git
cd Peer-to-Peer-Group-Based-File-Sharing-System

g++ tracker.cpp -o tracker -pthread
g++ client.cpp  -o client  -pthread -lcrypto
```

On OpenSSL 3.x the `SHA1()` call may produce deprecation warnings. These are harmless.

### Tracker info file

`tracker_info.txt` lists trackers as one `IP:PORT` per line:

```text
127.0.0.1:3601
127.0.0.1:3602
```

Only the first entry is used. The tracker binds to it and the clients connect to it.

### Run

Start the tracker:

```bash
./tracker tracker_info.txt
```

Start one or more clients. Each one needs its own `IP:PORT`, which it uses to serve pieces to other peers:

```bash
./client 127.0.0.1:18000 tracker_info.txt
./client 127.0.0.1:18001 tracker_info.txt
```

Stop the tracker with `Ctrl+C`.

## Usage

Type commands into a running client:

| Command | Description |
| --- | --- |
| `create_user <user_id> <password>` | Register a new user with the tracker |
| `login <user_id> <password>` | Log in |
| `create_group <group_id>` | Create a group (you become the owner) |
| `join_group <group_id>` | Request to join a group |
| `leave_group <group_id>` | Leave a group (the owner can't leave) |
| `list_requests <group_id>` | Owner only: list pending join requests |
| `accept_request <group_id> <user_id>` | Owner only: accept a join request |
| `list_groups` | List all groups on the network |
| `list_files <group_id>` | List files shared in a group |
| `upload_file <file_path> <group_id>` | Share a local file with a group |
| `download_file <group_id> <file_name> <destination_file_path>` | Download a shared file from peers |
| `show_downloads` | Show files this client has shared or downloaded in this session |
| `logout` | Log out |

### Example

```text
# Peer A (127.0.0.1:18000)
create_user alice pass1
login alice pass1
create_group g1
upload_file /home/alice/data/report.pdf g1

# Peer B (127.0.0.1:18001)
create_user bob pass2
login bob pass2
join_group g1

# Peer A
list_requests g1
accept_request g1 bob

# Peer B
list_files g1
download_file g1 report.pdf /home/bob/Downloads/report.pdf
show_downloads
```

## Known limitations

- Only a single tracker is supported. All state is kept in memory and is lost when the tracker stops.
- Use absolute file paths.
- `stop_share` is accepted by the client, but it only prints a confirmation and does not notify the tracker. Logging out does not stop the user's files from being shared.
- The piece size is fixed by `blockSize` in `client.cpp` (`512 * 102400` bytes). Each piece is received with a single `recv()` call, so large pieces may not arrive in full over a real network. The system is best suited to small files and local testing.
- The original notes say the SHA1 comparison can report a mismatch for binary files even when the download is correct.
- Passwords are stored in plain text in the tracker's memory and sent unencrypted.

## Context

Built for Assignment 3 of the Advanced Operating Systems course at IIIT Hyderabad (M.Tech, 2021). `AOS_Assignment3.pdf` is the original specification.

---

Made by [Mukul Kumar](https://mkk96.github.io) · [GitHub](https://github.com/mkk96) · [LinkedIn](https://www.linkedin.com/in/mukul-kumar-60322616b/)
