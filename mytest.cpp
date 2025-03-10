#include "filesys.h"
#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
using namespace std;

// Simple hash function
unsigned int simpleHash(string key) {
    return key.length() % 5;
}

// Random number generator class
class Random {
public:
    Random(int min, int max) : m_min(min), m_max(max), m_generator(10), m_unidist(min, max) {}
    int getRandNum() { return m_unidist(m_generator); }
private:
    int m_min, m_max;
    mt19937 m_generator;
    uniform_int_distribution<> m_unidist;
};

// Tester class
class Tester {
public:
    bool testInsertNonCollidingKeys();
    bool testCorrectBucketInsertion();
    bool testInsertCollidingKeys();
    bool testFindNonExistingKeys();
    bool testFindNonCollidingKeys();
    bool testFindCollidingKeys();
    bool testRemoveNonCollidingKeys();
    bool testRemoveCollidingKeysWithoutRehash();
    bool testRehashingLoadFactor();
    bool testRehashingDeleteRatio();
    void runTest(const string& testName, bool (Tester::*testFunc)(), int& passed, int& total);
};

// Test insertion of non-colliding keys and validate size
bool Tester::testInsertNonCollidingKeys() {
    FileSys filesys(10, simpleHash, LINEAR);
    vector<File> dataList = {
        File("file1.txt", 1001, true),
        File("file2.txt", 1002, true),
        File("file3.txt", 1003, true)
    };

    size_t initialSize = filesys.lambda(); // Record size before insertion
    for (const auto& file : dataList) {
        if (!filesys.insert(file)) {
            cout << "Failed to insert: " << file.getName() << endl;
            return false;
        }
    }

    if (filesys.lambda() <= initialSize) {
        cout << "Size did not increase after insertion!" << endl;
        return false;
    }

    return true;
}

// Test insertion with correct bucket placement
bool Tester::testCorrectBucketInsertion() {
    FileSys filesys(10, simpleHash, LINEAR);
    vector<File> dataList = {
        File("short", 1001, true), // Hashes to 5
        File("medium", 1002, true) // Hashes to 6
    };

    for (const auto& file : dataList) {
        filesys.insert(file);
    }

    // Verify bucket placement using simpleHash
    if (filesys.getFile("short", 1001).getDiskBlock() != 1001) {
        cout << "File 'short' not in correct bucket!" << endl;
        return false;
    }
    if (filesys.getFile("medium", 1002).getDiskBlock() != 1002) {
        cout << "File 'medium' not in correct bucket!" << endl;
        return false;
    }

    return true;
}

// Test insertion with colliding keys
bool Tester::testInsertCollidingKeys() {
    FileSys filesys(5, simpleHash, LINEAR);
    vector<File> dataList = {
        File("aaa", 1001, true), // Collides with "bbb" and "ccc"
        File("bbb", 1002, true),
        File("ccc", 1003, true)
    };

    for (const auto& file : dataList) {
        if (!filesys.insert(file)) {
            cout << "Failed to insert: " << file.getName() << endl;
            return false;
        }
    }

    for (const auto& file : dataList) {
        File retrieved = filesys.getFile(file.getName(), file.getDiskBlock());
        if (!(file == retrieved)) {
            cout << "Mismatch for: " << file.getName() << endl;
            return false;
        }
    }
    return true;
}

// Test finding a non-existing key
bool Tester::testFindNonExistingKeys() {
    FileSys filesys(10, simpleHash, LINEAR);
    filesys.insert(File("exists.txt", 1001, true));

    File retrieved = filesys.getFile("missing.txt", 9999);
    if (!retrieved.getName().empty()) {
        cout << "Unexpectedly found non-existing file." << endl;
        return false;
    }
    return true;
}

// Test finding non-colliding keys
bool Tester::testFindNonCollidingKeys() {
    FileSys filesys(10, simpleHash, LINEAR);
    vector<File> dataList = {
        File("fileA.txt", 2001, true),
        File("fileB.txt", 2002, true)
    };

    for (const auto& file : dataList) {
        filesys.insert(file);
    }

    for (const auto& file : dataList) {
        File retrieved = filesys.getFile(file.getName(), file.getDiskBlock());
        if (!(file == retrieved)) {
            cout << "Mismatch for: " << file.getName() << endl;
            return false;
        }
    }
    return true;
}

// Test finding colliding keys
bool Tester::testFindCollidingKeys() {
    FileSys filesys(5, simpleHash, LINEAR);
    vector<File> dataList = {
        File("key1", 1001, true),
        File("key2", 1002, true),
        File("key3", 1003, true) // Collides with key1 and key2
    };

    for (const auto& file : dataList) {
        filesys.insert(file);
    }

    for (const auto& file : dataList) {
        File retrieved = filesys.getFile(file.getName(), file.getDiskBlock());
        if (!(file == retrieved)) {
            cout << "Mismatch for: " << file.getName() << endl;
            return false;
        }
    }
    return true;
}

