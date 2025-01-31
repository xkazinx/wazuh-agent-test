#include <gtest/gtest.h>

#include "../src/http_socket.hpp"

#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>

class HttpSocketTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Set up the io_context for the socket
        m_ioContext = std::make_unique<boost::asio::io_context>();
        m_socket = std::make_unique<http_client::HttpSocket>(m_ioContext->get_executor());
    }

    void TearDown() override
    {
        m_socket.reset();
        m_ioContext.reset();
    }

    std::unique_ptr<boost::asio::io_context> m_ioContext;
    std::unique_ptr<http_client::HttpSocket> m_socket;
};

TEST_F(HttpSocketTest, SyncConnectValidEndpoint)
{
    boost::asio::ip::tcp::resolver resolver(m_ioContext->get_executor());
    auto endpoints = resolver.resolve("www.google.com", "80");

    boost::system::error_code ec;
    m_socket->Connect(endpoints, ec);
    m_ioContext->run();

    EXPECT_FALSE(ec);
}

TEST_F(HttpSocketTest, SyncConnectInvalidEndpoint)
{
    boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp> endpoints;

    boost::system::error_code ec;

    m_socket->Connect(endpoints, ec);
    m_ioContext->run();

    EXPECT_TRUE(ec);
}

TEST_F(HttpSocketTest, AsyncConnectValidEndpoint)
{
    boost::asio::ip::tcp::resolver resolver(m_ioContext->get_executor());
    auto endpoints = resolver.resolve("www.google.com", "80");

    boost::system::error_code ec;

    auto connect_coroutine =
        [&]() -> boost::asio::awaitable<void> // NOLINT(cppcoreguidelines-avoid-capturing-lambda-coroutines)
    {
        co_await m_socket->AsyncConnect(endpoints, ec);
    };

    boost::asio::co_spawn(m_ioContext->get_executor(), connect_coroutine, boost::asio::detached);
    m_ioContext->run();

    EXPECT_FALSE(ec);
}

TEST_F(HttpSocketTest, AsyncConnectInvalidEndpoint)
{
    boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp> endpoints;
    boost::system::error_code ec;

    auto connect_coroutine =
        [&]() -> boost::asio::awaitable<void> // NOLINT(cppcoreguidelines-avoid-capturing-lambda-coroutines)
    {
        try
        {
            co_await m_socket->AsyncConnect(endpoints, ec);
        }
        catch (std::exception& ex)
        {
            ec = boost::asio::error::operation_aborted;
        }
    };

    boost::asio::co_spawn(m_ioContext->get_executor(), connect_coroutine, boost::asio::detached);
    m_ioContext->run();
}

TEST_F(HttpSocketTest, SyncWrite)
{
    boost::asio::ip::tcp::resolver resolver(m_ioContext->get_executor());
    auto endpoints = resolver.resolve("www.google.com", "80");

    boost::system::error_code ec;
    m_socket->Connect(endpoints, ec);

    boost::beast::http::request<boost::beast::http::string_body> req {
        boost::beast::http::verb::get, "/", 11}; // NOLINT (avoid-magic-numbers)
    req.set(boost::beast::http::field::host, "www.google.com");

    m_socket->Write(req, ec);
    EXPECT_FALSE(ec);
}

TEST_F(HttpSocketTest, AsyncWrite)
{
    boost::asio::ip::tcp::resolver resolver(m_ioContext->get_executor());
    auto endpoints = resolver.resolve("www.google.com", "80");

    boost::system::error_code ec;

    auto connect_coroutine =
        [&]() -> boost::asio::awaitable<void> // NOLINT(cppcoreguidelines-avoid-capturing-lambda-coroutines)
    {
        co_await m_socket->AsyncConnect(endpoints, ec);
        if (ec)
            co_return;

        boost::beast::http::request<boost::beast::http::string_body> req {
            boost::beast::http::verb::get, "/", 11}; // NOLINT (avoid-magic-numbers)
        req.set(boost::beast::http::field::host, "www.google.com");

        co_await m_socket->AsyncWrite(req, ec);
    };

    boost::asio::co_spawn(m_ioContext->get_executor(), connect_coroutine, boost::asio::detached);
    m_ioContext->run();

    EXPECT_FALSE(ec);
}

TEST_F(HttpSocketTest, SyncRead)
{
    boost::asio::ip::tcp::resolver resolver(m_ioContext->get_executor());
    auto endpoints = resolver.resolve("www.google.com", "80");

    boost::system::error_code ec;
    m_socket->Connect(endpoints, ec);

    boost::beast::http::request<boost::beast::http::string_body> req {
        boost::beast::http::verb::get, "/", 11}; // NOLINT (avoid-magic-numbers)
    req.set(boost::beast::http::field::host, "www.google.com");

    m_socket->Write(req, ec);
    ASSERT_FALSE(ec); // Ensure write is successful

    boost::beast::http::response<boost::beast::http::dynamic_body> res;
    m_socket->Read(res, ec);
    EXPECT_FALSE(ec);
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
}

TEST_F(HttpSocketTest, AsyncRead)
{
    boost::asio::ip::tcp::resolver resolver(m_ioContext->get_executor());
    auto endpoints = resolver.resolve("www.google.com", "80");

    boost::system::error_code ec;

    auto read_coroutine =
        [&]() -> boost::asio::awaitable<void> // NOLINT(cppcoreguidelines-avoid-capturing-lambda-coroutines)
    {
        co_await m_socket->AsyncConnect(endpoints, ec);
        if (ec)
            co_return;

        boost::beast::http::request<boost::beast::http::string_body> req {
            boost::beast::http::verb::get, "/", 11}; // NOLINT (avoid-magic-numbers)
        req.set(boost::beast::http::field::host, "www.google.com");

        co_await m_socket->AsyncWrite(req, ec);
        if (ec)
            co_return;

        boost::beast::http::response<boost::beast::http::dynamic_body> res;
        co_await m_socket->AsyncRead(res, ec);
        EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    };

    boost::asio::co_spawn(m_ioContext->get_executor(), read_coroutine, boost::asio::detached);
    m_ioContext->run();

    EXPECT_FALSE(ec);
}

TEST_F(HttpSocketTest, CloseSocket)
{
    boost::asio::ip::tcp::resolver resolver(m_ioContext->get_executor());
    auto endpoints = resolver.resolve("www.google.com", "80");

    boost::system::error_code ec;
    m_socket->Connect(endpoints, ec);

    m_socket->Close();
    EXPECT_FALSE(m_socket->IsOpen());
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
