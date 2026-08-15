#include "history.h"

void History::push(const string& s) {
    while ((int)snapshots.size() > current + 1)
        snapshots.pop_back();
    snapshots.push_back(s);
    current = (int)snapshots.size() - 1;
    while ((int)snapshots.size() > maxSnapshots) {
        snapshots.erase(snapshots.begin());
        current--;
    }
}

bool History::canUndo() {
    return current > 0;
}

bool History::canRedo() {
    return current < (int)snapshots.size() - 1;
}

string History::undo() {
    if (!canUndo())
        return "";
    current--;
    return snapshots[current];
}

string History::redo() {
    if (!canRedo())
        return "";
    current++;
    return snapshots[current];
}