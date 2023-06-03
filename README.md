# CSC-335-Operating-System
Personal and Team project for Operating System class
Build upon os161 codebase from Harvard College
- `proj1-proj2` was an individual project to implement a doubly linked-list in C, resolving interleaving with mutex lock and semaphores, implement synchronization mechanisms: mutex lock, semaphores, condition variables and use synchronization to solve shared buffer problem
- `dvz-main` was a team project to build upon existing os161 code base by adding new file and process systemcalls. The result is a working OS with basic systemcalls implemented: (for file) open, close, read, write, lseek, dup2, __getcwd, __chdir | (for process) fork, excev, waitpid, getpid
- `os161-dvz-main/src/design/` folder contains all the design docs and progress report in a team
- `os161-dvz-main/src/kern/syscall` folder contains all the systemcalls implemented in a team project
- `os161-vud-proj1-proj2/src/kern/concurrent_list` folder contains the doubly linked-list implementation in C
- `os161-vud-proj1-proj2/src/kern/shared_buffer` folder contains the shared buffer synchronization problem with monitor
- `os161-vud-proj1-proj2/src/kern/thread/synch.c` contains the synchronization mechanisms implemented including mutex lock, semaphores, condition variables

