#include "filesys.h"

// Constructor
FileSys::FileSys(int size, hash_fn hash, prob_t probing)
    : m_hash(hash), m_currProbing(probing), m_newPolicy(DEFPOLCY), m_transferIndex(0) {
    m_currentCap = findNextPrime(size);
    m_currentSize = 0;
    m_currNumDeleted = 0;
    m_currentTable = new File*[m_currentCap]();
    m_oldTable = nullptr;
    m_oldCap = 0;
    m_oldSize = 0;
    m_oldNumDeleted = 0;
}

// Destructor
FileSys::~FileSys() {
    for (int i = 0; i < m_currentCap; ++i) {
        delete m_currentTable[i];
    }
    delete[] m_currentTable;

    if (m_oldTable) {
        completeRehashing();
    }
}

// Insert
bool FileSys::insert(File file) {
    checkRehashCriteria(); // Check if rehashing is needed
    incrementalRehash();   // Perform incremental rehashing if applicable

    int index = m_hash(file.getName()) % m_currentCap;
    int step = 0;

    while (true) {
        int probeIndex = (index + resolveCollision(step, file.getName(), false)) % m_currentCap;

        if (!m_currentTable[probeIndex] || !m_currentTable[probeIndex]->getUsed()) {
            delete m_currentTable[probeIndex];
            m_currentTable[probeIndex] = new File(file);
            m_currentTable[probeIndex]->setUsed(true);
            ++m_currentSize;
            return true;
        }

        if (*m_currentTable[probeIndex] == file) {
            return false; // File already exists
        }

        ++step;
    }
}

// Remove
bool FileSys::remove(File file) {
    incrementalRehash(); // Perform incremental rehashing if applicable

    // Check current table
    int index = m_hash(file.getName()) % m_currentCap;
    int step = 0;

    while (step < m_currentCap) {
        int probeIndex = (index + resolveCollision(step, file.getName(), false)) % m_currentCap;

        if (!m_currentTable[probeIndex]) break;

        if (*m_currentTable[probeIndex] == file) {
            m_currentTable[probeIndex]->setUsed(false);
            ++m_currNumDeleted;
            return true;
        }

        ++step;
    }

    // Check old table
    if (m_oldTable) {
        index = m_hash(file.getName()) % m_oldCap;
        step = 0;

        while (step < m_oldCap) {
            int probeIndex = (index + resolveCollision(step, file.getName(), true)) % m_oldCap;

            if (!m_oldTable[probeIndex]) break;

            if (*m_oldTable[probeIndex] == file) {
                m_oldTable[probeIndex]->setUsed(false);
                ++m_oldNumDeleted;
                return true;
            }

            ++step;
        }
    }

    return false; // File not found
}

// Get File
const File FileSys::getFile(string name, int block) const {
    // Check current table
    int index = m_hash(name) % m_currentCap;
    int step = 0;

    while (step < m_currentCap) {
        int probeIndex = (index + resolveCollision(step, name, false)) % m_currentCap;

        if (!m_currentTable[probeIndex]) break;

        if (m_currentTable[probeIndex]->getName() == name &&
            m_currentTable[probeIndex]->getDiskBlock() == block &&
            m_currentTable[probeIndex]->getUsed()) {
            return *m_currentTable[probeIndex];
        }

        ++step;
    }

    // Check old table
    if (m_oldTable) {
        index = m_hash(name) % m_oldCap;
        step = 0;

        while (step < m_oldCap) {
            int probeIndex = (index + resolveCollision(step, name, true)) % m_oldCap;

            if (!m_oldTable[probeIndex]) break;

            if (m_oldTable[probeIndex]->getName() == name &&
                m_oldTable[probeIndex]->getDiskBlock() == block &&
                m_oldTable[probeIndex]->getUsed()) {
                return *m_oldTable[probeIndex];
            }

            ++step;
        }
    }

    return File(); // File not found
}

// Update Disk Block
bool FileSys::updateDiskBlock(File file, int block) {
    File found = getFile(file.getName(), file.getDiskBlock());
    if (found.getName().empty()) return false; // File not found

    return remove(found) && insert(File(file.getName(), block, true));
}

