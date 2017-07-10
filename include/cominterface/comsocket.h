/**
 * @file    comsocket.h
 * @author  Juan Manuel Fernández Muñoz
 * @date    January, 2017
 * @brief   TCP/IP Socket communication interface header.
 */

#ifndef _COMSOCKET_H_
#define _COMSOCKET_H_

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "cominterface/cominterface.h"

/**
 * @brief TCP/IP Socket communication interface.
 * It can be used as a server or a client.
 */
class ComSocket: public ComInterface
{
public:
    /**
     * @brief TCP/IP socket interface constructor.
     * @param address IP address of the device to connect. Example: "192.168.1.100".
     * If set to "", the use mode is server. Else, client mode.
     * @param port TCP port of the service to connect.
     * @param timeout Timeout in milliseconds for Open, Read and Write.
     */
    ComSocket(const std::string& address = "127.0.0.1", unsigned int port = 3444,
              unsigned int timeout = 1000);

    /**
     * @brief Virtual destructor for the socket interface.
     * It is necessary for polymorphism.
     */
    virtual ~ComSocket();

    virtual bool Open();

    virtual bool Close();

    virtual bool Opened();

    virtual int ReadSome(void *buffer_in, size_t len);

    virtual int WriteSome(const void *buffer_out, size_t len);

    virtual int Read(void *buffer_in, size_t len);

    virtual int Write(const void *buffer_out, size_t len);

    virtual void Abort();

    virtual bool SetWriteTimeout(unsigned int write_timeout);

    virtual unsigned int GetWriteTimeout();

    virtual bool SetReadTimeout(unsigned int read_timeout);

    virtual unsigned int GetReadTimeout();

    /**
     * @brief Set the timeout time of the Open operations.
     * @param open_timeout Time in milliseconds.
     * @return true if the function executes correctly, false otherwise.
     */
    bool SetOpenTimeout(unsigned int open_timeout);

    /**
     * @brief Get the timeout time of the Open operations.
     * @return Time in milliseconds.
     */
    unsigned int GetOpenTimeout();

    /**
     * @brief Set the IP address of the device to connect.
     * @param address IP address of the device to connect. Example: "192.168.1.100".
     * If set to "", the use mode is server. Else, client mode.
     */
    bool SetAddress(const std::string& address);

    /**
     * @brief Get the IP address of the device to connect.
     * @return IP address of the device to connect. Example: "192.168.1.100".
     * @note If the current mode is server, the returned value will be "".
     */
    std::string GetAddress();

    /**
     * @brief Set the TCP port of the service to connect.
     * @param port TCP port of the service to connect.
     */
    bool SetPort(unsigned int port);

    /**
     * @brief Get the TCP port of the service to connect.
     * @param port TCP port of the service to connect.
     */
    unsigned int GetPort();

private:
    // Control de boost asio
    boost::asio::io_service m_io_service;               ///< Service for access OS resources.
    boost::asio::ip::tcp::socket m_socket;              ///< Socket handler.
    boost::asio::deadline_timer m_timer;                ///< Timeout timer for the asynchronous operations.
    boost::asio::ip::tcp::acceptor m_acceptor;          ///< Acceptor for incoming connections in server mode.
    boost::posix_time::time_duration m_write_timeout;   ///< Time in milliseconds for the transmission timeout timer.
    boost::posix_time::time_duration m_read_timeout;    ///< Time in milliseconds for the reception timeout timer.
    boost::posix_time::time_duration m_open_timeout;    ///< Time in milliseconds for the open timeout timer.

    // Configuración de la conexión
    boost::asio::ip::address m_address;                 ///< IP address.
    unsigned int m_port;                                ///< TCP port.

    boost::mutex m_mutex;                               ///< Mutex to make the interface thread safe.

    /**
     * @brief This function is executed when a connect or accept asynchronous
     * operation is completed.
     * @param error Indicate if an error occurred during the asynchronous operation.
     * If no error occurs, its value will be 0.
     * @param ret_code Return code for the current asynchronous operation.
     */
    void open_handler(const boost::system::error_code& error, bool *ret_code);

    /**
     * @brief This function is executed when a read/write asynchronous operation
     * is completed.
     * @param error Indicate if an error occurred during the asynchronous operation.
     * If no error occurs, its value will be 0.
     * @param bytes_transferred Number of bytes transmitted or received in the asynchronous operation.
     * @param ret_code Return code for the current asynchronous operation.
     */
    void read_write_handler(const boost::system::error_code& error,
                            size_t bytes_transferred, int *ret_code);

    /**
     * @brief This function is executed when a timeout timer asynchronous operation
     * is completed.
     * @param error Indicate if an error occurred during the asynchronous operation.
     * If no error occurs (the timeout timer has expired), its value will be 0.
     */
    void timeout_handler(const boost::system::error_code& error);

    /**
     * @brief This function is executed when a timeout occurs on an asynchronous accept.
     * @param error Indicate if an error occurred during the asynchronous operation.
     * If no error occurs (the timeout timer has expired), its value will be 0.
     */
    void timeout_accept_handler(const boost::system::error_code& error);
};

#endif // _COMSOCKET_H_
