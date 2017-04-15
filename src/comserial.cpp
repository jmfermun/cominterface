/**
 * @file    comserial.cpp
 * @author  Juan Manuel Fernández Muñoz
 * @date    January, 2017
 * @brief   Serial Port communication interface implementation
 */

#if !defined(BOOST_WINDOWS) && !defined(__CYGWIN__)
#include <sys/file.h>
#endif

#include <stdexcept>

#include <boost/bind.hpp>
#include <boost/chrono.hpp>

#include "comserial.h"

////////////////////
// Public Methods //
////////////////////

ComSerial::ComSerial(const std::string& device, unsigned int baud_rate,
                     unsigned int data_bits, unsigned int stop_bits,
                     char parity, char flow_control, unsigned int timeout):
                         m_io_service(), m_port(m_io_service), m_timer(m_io_service)
{
    if (!SetDevice(device))
        throw std::invalid_argument("invalid device name");

    if (!SetBaudRate(baud_rate))
        throw std::invalid_argument("invalid baud rate");

    if (!SetDataBits(data_bits))
        throw std::invalid_argument("invalid data bits value");

    if (!SetStopBits(stop_bits))
        throw std::invalid_argument("invalid stop bits value");

    if (!SetParity(parity))
        throw std::invalid_argument("invalid parity value");

    if (!SetFlowControl(flow_control))
        throw std::invalid_argument("invalid flow control value");

    if (!SetWriteTimeout(timeout) || !SetReadTimeout(timeout))
        throw std::invalid_argument("invalid timeout value");
}

ComSerial::~ComSerial()
{

}

bool ComSerial::Open()
{
    boost::system::error_code ec;

    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    // If the serial port is already opened, close it
    if (m_port.is_open())
        m_port.close(ec);

    // Open the serial port
    m_port.open(m_device, ec);

    // Error at opening?
    if (ec)
        return false;

#if !defined(BOOST_WINDOWS) && !defined(__CYGWIN__)
    // In Linux, it is necessary to set exclusive access to the serial port
    if (0 != ::ioctl(m_port.lowest_layer().native_handle(), TIOCEXCL) ||
        0 != ::flock(m_port.lowest_layer().native_handle(), LOCK_EX | LOCK_NB))
    {
        m_port.close(ec);
        return false;
    }
#endif

    // Set the serial port configuration
    try
    {
        m_port.set_option(m_baud_rate);
        m_port.set_option(m_data_bits);
        m_port.set_option(m_stop_bits);
        m_port.set_option(m_parity);
        m_port.set_option(m_flow_control);
    }
    catch (std::exception &e)
    {
        m_port.close(ec);
        return false;
    }

    return true;
}

bool ComSerial::Close()
{
    boost::system::error_code ec;

    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    // If the serial port is opened, close it
    if (m_port.is_open())
        m_port.close(ec);

    // Error at closing?
    if (ec)
        return false;

    return true;
}

bool ComSerial::Opened()
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    return m_port.is_open();
}

int ComSerial::ReadSome(void *buffer_in, size_t len)
{
    boost::system::error_code ec;
    int ret_code;

    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    // Get the number of available bytes in the kernel read buffer
    ret_code = available_for_read();

    // If the number of available bytes is 0, return this value.
    // If an error occurred, return the error code (-1)
    if (ret_code <= 0)
        return ret_code;

    // Make a synchronous read
    ret_code = m_port.read_some(boost::asio::buffer(buffer_in, len), ec);

    if (ec)
        ret_code = -1;

    return ret_code;
}

int ComSerial::WriteSome(const void *buffer_out, size_t len)
{
    boost::system::error_code ec;
    int ret_code;

    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    // Get the number of remaining bytes in the kernel write buffer
    ret_code = pending_for_write();

    // If the number of remaining bytes is greater than 0, return 0.
    // If an error occurred, return the error code (-1)
    if (ret_code != 0)
    {
        if (ret_code > 0)
            return 0;
        else
            return -1;
    }

    // Make a synchronous write
    ret_code = m_port.write_some(boost::asio::buffer(buffer_out, len), ec);

    if (ec)
        ret_code = -1;

    return ret_code;
}

