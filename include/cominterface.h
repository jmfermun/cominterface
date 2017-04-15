/**
 * @file    cominterface.h
 * @author  Juan Manuel Fernández Muñoz
 * @date    January, 2017
 * @brief   Base communication interface header
 */

#ifndef _COMINTERFACE_H_
#define _COMINTERFACE_H_

/**
 * @brief   Base interface for various specific communication
 *          interfaces. It allows to use polymorphism
 */
class ComInterface
{
public:
    /**
     * @brief Virtual destructor for the interface.
     * It is necessary for polymorphism
     */
    virtual ~ComInterface() {};

    /**
     * @brief Open the interface
     * @return true if OK, false if an error occurs
     */
    virtual bool Open() = 0;

    /**
     * @brief Close the interface
     * @return true if OK, false if an error occurs
     */
    virtual bool Close() = 0;

    /**
     * @brief Check if the interface is currently opened
     * @return true if the interface is opened, false if it is closed
     */
    virtual bool Opened() = 0;

    /**
     * @brief Non blocking read. It tries to read the available bytes for the interface
     * @param buffer_in Buffer that will contain the received data
     * @param len Number of bytes to read
     * @return Number of bytes read or -1 in case of error
     */
    virtual int ReadSome(void *buffer_in, size_t len) = 0;

    /**
     * @brief Non blocking write. It tries to write the the data passed to it
     * @param buffer_out Buffer that contains the data to be transmitted
     * @param len Number of bytes to write
     * @return Number of bytes written or -1 in case of error
     */
    virtual int WriteSome(const void *buffer_out, size_t len) = 0;

    /**
     * @brief Blocking read. It waits until the indicated number of bytes
     * are received or the timeout expires
     * @param buffer_in Buffer that will contain the received data
     * @param len Number of bytes to read
     * @return Number of bytes read or -1 in case of error
     */
    virtual int Read(void *buffer_in, size_t len) = 0;

    /**
     * @brief Blocking write. It waits until the indicated number of bytes
     * are transmitted or the timeout expires
     * @param buffer_out Buffer that contains the data to be transmitted
     * @param len Number of bytes to write
     * @return Number of bytes written or -1 in case of error
     */
    virtual int Write(const void *buffer_out, size_t len) = 0;

    /**
     * @brief Abort the current operation on the interface
     */
    virtual void Abort() {}

    /**
     * @brief Set the timeout time of the Write operations
     * @param write_timeout Time in milliseconds
     * @return true if the function executes correctly, false otherwise
     */
    virtual bool SetWriteTimeout(unsigned int write_timeout) = 0;

    /**
     * @brief Get the timeout time of the Write operations
     * @return Time in milliseconds
     */
    virtual unsigned int GetWriteTimeout() = 0;

    /**
     * @brief Set the timeout time of the Read operations
     * @param read_timeout Time in milliseconds
     * @return true if the function executes correctly, false otherwise
     */
    virtual bool SetReadTimeout(unsigned int read_timeout) = 0;

    /**
     * @brief Get the timeout time of the Read operations
     * @return Time in milliseconds
     */
    virtual unsigned int GetReadTimeout() = 0;
};

#endif // _COMINTERFACE_H_
