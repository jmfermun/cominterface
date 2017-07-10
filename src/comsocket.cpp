/**
 * @file    comsocket.cpp
 * @author  Juan Manuel Fernández Muñoz
 * @date    January, 2017
 * @brief   TCP/IP Socket communication interface implementation.
 */

// Under Windows XP, Windows Server 2003 or older, this class
// must be compiled with the flag BOOST_ASIO_ENABLE_CANCELIO.
// This allows to cancel the asynchronous operations.
#define BOOST_ASIO_ENABLE_CANCELIO

#include <boost/bind.hpp>
#include <boost/chrono.hpp>

#include "cominterface/comsocket.h"

////////////////////
// Public Methods //
////////////////////

ComSocket::ComSocket(const std::string& address, unsigned int port,
                     unsigned int timeout):
                       m_io_service(), m_socket(m_io_service),
                       m_timer(m_io_service), m_acceptor(m_io_service)
{
    if (!SetAddress(address))
        throw std::invalid_argument("invalid IP address");

    if (!SetPort(port))
        throw std::invalid_argument("invalid TCP port");

    if (!SetOpenTimeout(timeout) ||
        !SetWriteTimeout(timeout) ||
        !SetReadTimeout(timeout))
        throw std::invalid_argument("invalid timeout value");
}

ComSocket::~ComSocket()
{

}

bool ComSocket::Open()
{
    boost::system::error_code ec;
    bool ret_code = false;

    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    // Due to previous cancellation of operations on io_service, it is
    // necessary to reset it
    m_socket.get_io_service().reset();

    // If the socket is already opened, close it
    if (m_socket.is_open())
        m_socket.close(ec);

    // If the IP address is unspecified, the mode is server.
    // Else, the mode is client
    if (m_address.is_unspecified())
    {
        // Set the timeout for the asynchronous operations
        m_timer.expires_from_now(m_open_timeout);
        m_timer.async_wait(boost::bind(&ComSocket::timeout_accept_handler, this,
                                       boost::asio::placeholders::error));

        // Connection endpoint
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), m_port);

        // Connection acceptor
        try
        {
            m_acceptor.open(endpoint.protocol());
            m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
            m_acceptor.bind(endpoint);
            m_acceptor.listen();
        }
        catch (std::exception &e)
        {
            m_acceptor.close(ec);
            return false;
        }

        // Start the asynchronous operation (accept)
        m_acceptor.async_accept(m_socket, boost::bind(&ComSocket::open_handler,
                                                    this, _1, &ret_code));

        // Wait until the asynchronous operations are completed
        m_acceptor.get_io_service().run(ec);

        // Close the acceptance of new connections
        boost::system::error_code ec_acceptor_close;
        m_acceptor.close(ec_acceptor_close);

        // There is no matter of the error code. If an error occurs, the return
        // code will be false
        if (ec_acceptor_close)
            ec = ec_acceptor_close;
    }
    else
    {
        // Set the timeout for the asynchronous operations
        m_timer.expires_from_now(m_open_timeout);
        m_timer.async_wait(boost::bind(&ComSocket::timeout_handler, this,
                                       boost::asio::placeholders::error));

        // Connection endpoint
        boost::asio::ip::tcp::endpoint endpoint(m_address, m_port);

        // Start the asynchronous operation (connect)
        m_socket.async_connect(endpoint, boost::bind(&ComSocket::open_handler,
                                                     this, _1, &ret_code));

        // Wait until the asynchronous operations are completed
        m_socket.get_io_service().run(ec);
    }

    if (ec)
        ret_code = false;
    else
    {
        // Set the socket synchronous operations to non blocking mode
        m_socket.non_blocking(true, ec);

        // If the operation fails, close the socket and return false
        if (ec)
        {
            m_socket.close(ec);
            ret_code = false;
        }
    }

    return ret_code;
}

bool ComSocket::Close()
{
    boost::system::error_code ec;

    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    // If the socket is opened, close it
    if (m_socket.is_open())
        m_socket.close(ec);

    // Error?
    if (ec)
        return false;

    return true;
}

bool ComSocket::Opened()
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    return m_socket.is_open();
}

int ComSocket::ReadSome(void *buffer_in, size_t len)
{
    boost::system::error_code ec;
    int ret_code;

    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    // Make a non blocking read
    ret_code = m_socket.read_some(boost::asio::buffer(buffer_in, len), ec);

    // If error would_block occurs, there is no data available. Else,
    // an unexpected error occurs
    if (ec)
    {
        if (ec == boost::asio::error::would_block)
            ret_code = 0;
        else
            ret_code = -1;
    }

    return ret_code;
}

int ComSocket::WriteSome(const void *buffer_out, size_t len)
{
    boost::system::error_code ec;
    int ret_code;

    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    // Make a non blocking write
    ret_code = m_socket.write_some(boost::asio::buffer(buffer_out, len), ec);

    // If error would_block occurs, the operation can't be made. Else,
    // an unexpected error occurs
    if (ec)
    {
        if (ec == boost::asio::error::would_block)
            ret_code = 0;
        else
            ret_code = -1;
    }

    return ret_code;
}