// Test removing non-colliding keys
bool Tester::testRemoveNonCollidingKeys() {
    FileSys filesys(10, simpleHash, LINEAR);
    File file1("toRemove1.txt", 3001, true);
    File file2("toRemove2.txt", 3002, true);

    filesys.insert(file1);
    filesys.insert(file2);

    filesys.remove(file1);

    File retrieved = filesys.getFile(file1.getName(), file1.getDiskBlock());
    if (!retrieved.getName().empty()) {
        cout << "Failed to remove file: " << file1.getName() << endl;
        return false;
    }

    retrieved = filesys.getFile(file2.getName(), file2.getDiskBlock());
    if (!(file2 == retrieved)) {
        cout << "Mismatch for: " << file2.getName() << endl;
        return false;
    }
    return true;
}

// Test removing colliding keys without rehashing
bool Tester::testRemoveCollidingKeysWithoutRehash() {
    FileSys filesys(5, simpleHash, LINEAR);
    File file1("aaa", 4001, true); // Collides with "bbb"
    File file2("bbb", 4002, true);

    filesys.insert(file1);
    filesys.insert(file2);

    filesys.remove(file1);

    if (!filesys.getFile(file1.getName(), file1.getDiskBlock()).getName().empty()) {
        cout << "Failed to remove 'aaa'" << endl;
        return false;
    }

    if (filesys.getFile(file2.getName(), file2.getDiskBlock()).getName().empty()) {
        cout << "Removed 'bbb' unexpectedly!" << endl;
        return false;
    }

    return true;
}

// Test rehashing due to load factor
bool Tester::testRehashingLoadFactor() {
    FileSys filesys(10, simpleHash, LINEAR);

    for (int i = 0; i < 20; i++) {
        filesys.insert(File("file" + to_string(i), 5000 + i, true));
    }

    for (int i = 0; i < 20; i++) {
        File retrieved = filesys.getFile("file" + to_string(i), 5000 + i);
        if (retrieved.getName().empty()) {
            cout << "Rehashing error: Missing file" << i << endl;
            return false;
        }
    }
    return true;
}

// Test rehashing due to delete ratio
bool Tester::testRehashingDeleteRatio() {
    FileSys filesys(10, simpleHash, LINEAR);

    vector<File> dataList;
    for (int i = 0; i < 20; i++) {
        File file("file" + to_string(i), 6000 + i, true);
        dataList.push_back(file);
        filesys.insert(file);
    }

    for (int i = 0; i < 16; i++) { // Remove 80% of entries
        filesys.remove(dataList[i]);
    }

    for (int i = 16; i < 20; i++) {
        File retrieved = filesys.getFile(dataList[i].getName(), dataList[i].getDiskBlock());
        if (!(dataList[i] == retrieved)) {
            cout << "Rehashing error: Missing file" << i << endl;
            return false;
        }
    }
    return true;
}

// Runs a single test and prints the result
void Tester::runTest(const string& testName, bool (Tester::*testFunc)(), int& passed, int& total) {
    cout << testName << ": ";
    if ((this->*testFunc)()) {
        cout << "PASSED" << endl;
        passed++;
    } else {
        cout << "FAILED" << endl;
    }
    total++;
}

// Main function to run tests
int main() {
    Tester tester;
    int passed = 0, total = 0;

    tester.runTest("Test Insert Non-Colliding Keys", &Tester::testInsertNonCollidingKeys, passed, total);
    tester.runTest("Test Correct Bucket Insertion", &Tester::testCorrectBucketInsertion, passed, total);
    tester.runTest("Test Insert Colliding Keys", &Tester::testInsertCollidingKeys, passed, total);
    tester.runTest("Test Find Non-Existing Keys", &Tester::testFindNonExistingKeys, passed, total);
    tester.runTest("Test Find Non-Colliding Keys", &Tester::testFindNonCollidingKeys, passed, total);
    tester.runTest("Test Find Colliding Keys", &Tester::testFindCollidingKeys, passed, total);
    tester.runTest("Test Remove Non-Colliding Keys", &Tester::testRemoveNonCollidingKeys, passed, total);
    tester.runTest("Test Remove Colliding Keys Without Rehash", &Tester::testRemoveCollidingKeysWithoutRehash, passed, total);
    tester.runTest("Test Rehashing Load Factor", &Tester::testRehashingLoadFactor, passed, total);
    tester.runTest("Test Rehashing Delete Ratio", &Tester::testRehashingDeleteRatio, passed, total);

    cout << "\nSummary: " << passed << " / " << total << " tests passed." << endl;
    return 0;
}