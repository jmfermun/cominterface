/**
 * @file    comserial.h
 * @author  Juan Manuel Fernández Muñoz
 * @date    January, 2017
 * @brief   Serial Port communication interface header.
 */

#ifndef _COMSERIAL_H_
#define _COMSERIAL_H_

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "cominterface/cominterface.h"

/**
 * @brief Serial Port communication interface.
 */
class ComSerial : public ComInterface
{
public:
    /**
     * @brief Serial port interface constructor.
     * @param device Name of the serial port. Windows example: "COM1".
     * Linux example: "/dev/ttyS0".
     * @param baud_rate Baudrate.
     * @param data_bits Number of data bits.
     * @param stop_bits Number of stop bits. Set this parameter to 3
     * means 1.5 stop bits.
     * @param parity Parity. It can be even 'e', odd 'o' or nothing 'n'.
     * @param flow_control Flow control. It can be hardware 'h',
     * software 's' or nothing 'n'.
     * @param timeout Timeout in milliseconds for Read and Write.
     */
    ComSerial(const std::string& device = "COM1", unsigned int baud_rate = 38400,
              unsigned int data_bits = 8, unsigned int stop_bits = 1,
              char parity = 'n', char flow_control = 'h', unsigned int timeout = 1000);

    /**
     * @brief Virtual destructor for the serial port interface.
     * It is necessary for polymorphism.
     */
    virtual ~ComSerial();

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
     * @brief Set the device name of the serial port.
     * @param device Name of the serial port. Windows example: "COM1".
     * Linux example: "/dev/ttyS0".
     * @return true if the function executes correctly, false otherwise.
     */
    bool SetDevice(const std::string& device);

    /**
     * @brief Get the device name of the serial port.
     * @return Name of the serial port. Windows example: "COM1".
     * Linux example: "/dev/ttyS0".
     */
    std::string GetDevice();

    /**
     * @brief Set the baud rate of the serial port.
     * @param baud_rate Baudrate.
     * @return true if the function executes correctly, false otherwise.
     */
    bool SetBaudRate(unsigned int baud_rate);

    /**
     * @brief Get the baud rate of the serial port.
     * @return Baudrate.
     */
    unsigned int GetBaudRate();

    /**
     * @brief Set the number of data bits of the serial port.
     * @param data_bits Number of data bits.
     * @return true if the function executes correctly, false otherwise.
     */
    bool SetDataBits(unsigned int data_bits);

    /**
     * @brief Get the number of data bits of the serial port.
     * @return Number of data bits.
     */
    unsigned int GetDataBits();

    /**
     * @brief Set the number of stop bits of the serial port.
     * @param stop_bits Number of stop bits. Set this parameter to 3
     * means 1.5 stop bits.
     * @return true if the function executes correctly, false otherwise.
     */
    bool SetStopBits(unsigned int stop_bits);

    /**
     * @brief Get the number of stop bits of the serial port.
     * @return stop_bits Number of stop bits. If the returned value is 3,
     * it means 1.5 stop bits.
     */
    unsigned int GetStopBits();

    /**
     * @brief Set the parity of the serial port.
     * @param parity Parity. It can be even 'e', odd 'o' or nothing 'n'.
     * @return true if the function executes correctly, false otherwise.
     */
    bool SetParity(char parity);

    /**
     * @brief Get the parity of the serial port.
     * @return Parity. It can be even 'e', odd 'o' or nothing 'n'.
     */
    char GetParity();

    /**
     * @brief Set the flow control of the serial port.
     * @param flow_control Flow control. It can be hardware 'h',
     * software 's' or nothing 'n'.
     * @return true if the function executes correctly, false otherwise.
     */
    bool SetFlowControl(char flow_control);

    /**
     * @brief Get the flow control of the serial port.
     * @return Flow control. It can be hardware 'h',
     * software 's' or nothing 'n'.
     */
    char GetFlowControl();

    /**
     * @brief Discard the pending bytes in the kernel buffers of
     * the serial port.
     * @return true if function succeeds, false if not.
     */
    bool Flush();

private:
    // Control de boost asio
    boost::asio::io_service m_io_service;               ///< Service for access OS resources.
    boost::asio::serial_port m_port;                    ///< Serial port handler.
    boost::asio::deadline_timer m_timer;                ///< Timeout timer for the asynchronous operations.
    boost::posix_time::time_duration m_write_timeout;   ///< Time in milliseconds for the transmission timeout timer.
    boost::posix_time::time_duration m_read_timeout;    ///< Time in milliseconds for the reception timeout timer.

    // Configuración del puerto serie
    std::string m_device;                                       ///< Name of the serial port.
    boost::asio::serial_port_base::baud_rate m_baud_rate;       ///< Baudrate.
    boost::asio::serial_port_base::character_size m_data_bits;  ///< Number of data bits.
    boost::asio::serial_port_base::stop_bits m_stop_bits;       ///< Number of stop bits.
    boost::asio::serial_port_base::parity m_parity;             ///< Parity.
    boost::asio::serial_port_base::flow_control m_flow_control; ///< Flow control.

    boost::mutex m_mutex;       ///< Mutex to make the interface thread safe.

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
     * @brief Get the number of bytes in the kernel read buffer of the serial port.
     * @return Number of bytes. If an error occurs, it returns -1.
     */
    int available_for_read();

    /**
     * @brief Get the number of bytes in the kernel write buffer of the serial port.
     * @return Number of bytes. If an error occurs, it returns -1.
     */
    int pending_for_write();
};

#endif // _COMSERIAL_H_
