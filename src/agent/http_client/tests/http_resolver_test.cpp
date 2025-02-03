#include "../src/http_resolver.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>

class HttpResolverTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_ioContext = std::make_unique<boost::asio::io_context>();
        m_resolver = std::make_unique<http_client::HttpResolver>(m_ioContext->get_executor());
    }

    void TearDown() override
    {
        m_resolver.reset();
        m_ioContext.reset();
    }

    std::unique_ptr<boost::asio::io_context> m_ioContext;
    std::unique_ptr<http_client::HttpResolver> m_resolver;
};

TEST_F(HttpResolverTest, SyncResolveValidHostAndPort)
{
    const std::string host = "www.google.com";
    const std::string port = "80";

    auto results = m_resolver->Resolve(host, port);

    EXPECT_FALSE(results.empty());
}

TEST_F(HttpResolverTest, SyncResolveInvalidHost)
{
    const std::string host = "invalid.host.that.does.not.exist";
    const std::string port = "80";

    auto results = m_resolver->Resolve(host, port);
    EXPECT_TRUE(results.empty());
}

TEST_F(HttpResolverTest, AsyncResolveValidHostAndPort)
{
    const std::string host = "www.google.com";
    const std::string port = "80";

    auto resolve_coroutine = [this, &host, &port]() // NOLINT(cppcoreguidelines-avoid-capturing-lambda-coroutines)
        -> boost::asio::awaitable<void>
    {
        auto results = co_await m_resolver->AsyncResolve(host, port);

        EXPECT_FALSE(results.empty());
    };

    boost::asio::co_spawn(m_ioContext->get_executor(), resolve_coroutine, boost::asio::detached);
    m_ioContext->run();
}

TEST_F(HttpResolverTest, AsyncResolveInvalidHost)
{
    const std::string host = "invalid.host.that.does.not.exist";
    const std::string port = "80";

    auto resolve_coroutine = [this, &host, &port]() // NOLINT(cppcoreguidelines-avoid-capturing-lambda-coroutines)
        -> boost::asio::awaitable<void>
    {
        auto results = co_await m_resolver->AsyncResolve(host, port);

        EXPECT_TRUE(results.empty());
    };

    boost::asio::co_spawn(m_ioContext->get_executor(), resolve_coroutine, boost::asio::detached);
    m_ioContext->run();
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
