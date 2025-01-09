#include <boost/asio/awaitable.hpp>
#include <logger.hpp>
#include <restart.hpp>

namespace restart
{

    boost::asio::awaitable<module_command::CommandExecutionResult> Restart::HandleRestartCommand()
    {
        // TODO
        co_return module_command::CommandExecutionResult {module_command::Status::FAILURE,
                                                          "Restart is not implemented yet"};
    }

} // namespace restart
