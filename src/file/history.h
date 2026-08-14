#pragma once

#include <vector>
#include <string>

using namespace std;

class History {
public:
    vector<string> snapshots;
    int current = -1;

    void push(const string& s);
    bool canUndo();
    bool canRedo();
    string undo();
    string redo();
};