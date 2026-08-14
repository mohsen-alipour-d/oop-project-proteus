#pragma once

#include <vector>
#include <string>

using namespace std;

struct LogMessage {
    string text;
    bool isError = false;
    double timestamp = 0;
};

class LogBuffer {
public:
    vector<LogMessage> messages;

    void clear();
    void info(const string& msg);
    void error(const string& msg);
    void infoAt(const string& msg, double t);
    void errorAt(const string& msg, double t);
};