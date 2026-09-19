#pragma once

#include <QString>
#include <QStringList>

namespace hash_core {

// Exit codes returned by the command. They are part of the command-line
// contract.
constexpr int ExitSuccess = 0;
constexpr int ExitFailure = 1;
constexpr int ExitUsage = 2;
constexpr int ExitCanceled = 3;

// Error naming an unknown --format value.
inline const char kUnsupportedFormatMessage[] = "unsupported output format";

// Config is the parsed command line.
struct Config {
    QStringList algorithms;
    bool all = false;
    bool showSize = false;
    bool showModified = false;
    bool utc = false;
    int workers = 0;
    QString format = QStringLiteral("text");
    QString output;
    bool recursive = false;
    bool literal = false;
    bool failFast = false;
    QStringList paths;
};

// parseArgs interprets args. Usage and parse errors are reported through
// error; the caller prints the usage text.
bool parseArgs(const QStringList &args, Config *config, QString *error);

// usageText renders the flag summary printed on parse errors and -h.
QString usageText();

// selectAlgorithms resolves config into a validated, normalized algorithm
// list.
bool selectAlgorithms(const Config &config, QStringList *selected, QString *error);

} // namespace hash_core
