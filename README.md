# Filesys

This project is a C++ implementation of a file system manager, built on top of a hash table with dynamic rehashing, supporting advanced collision handling policies. The system stores file objects identified by names and disk blocks and supports insertion, deletion, searching, updating, and dumping. It employs incremental rehashing for performance efficiency and supports dynamic switching between collision resolution strategies like Linear, Quadratic, and Double Hashing.

Core Features

1. Dynamic Hash Table for File Management
Each File object includes:

File name (string).
Disk block address (int).
Usage flag (for lazy deletion).
File System (FileSys class) functionalities:

Insert new file.
Remove existing file.
Search/Get file by name and block.
Update disk block of an existing file.
Dump current and old tables for debugging.
Dynamic rehashing and collision policy switching.

2. Object-Oriented Class Design
File Class:
Represents a file with:
Name (key), Disk block (unique), Usage flag.
Supports:
Comparisons (==), assignment.
Formatted output (overloaded <<).
FileSys Class:
Manages the file system using two hash tables (for rehashing transition):
m_currentTable: active table.
m_oldTable: previous table during rehash.
Supports incremental rehashing to spread the cost over operations.
Collision policies supported:
LINEAR, QUADRATIC, DOUBLEHASH.
Monitors load factor and delete ratio to trigger rehashing.

3. Hash Table Mechanics & Algorithms
Feature	Description
Collision handling	Linear, Quadratic, Double Hashing
Dynamic resizing	Automatically resizes when load or delete thresholds are crossed
Incremental rehashing	Gradually migrates entries to avoid large delays
Lazy deletion	Marks entries as unused, reused when possible
Prime sizing	Uses prime numbers for optimal hash distribution
Custom hash functions	Pluggable via function pointers (e.g., hashCode)

4. Rehashing Logic
Triggers:
Load factor λ > 0.5.
Deleted entries ratio > 0.8.
Incremental Process:
Transfers ~25% of old table on each operation.
Collision Policy Change:
On-demand rehash to switch between linear, quadratic, or double hash probing.

5. Memory Management
Safely handles:
Dynamic allocation and deallocation of File objects.
Rebuilds tables during rehashing.
Clears old tables once fully migrated.

6. Comprehensive Testing (mytest.cpp)
Includes 10+ unit tests covering:

Test Name	Purpose
testInsertNonCollidingKeys()	Insert unique keys without collision
testCorrectBucketInsertion()	Validate correct hash placement
testInsertCollidingKeys()	Insert keys causing hash collisions
testFindNonExistingKeys()	Search for keys that don't exist
testFindNonCollidingKeys()	Retrieve non-colliding keys
testFindCollidingKeys()	Retrieve colliding keys
testRemoveNonCollidingKeys()	Delete non-colliding keys and check state
testRemoveCollidingKeysWithoutRehash()	Handle collisions in delete without rehash
testRehashingLoadFactor()	Test automatic rehashing when load factor high
testRehashingDeleteRatio()	Test rehashing when many lazy deletes

Skills & Concepts Demonstrated
Hash Table design: Dynamic, incremental resizing.
Collision handling: Linear, Quadratic, Double Hashing.
Lazy deletion and rehashing thresholds.
Incremental rehashing to maintain constant-time operations.
Memory management: Dynamic allocation and safe cleanup.
Testing and debugging: Automated, thorough verification.
Randomized test data and hashing for robust testing.
