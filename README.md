# CSE 220 Fall 2024 Lab 3
By Project Group 38: Jacob Dickerman and Qizhong Wang.  

## Overview

In this project, we implemented the Domino Temporal Data Prefetcher (as outlined in *Mohammad Bakhshalipour, Pejman Lotfi-Kamran, and Hamid Sarbazi-Azad. “Domino Temporal Data Prefetcher.” In HPCA. 2018`*) into the Scarab environment.  

Domino is a temporal prefetcher. The basic idea is that if an address is fetched more than once, the addresses that were fetched after it before would probably be called again soon, so we should prefetch them now.

To implement this, each core keeps a History Buffer and an Index Table; the History Buffer is a list of cache misses and prefetch hits, and the Index Table is a map that links each address to its most recent entry in the History Buffer. When an address is fetched, the Index Table is…indexed, the most recent History Buffer entry is found, and all entries since then are fetched.

## New Files

- **pref_domino.cc / pref_domino.h**: Implementation of the Domino prefetcher
- **pref_domino.param.def / pref_domino.param.h**: Interface between the Domino prefetcher and the Scarab environment

## Changed Files

- **pref_common.c**
- **pref_table.def**
- **param_files.def**

All had minor changes to add Domino to the existing prefetcher structure