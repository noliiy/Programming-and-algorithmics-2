# PA2 – Programming and Algorithmics 2 (C++)

Solutions to the homework assignments and lab practices of the PA2 course, written in modern C++.

## Exam result: 100 / 100

In the course exam I designed and implemented an **advanced-level algorithm within the 4-hour time limit**
and scored the **full 100 points**.

## Repository layout

| Path | Topic |
|------|-------|
| `homework/hw1-vat-register` | `CVATRegister` – VAT/tax database |
| `homework/hw2-date` | `CDate` – Gregorian calendar date class |
| `homework/hw3` | Homework 3 |
| `homework/hw4-mailbox` | `CMailBox` / `CMail` – mail server simulation |
| `homework/hw5-base-logs` | Military base visitor-log analysis |
| `homework/hw6-dns` | DNS server backend |
| `homework/hw7-tensor` | `CTensor` – multidimensional arrays and views (3 parts) |
| `labs/practice3` … `practice6` | Lab practice tasks |
| `exam/` | Exam solution (see below) |

Each folder contains the solution (`.cpp`) and the original task statement (`.txt`).

## Build

```sh
g++ -std=c++17 -Wall -Wextra -pedantic homework/hw2-date/homework2.cpp -o hw2
```
