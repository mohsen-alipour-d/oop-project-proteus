#include "log_buffer.h"

void LogBuffer::clear() {
    messages.clear();
}

void LogBuffer::info(const string& msg) {
    LogMessage m;
    m.text = msg;
    m.isError = false;
    messages.push_back(m);
}

void LogBuffer::error(const string& msg) {
    LogMessage m;
    m.text = msg;
    m.isError = true;
    messages.push_back(m);
}

void LogBuffer::infoAt(const string& msg, double t) {
    LogMessage m;
    m.text = msg;
    m.isError = false;
    m.timestamp = t;
    messages.push_back(m);
}

void LogBuffer::errorAt(const string& msg, double t) {
    LogMessage m;
    m.text = msg;
    m.isError = true;
    m.timestamp = t;
    messages.push_back(m);
}