// Change Collision Policy
void FileSys::changeProbPolicy(prob_t policy) {
    m_newPolicy = policy;

    // Initiate rehashing if it hasn't started
    if (m_oldTable == nullptr) {
        m_oldTable = m_currentTable;
        m_oldCap = m_currentCap;
        m_oldSize = m_currentSize;
        m_oldNumDeleted = m_currNumDeleted;
        m_oldProbing = m_currProbing;

        m_currProbing = m_newPolicy;
        m_currentCap = findNextPrime((m_currentSize - m_currNumDeleted) * 4);
        m_currentTable = new File*[m_currentCap]();
        m_currentSize = 0;
        m_currNumDeleted = 0;
        m_transferIndex = 0;
    }
}

// Rehashing Helpers
void FileSys::checkRehashCriteria() {
    if (lambda() > 0.5 || deletedRatio() > 0.8) {
        changeProbPolicy(m_currProbing);
    }
}

void FileSys::incrementalRehash() {
    if (m_oldTable == nullptr) return;

    int transferLimit = m_oldCap / 4; // 25% of the old table
    for (int i = 0; i < transferLimit && m_transferIndex < m_oldCap; ++i, ++m_transferIndex) {
        if (m_oldTable[m_transferIndex] && m_oldTable[m_transferIndex]->getUsed()) {
            insert(*m_oldTable[m_transferIndex]);
            delete m_oldTable[m_transferIndex];
            m_oldTable[m_transferIndex] = nullptr;
        }
    }

    if (m_transferIndex >= m_oldCap) {
        completeRehashing();
    }
}

void FileSys::completeRehashing() {
    if (m_oldTable == nullptr) return;

    for (int i = 0; i < m_oldCap; ++i) {
        delete m_oldTable[i];
    }
    delete[] m_oldTable;

    m_oldTable = nullptr;
    m_oldCap = 0;
    m_oldSize = 0;
    m_oldNumDeleted = 0;
}

// Collision Resolution
int FileSys::resolveCollision(int step, const string& name, bool isOldTable) const {
    prob_t policy = isOldTable ? m_oldProbing : m_currProbing;
    switch (policy) {
        case LINEAR:
            return step + 1;
        case QUADRATIC:
            return step * step;
        case DOUBLEHASH: {
            int secondaryHash = 1 + (m_hash(name) % (isOldTable ? m_oldCap : m_currentCap - 1));
            return step * secondaryHash;
        }
    }
    return 0; // Default case
}

// Load Factor
float FileSys::lambda() const { return static_cast<float>(m_currentSize - m_currNumDeleted) / m_currentCap; 
}

float FileSys::deletedRatio() const { return static_cast<float>(m_currNumDeleted) / m_currentSize; 
}

// Dump
void FileSys::dump() const {
    cout << "Dump for the current table: " << endl;
    if (m_currentTable != nullptr) {
        for (int i = 0; i < m_currentCap; i++) {
            cout << "[" << i << "] : " << m_currentTable[i] << endl;
        }
    }
    cout << "Dump for the old table: " << endl;
    if (m_oldTable != nullptr) {
        for (int i = 0; i < m_oldCap; i++) {
            cout << "[" << i << "] : " << m_oldTable[i] << endl;
        }
    }
}

// Helper: Check if a number is prime
bool FileSys::isPrime(int number) {
    bool result = true;
    for (int i = 2; i <= number / 2; ++i) {
        if (number % i == 0) {
            result = false;
            break;
        }
    }
    return result;
}

// Helper: Find the next prime number
int FileSys::findNextPrime(int current) {
    if (current < MINPRIME) current = MINPRIME - 1;
    for (int i = current; i < MAXPRIME; i++) {
        for (int j = 2; j * j <= i; j++) {
            if (i % j == 0) 
                break;
            else if (j + 1 > sqrt(i) && i != current) {
                return i;
            }
        }
    }
    return MAXPRIME;
}