#include "versionComparator.h"

#include <iostream>

using namespace std;

bool SemanticVersionComparator::isNewer(const string& remote, const string& local) {
    auto remoteParts = splitVersion(remote);
    auto localParts = splitVersion(local);

    for (size_t i = 0; i < min(remoteParts.size(), localParts.size()); ++i) {
        if (remoteParts[i] > localParts[i]) return true;
        if (remoteParts[i] < localParts[i]) return false;
    }
    return remoteParts.size() > localParts.size();
}

vector<int> SemanticVersionComparator::splitVersion(const string& version) {
    vector<int> parts;
    size_t start = 0;
    size_t end = version.find('.');
    while (end != string::npos) {
        parts.push_back(stoi(version.substr(start, end - start)));
        start = end + 1;
        end = version.find('.', start);
    }
    parts.push_back(stoi(version.substr(start)));
    return parts;
}

bool TimestampVersionComparator::isNewer(const string& remote, const string& local) {
    try {
        // ���ַ���ת��Ϊʱ���
        long remoteTs = parseTimestamp(remote);
        long localTs = parseTimestamp(local);

        // ֱ�ӱȽ�ʱ�����ֵ
        return remoteTs > localTs;
    }
    catch (const invalid_argument&) {
        cerr << "Invalid timestamp format" << endl;
        return false;
    }
}

long TimestampVersionComparator::parseTimestamp(const string& timestampStr) {
    size_t pos;
    long timestamp = stol(timestampStr, &pos);

    // ��������ַ����Ƿ�������
    if (pos != timestampStr.length()) {
        throw invalid_argument("Invalid timestamp format");
    }
    return timestamp;
}