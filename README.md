# Parallel A* Pathfinding (Assignment 2)

## Overview / Intention

This project aims to implement, test, and evaluate sequential and parallel
versions of the A\* pathfinding algorithm. The primary goal is to fulfill the
requirements of Assignment 2 for the Parallel Computing course (A/Prof. Dr.
Tran Van Hoai, Dr. Diep Thanh Dang, April 12, 2025). The target application
involves finding near-optimal paths in real-world road networks.

## Scope

The project encompasses the following key areas:

1. **Sequential A\* Implementation:**

    * Develop a working sequential A\* algorithm.

    * Adapt the algorithm to handle specific circumstances: dynamic/uncertain
    heuristic functions (`h`) and vector cost functions (`f`).

2. **Concurrent Data Structure:**

    * Implement two distinct variants of a concurrent sorted linked list (used
    as a set/priority data structure), exploring different synchronization
    strategies (e.g., coarse-grained, fine-grained, optimistic, lazy).

3. **Parallel A\* Implementation:**

    * Adapt the sequential A\* algorithm and the concurrent sorted list to
    create a functional parallel A\* version.

4. **Testing & Evaluation:**

    * Devise and implement methods to test the correctness of all algorithm and
    data structure implementations.

    * Conduct a comprehensive performance and scalability study comparing the
    implemented sequential and parallel A\* versions across multiple
    shared-memory platforms. Performance metrics include execution time and
    quality of results (path optimality/cost).

## Core Requirements (from Assignment 2)

* Working sequential A\* program (including variants for dynamic `h`/vector
`f`).

* Implementations of two concurrent sorted list variants.

* Working parallel A\* program merging the adapted algorithm and concurrent
list.

* Correctness testing methodology and implementation for all parts.

* A final report detailing implementations, testing, and
performance/scalability analysis.

* Source code submission (Python/C++ mix allowed).

* Performance tests run on at least 'N' shared-memory platforms, where 'N' is
the number of group members.

## Technical Approach

To balance development complexity with the performance requirements needed for
evaluation, we will adopt a hybrid C++/Python approach:

* **C++ Core:** Performance-critical components will be implemented in C++:

  * The concurrent sorted linked list variants (required by the assignment
    for parallel A\*).

  * The core A\* algorithm logic (sequential and parallel adaptations).

  * Parallelism implementation using native C++ concurrency features (e.g.,
    `std::thread`, `std::mutex`).

* **Python:** Python will be used for:

  * Loading and parsing OpenStreetMap data.

  * Orchestrating the overall workflow.

  * Interfacing with the C++ core via Python bindings (e.g., using
    `pybind11`).

  * Implementing verification tests.

  * Performing performance analysis and generating plots/results for the
    report.

This approach leverages C++'s performance for computation and concurrent data
structures while using Python's strengths in data handling, scripting, and
analysis.

## Getting Started

*(Placeholder: Add build instructions, dependencies, and how to run the code
here later)*
