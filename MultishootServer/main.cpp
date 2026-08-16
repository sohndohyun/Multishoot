#include "game/multi_shoot.hpp"
#include "network/packet.hpp"

#include <Windows.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>

namespace {

void print_usage(const char* executable) {
    std::fprintf(
        stderr,
        "Usage: %s [port] [--port N] [--db-host HOST] [--db-port N] "
        "[--db-user USER] [--db-password PASSWORD] [--db-name NAME]\n",
        executable);
}

bool parse_unsigned(const char* value, unsigned int& result) {
    if (!value || *value == '\0')
        return false;
    char* end = nullptr;
    const auto parsed = std::strtoul(value, &end, 10);
    if (*end != '\0' || parsed > 65535)
        return false;
    result = static_cast<unsigned int>(parsed);
    return true;
}

bool parse_arguments(int argc, char* argv[], int& port, database_config& database,
                     std::string& error) {
    bool positional_port_seen = false;
    for (int index = 1; index < argc; ++index) {
        const std::string option = argv[index];
        auto next_value = [&](const char* name, std::string& target) {
            if (index + 1 >= argc) {
                error = std::string("missing value for ") + name;
                return false;
            }
            target = argv[++index];
            return true;
        };

        if (option == "--port") {
            unsigned int value = 0;
            if (index + 1 >= argc || !parse_unsigned(argv[++index], value) || value == 0) {
                error = "invalid --port";
                return false;
            }
            port = static_cast<int>(value);
        } else if (option == "--db-host") {
            if (!next_value("--db-host", database.host))
                return false;
        } else if (option == "--db-port") {
            unsigned int value = 0;
            if (index + 1 >= argc || !parse_unsigned(argv[++index], value) || value == 0) {
                error = "invalid --db-port";
                return false;
            }
            database.port = value;
        } else if (option == "--db-user") {
            if (!next_value("--db-user", database.user))
                return false;
        } else if (option == "--db-password") {
            if (!next_value("--db-password", database.password))
                return false;
        } else if (option == "--db-name") {
            if (!next_value("--db-name", database.name))
                return false;
        } else if (!positional_port_seen && index == 1) {
            unsigned int value = 0;
            if (!parse_unsigned(argv[index], value) || value == 0) {
                error = "invalid positional port";
                return false;
            }
            port = static_cast<int>(value);
            positional_port_seen = true;
        } else {
            error = "unknown argument: " + option;
            return false;
        }
    }
    return true;
}

} // namespace

int main(int argc, char* argv[]) {
    char message[dr::packet_capacity];
    database_config database;
    int port = 3000;
    std::string error;
    if (!parse_arguments(argc, argv, port, database, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        print_usage(argv[0]);
        return 2;
    }

    multi_shoot server(std::move(database));
    if (!server.start_database(error)) {
        std::fprintf(stderr, "database initialization failed: %s\n", error.c_str());
        return 1;
    }

    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    int result = server.init(port, system_info.dwNumberOfProcessors);
    if (result != 0) {
        fprintf(stderr, "server initialization failed: %d\n", result);
        return result;
    }

    fputs("input message to quit: ", stdout);
    server.start();

    fgets(message, dr::packet_capacity, stdin);
    server.end();

    return 0;
}