int ComSerial::Read(void *buffer_in, size_t len)
{
    boost::system::error_code ec;
    int ret_code;

    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    // Due to previous cancellation of operations on io_service, it is
    // necessary to reset it
    m_port.get_io_service().reset();

    // Set the timeout for the asynchronous operations
    m_timer.expires_from_now(m_read_timeout);
    m_timer.async_wait(boost::bind(&ComSerial::timeout_handler, this,
                                   boost::asio::placeholders::error));

    // Start the asynchronous operation (blocking read)
    boost::asio::async_read(m_port, boost::asio::buffer(buffer_in, len),
                            boost::bind(&ComSerial::read_write_handler, this,
                                        _1, _2, &ret_code));

    // Wait until the asynchronous operations are completed
    m_port.get_io_service().run(ec);

    if (ec)
        ret_code = -1;

    return ret_code;
}

int ComSerial::Write(const void *buffer_out, size_t len)
{
    boost::system::error_code ec;
    int ret_code;

    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    // Due to previous cancellation of operations on io_service, it is
    // necessary to reset it
    m_port.get_io_service().reset();

    // Set the timeout for the asynchronous operations
    m_timer.expires_from_now(m_write_timeout);
    m_timer.async_wait(boost::bind(&ComSerial::timeout_handler, this,
                                   boost::asio::placeholders::error));

    // Start the asynchronous operation (blocking write)
    boost::asio::async_write(m_port, boost::asio::buffer(buffer_out, len),
                            boost::bind(&ComSerial::read_write_handler, this,
                                        _1, _2, &ret_code));

    // Wait until the asynchronous operations are completed
    m_port.get_io_service().run(ec);

    if (ec)
        ret_code = -1;

    return ret_code;
}

void ComSerial::Abort()
{
    boost::system::error_code ec;

    // Cancel the timeout timer asynchronous operations
    m_timer.cancel(ec);

    // Cancel the serial port asynchronous operations
    m_port.cancel(ec);
}

bool ComSerial::SetWriteTimeout(unsigned int write_timeout)
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    if (write_timeout == 0)
        return false;

    m_write_timeout = boost::posix_time::milliseconds(write_timeout);

    return true;
}

unsigned int ComSerial::GetWriteTimeout()
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    return m_write_timeout.total_milliseconds();
}

bool ComSerial::SetReadTimeout(unsigned int read_timeout)
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    if (read_timeout == 0)
        return false;

    m_read_timeout = boost::posix_time::milliseconds(read_timeout);

    return true;
}

unsigned int ComSerial::GetReadTimeout()
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    return m_read_timeout.total_milliseconds();
}

bool ComSerial::SetDevice(const std::string& device)
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    if (device.empty())
        return false;

    m_device = device;

    return true;
}

std::string ComSerial::GetDevice()
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    return m_device;
}

bool ComSerial::SetBaudRate(unsigned int baud_rate)
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    if (baud_rate == 0)
        return false;

    try
    {
        m_baud_rate = boost::asio::serial_port_base::baud_rate(baud_rate);
    }
    catch (...)
    {
        return false;
    }

    return true;
}

unsigned int ComSerial::GetBaudRate()
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    return m_baud_rate.value();
}

bool ComSerial::SetDataBits(unsigned int data_bits)
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    try
    {
        m_data_bits = boost::asio::serial_port_base::character_size(data_bits);
    }
    catch (...)
    {
        return false;
    }

    return true;
}

unsigned int ComSerial::GetDataBits()
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    return m_data_bits.value();
}

bool ComSerial::SetStopBits(unsigned int stop_bits)
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    try
    {
        switch (stop_bits)
        {
        case 1:
            m_stop_bits = boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one);
            break;

        case 2:
            m_stop_bits = boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::two);
            break;

        case 3:
            m_stop_bits = boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::onepointfive);
            break;

        default:
            return false;
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}

unsigned int ComSerial::GetStopBits()
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    unsigned int ret_code = 0;

    switch (m_stop_bits.value())
    {
    case boost::asio::serial_port_base::stop_bits::one:
        ret_code = 1;
        break;

    case boost::asio::serial_port_base::stop_bits::two:
        ret_code = 2;
        break;

    case boost::asio::serial_port_base::stop_bits::onepointfive:
        ret_code = 3;
        break;

    default:
        ret_code = 0;
    }

    return ret_code;
}

bool ComSerial::SetParity(char parity)
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    try
    {
        switch (parity)
        {
        case 'e':
        case 'E':
            m_parity = boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::even);
            break;

        case 'o':
        case 'O':
            m_parity = boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::odd);
            break;

        case 'n':
        case 'N':
            m_parity = boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none);
            break;

        default:
            return false;
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}

