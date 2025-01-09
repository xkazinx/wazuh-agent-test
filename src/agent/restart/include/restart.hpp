#pragma once
#include <command_entry.hpp>

#include <boost/asio/awaitable.hpp>

#include <string>
#include <vector>

namespace restart
{
    /// @brief Restart class.
    class Restart
    {
    public:
        explicit Restart();

        static boost::asio::awaitable<module_command::CommandExecutionResult> HandleRestartCommand();
    };
} // namespace restart