int ComSocket::Read(void *buffer_in, size_t len)
{
    boost::system::error_code ec;
    int ret_code;

    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    // Due to previous cancellation of operations on io_service, it is
    // necessary to reset it
    m_socket.get_io_service().reset();

    // Set the timeout for the asynchronous operations
    m_timer.expires_from_now(m_read_timeout);
    m_timer.async_wait(boost::bind(&ComSocket::timeout_handler, this,
                                   boost::asio::placeholders::error));

    // Start the asynchronous operation (blocking read)
    boost::asio::async_read(m_socket, boost::asio::buffer(buffer_in, len),
                            boost::bind(&ComSocket::read_write_handler, this,
                                        _1, _2, &ret_code));

    // Wait until the asynchronous operations are completed
    m_socket.get_io_service().run(ec);

    if (ec)
        ret_code = -1;

    return ret_code;
}

int ComSocket::Write(const void *buffer_out, size_t len)
{
    boost::system::error_code ec;
    int ret_code;

    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    // Due to previous cancellation of operations on io_service, it is
    // necessary to reset it
    m_socket.get_io_service().reset();

    // Set the timeout for the asynchronous operations
    m_timer.expires_from_now(m_write_timeout);
    m_timer.async_wait(boost::bind(&ComSocket::timeout_handler, this,
                                   boost::asio::placeholders::error));

    // Start the asynchronous operation (blocking write)
    boost::asio::async_write(m_socket, boost::asio::buffer(buffer_out, len),
                            boost::bind(&ComSocket::read_write_handler, this,
                                        _1, _2, &ret_code));

    // Wait until the asynchronous operations are completed
    m_socket.get_io_service().run(ec);

    if (ec)
        ret_code = -1;

    return ret_code;
}

void ComSocket::Abort()
{
    boost::system::error_code ec;

    // Cancel the timeout timer asynchronous operations
    m_timer.cancel(ec);

    // Cancel the socket asynchronous operations
    m_socket.cancel(ec);

    // In Windows Server 2003, Windows XP and older, cancel don't work
    // if it is called from a thread different from the thread that calls
    // the asynchronous operation
    if (ec == boost::asio::error::operation_not_supported)
        m_socket.close(ec);

    // Cancel the acceptor asynchronous operations
    m_acceptor.cancel(ec);

    if (ec == boost::asio::error::operation_not_supported)
        m_acceptor.close(ec);
}

bool ComSocket::SetWriteTimeout(unsigned int write_timeout)
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    if (write_timeout == 0)
        return false;

    m_write_timeout = boost::posix_time::milliseconds(write_timeout);

    return true;
}

unsigned int ComSocket::GetWriteTimeout()
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    return m_write_timeout.total_milliseconds();
}

bool ComSocket::SetReadTimeout(unsigned int read_timeout)
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    if (read_timeout == 0)
        return false;

    m_read_timeout = boost::posix_time::milliseconds(read_timeout);

    return true;
}

unsigned int ComSocket::GetReadTimeout()
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    return m_read_timeout.total_milliseconds();
}

bool ComSocket::SetOpenTimeout(unsigned int open_timeout)
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    if (open_timeout == 0)
        return false;

    m_open_timeout = boost::posix_time::milliseconds(open_timeout);

    return true;
}

unsigned int ComSocket::GetOpenTimeout()
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    return m_open_timeout.total_milliseconds();
}

bool ComSocket::SetAddress(const std::string& address)
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    boost::system::error_code ec;

    if (address.empty())
        m_address = boost::asio::ip::address();
    else
        m_address = boost::asio::ip::address::from_string(address, ec);

    if (ec)
        return false;
    else
        return true;
}

std::string ComSocket::GetAddress()
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    if (m_address.is_unspecified())
        return std::string();
    else
        return m_address.to_string();
}

bool ComSocket::SetPort(unsigned int port)
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    if (port > 65535)
        return false;

    m_port = port;

    return true;
}

unsigned int ComSocket::GetPort()
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    return m_port;
}

//////////////////////
// Private Methods //
//////////////////////

void ComSocket::open_handler(const boost::system::error_code &error, bool *ret_code)
{
    boost::system::error_code ec;

    // If an error is present and the operation hasn't been canceled,
    // an unhandled error occurred
    if (error && error != boost::asio::error::operation_aborted)
        *ret_code = false;
    else
        *ret_code = true;

    // Cancel the timeout timer
    m_timer.cancel(ec);
}

void ComSocket::read_write_handler(const boost::system::error_code &error,
                                   size_t bytes_transferred, int *ret_code)
{
    boost::system::error_code ec;

    // If an error is present and the operation hasn't been canceled,
    // an unhandled error occurred
    if (error && error != boost::asio::error::operation_aborted)
        *ret_code = -1;
    else
        *ret_code = bytes_transferred;

    // Cancel the timeout timer
    m_timer.cancel(ec);
}

void ComSocket::timeout_handler(const boost::system::error_code &error)
{
    boost::system::error_code ec;

    // If the timeout timer has been canceled, the operation
    // finished correctly
    if (error == boost::asio::error::operation_aborted)
        return;

    // If the timeout timer expired, cancel the socket operation
    m_socket.cancel(ec);
}

void ComSocket::timeout_accept_handler(const boost::system::error_code &error)
{
    boost::system::error_code ec;

    // If the timeout timer has been canceled, the operation
    // finished correctly
    if (error == boost::asio::error::operation_aborted)
        return;

    // If the timeout timer expired, cancel the socket operation
    m_acceptor.cancel(ec);
}
