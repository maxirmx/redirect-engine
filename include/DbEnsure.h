#pragma once

#include <pqxx/pqxx>
#include <string>

class DbEnsure {
public:
    DbEnsure(pqxx::connection& c) : connection{c} { } ;
    DbEnsure(const DbEnsure&) = delete;
    DbEnsure& operator=(const DbEnsure&) = delete;

    void Ensure(void);

private:
    static const std::string sqlScript_0_2_0;

    pqxx::connection& connection;

    static std::string PuVersionUpdateQuery(const std::string& version);
    static std::string VQuery(const std::string& version);

    bool VCheck(const std::string& version);
    int Ensure_0_2_0(void);
    void PuVersionUpdate(const std::string& version);
    void EnsureVersion(const std::string& version, const std::string& statement);
};
