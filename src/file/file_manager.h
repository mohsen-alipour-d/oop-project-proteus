#pragma once

#include <string>
#include <vector>

#include "../core/circuit.h"

using namespace std;

class FileManager {
public:
    string projectName;
    string filePath;
    bool saved = false;
    vector<string> recentProjects;

    bool save(Circuit& c);
    bool saveAs(Circuit& c, const string& path);
    bool load(Circuit& c, const string& path);

    string serialize(Circuit& c);
    void deserialize(Circuit& c, const string& text);

    void loadRecentList();
    void pushRecent(const string& entry);
};