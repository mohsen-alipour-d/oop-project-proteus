#include "file_manager.h"

#include <fstream>

#include "component_factory.h"

static vector<string> split(const string& s, char sep) {
    vector<string> out;
    string cur = "";
    for (char ch : s) {
        if (ch == sep) {
            if (cur != "")
                out.push_back(cur);
            cur = "";
        } else
            cur += ch;
    }
    if (cur != "")
        out.push_back(cur);
    return out;
}

static int pinIndex(Pin* p) {
    for (int i = 0; i < (int)p->owner->pins.size(); i++)
        if (&p->owner->pins[i] == p)
            return i;
    return 0;
}

static Component* findComponent(Circuit& c, const string& name) {
    for (Component* comp : c.components)
        if (comp->name == name)
            return comp;
    return nullptr;
}

string FileManager::serialize(Circuit& c) {
    string out = "";
    for (Component* comp : c.components)
        out += comp->serialize() + "\n";
    for (Wire* w : c.wires)
        out += "W " + w->startPin->owner->name + " " + to_string(pinIndex(w->startPin)) + " " + w->endPin->owner->name + " " + to_string(pinIndex(w->endPin)) + "\n";
    for (Junction* j : c.junctions) {
        out += "J " + to_string((int)j->position.x) + " " + to_string((int)j->position.y);
        for (Wire* w : j->wires) {
            for (int i = 0; i < (int)c.wires.size(); i++)
                if (c.wires[i] == w)
                    out += " " + to_string(i);
        }
        out += "\n";
    }
    return out;
}

void FileManager::deserialize(Circuit& c, const string& text) {
    c.clear();
    vector<string> lines = split(text, '\n');
    vector<vector<string>> wireLines;
    vector<vector<string>> juncLines;
    for (string& line : lines) {
        if (line == "")
            continue;
        vector<string> t = split(line, ' ');
        if (t.empty())
            continue;
        if (t[0] == "W")
            wireLines.push_back(t);
        else if (t[0] == "J")
            juncLines.push_back(t);
        else {
            try {
                Component* comp = buildComponent(t);
                if (comp)
                    c.addComponent(comp);
            } catch (...) {
            }
        }
    }
    for (vector<string>& t : wireLines) {
        try {
            Component* a = findComponent(c, t[1]);
            Component* b = findComponent(c, t[3]);
            int ai = stoi(t[2]);
            int bi = stoi(t[4]);
            if (a && b && ai >= 0 && ai < (int)a->pins.size() && bi >= 0 && bi < (int)b->pins.size())
                c.addWire(&a->pins[ai], &b->pins[bi]);
        } catch (...) {
        }
    }
    for (vector<string>& t : juncLines) {
        try {
            Junction* j = new Junction(stod(t[1]), stod(t[2]));
            for (int k = 3; k < (int)t.size(); k++) {
                int wi = stoi(t[k]);
                if (wi >= 0 && wi < (int)c.wires.size())
                    j->addWire(c.wires[wi]);
            }
            if ((int)j->wires.size() >= 2)
                c.junctions.push_back(j);
            else
                delete j;
        } catch (...) {
        }
    }
    c.rebuildNets();
}

bool FileManager::save(Circuit& c) {
    if (!saved || filePath == "")
        return false;
    ofstream f(filePath);
    if (!f.is_open())
        return false;
    f << serialize(c);
    return true;
}

bool FileManager::saveAs(Circuit& c, const string& path) {
    ofstream f(path);
    if (!f.is_open())
        return false;
    f << serialize(c);
    filePath = path;
    saved = true;
    projectName = path;
    pushRecent(path);
    return true;
}

bool FileManager::load(Circuit& c, const string& path) {
    ifstream f(path);
    if (!f.is_open())
        return false;
    string text = "";
    string line;
    while (getline(f, line))
        text += line + "\n";
    deserialize(c, text);
    filePath = path;
    saved = true;
    projectName = path;
    pushRecent(path);
    return true;
}

void FileManager::loadRecentList() {
    recentProjects.clear();
    ifstream f("recent_projects.txt");
    string line;
    while (getline(f, line))
        if (line != "")
            recentProjects.push_back(line);
}

void FileManager::pushRecent(const string& entry) {
    loadRecentList();
    vector<string> fresh;
    fresh.push_back(entry);
    for (string& s : recentProjects)
        if (s != entry && (int)fresh.size() < 5)
            fresh.push_back(s);
    recentProjects = fresh;
    ofstream f("recent_projects.txt");
    for (string& s : recentProjects)
        f << s << "\n";
}