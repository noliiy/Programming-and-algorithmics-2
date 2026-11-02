# Programming & Algorithmics 2 (PA2) — C++ Portfolio

Solutions to the coursework of **PA2 (Programming and Algorithmics 2)**, an advanced C++ course:
7 homework assignments, 4 lab practices and a timed exam.

## Highlights

- **Exam: 100 / 100** — designed and implemented an **advanced-level algorithm in 4 hours** and received the full score.
- ~5,600 lines of C++ (C++17/20), all compiling cleanly.
- Solutions are written for automated judging (Progtest): correctness on large hidden tests and strict memory/time limits.

## Skills demonstrated

| Area | Where |
|------|-------|
| Object-oriented design, polymorphism, rule of three/five | HW 3, 4, 6, Lab 6 |
| Manual memory management, copy-on-write, reference counting | HW 3, Lab 6 |
| STL containers & algorithms, custom comparators | HW 1, 4, 5, 6 |
| Graph traversal & event/log processing | HW 5 |
| Templates, variadic templates, compile-time dimensions | HW 7 |
| Operator overloading, stream I/O, C++20 `<=>` | HW 2, Lab 5 |
| Algorithmic complexity (O(log n) lookups, efficient indexing) | HW 1, 5 |

## Contents

| Project | Description |
|---------|-------------|
| [`hw1-vat-register`](homework/hw1-vat-register) | Tax database indexed by tax ID and (name, address) |
| [`hw2-date`](homework/hw2-date) | Gregorian `CDate` with arithmetic and custom stream formatting |
| [`hw3`](homework/hw3) | Versioned file with undo, copy-on-write 128-byte blocks |
| [`hw4-mailbox`](homework/hw4-mailbox) | Mail server mailbox simulation |
| [`hw5-base-logs`](homework/hw5-base-logs) | Visitor-log analysis on a graph of base zones |
| [`hw6-dns`](homework/hw6-dns) | DNS backend with A / AAAA / MX records |
| [`hw7-tensor`](homework/hw7-tensor) | Generic multidimensional `CTensor` with views (3 parts) |
| [`labs`](labs) | Practices 3–6 (contacts, time class, linked set) |
| [`exam`](exam) | Exam result and notes |

Every folder holds the solution (`.cpp`) next to the original task statement (`.txt`).

## Build

```sh
g++ -std=c++20 -Wall -Wextra -pedantic homework/hw2-date/homework2.cpp -o hw2 && ./hw2
```

## Author

Yusuf Emre Erdem — [erdemyusufemre@gmail.com](mailto:erdemyusufemre@gmail.com)