char ComSerial::GetParity()
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    char ret_code = '\0';

    switch (m_parity.value())
    {
    case boost::asio::serial_port_base::parity::even:
        ret_code = 'e';
        break;

    case boost::asio::serial_port_base::parity::odd:
        ret_code = 'o';
        break;

    case boost::asio::serial_port_base::parity::none:
        ret_code = 'n';
        break;

    default:
        ret_code = '\0';
    }

    return ret_code;
}

bool ComSerial::SetFlowControl(char flow_control)
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    switch (flow_control)
    {
    case 'h':
    case 'H':
        m_flow_control = boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::hardware);
        break;

    case 's':
    case 'S':
        m_flow_control = boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::software);
        break;

    case 'n':
    case 'N':
        m_flow_control = boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none);
        break;

    default:
        return false;
    }

    return true;
}

char ComSerial::GetFlowControl()
{
    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

    char ret_code = '\0';

    switch (m_flow_control.value())
    {
    case boost::asio::serial_port_base::flow_control::hardware:
        ret_code = 'h';
        break;

    case boost::asio::serial_port_base::flow_control::software:
        ret_code = 's';
        break;

    case boost::asio::serial_port_base::flow_control::none:
        ret_code = 'n';
        break;

    default:
        ret_code = '\0';
    }

    return ret_code;
}

bool ComSerial::Flush()
{
    bool ok;

    // Lock for thread safe
    boost::lock_guard<boost::mutex> lock(m_mutex);

#if defined(BOOST_WINDOWS) || defined(__CYGWIN__)
    ok = (::PurgeComm(m_port.lowest_layer().native_handle(), PURGE_RXABORT |
                      PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR) != 0);
#else
    ok = (::tcflush(m_port.lowest_layer().native_handle(), TCIOFLUSH) == 0);
#endif

//    // Error?
//    if (!ok)
//    {
//        boost::system::error_code ec;
//
//#if defined(BOOST_WINDOWS) || defined(__CYGWIN__)
//        ec = boost::system::error_code(::GetLastError(), boost::asio::error::get_system_category());
//#else
//        ec = boost::system::error_code(errno, boost::asio::error::get_system_category());
//#endif
//
//        // Error handling not implemented
//    }

    return ok;
}

/////////////////////
// Private Methods //
/////////////////////

void ComSerial::read_write_handler(const boost::system::error_code &error,
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

void ComSerial::timeout_handler(const boost::system::error_code &error)
{
    boost::system::error_code ec;

    // If the timeout timer has been canceled, the operation
    // finished correctly
    if (error == boost::asio::error::operation_aborted)
        return;

    // If the timeout timer expired, cancel the serial port operation
    m_port.cancel(ec);
}

int ComSerial::available_for_read()
{
    int value;

#if defined(BOOST_WINDOWS) || defined(__CYGWIN__)
    COMSTAT status;

    if (0 != ::ClearCommError(m_port.lowest_layer().native_handle(), NULL, &status))
        value = status.cbInQue;
    else
        value = -1;
#else
    if (0 > ::ioctl(m_port.lowest_layer().native_handle(), FIONREAD, &value))
        value = -1;
#endif

//    // Error?
//    if (value == -1)
//    {
//        boost::system::error_code ec;
//
//#if defined(BOOST_WINDOWS) || defined(__CYGWIN__)
//        ec = boost::system::error_code(::GetLastError(), boost::asio::error::get_system_category());
//#else
//        ec = boost::system::error_code(errno, boost::asio::error::get_system_category());
//#endif
//
//        // Error handling not implemented
//    }

    return value;
}

int ComSerial::pending_for_write()
{
    int value;

#if defined(BOOST_WINDOWS) || defined(__CYGWIN__)
    COMSTAT status;

    if (0 != ::ClearCommError(m_port.lowest_layer().native_handle(), NULL, &status))
        value = status.cbOutQue;
    else
        value = -1;
#else
    if (0 > ::ioctl(m_port.lowest_layer().native_handle(), TIOCOUTQ, &value))
        value = -1;
#endif

//    // Error?
//    if (value == -1)
//    {
//        boost::system::error_code ec;
//
//#if defined(BOOST_WINDOWS) || defined(__CYGWIN__)
//        ec = boost::system::error_code(::GetLastError(), boost::asio::error::get_system_category());
//#else
//        ec = boost::system::error_code(errno, boost::asio::error::get_system_category());
//#endif
//
//        // Error handling not implemented
//    }

    return value;
}
