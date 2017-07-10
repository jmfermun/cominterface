//============================================================================
// Name        : example-cominterface.cpp
// Author      : Juan Manuel Fernández Muñoz
// Date        : January, 2017
// Description : Example for the Serial Port and Socket (server) interfaces
//============================================================================

#include <iostream>
#include <string>
#include <stdlib.h>

#include "cominterface/comserial.hpp"
#include "cominterface/comsocket.hpp"

int main()
{
    ComInterface *interface;        // Interface handler
    std::string sel_interface;      // User interface selection
    std::string sel_port;           // User port selection

    std::string description =
    "Program options:\n"\
    "- Interface:\n"\
    "  + serial: use of the serial port.\n"\
    "  + socket: create a socket server waiting for incoming connections.\n"\
    "- Port:\n"\
    "  + If Interface is serial, it is the name of the serial port to use, e.g. COM1.\n"\
    "  + If Interface is socket, it is the TCP port to use.\n"\
    "Note: You can use a terminal program to test this program.\n";

    // Print the program options description
    std::cout << description << std::endl;

    // User interface selection
    std::cout << "Interface = ";
    std::getline(std::cin, sel_interface);

    // User port selection
    std::cout << "Port = ";
    std::getline(std::cin, sel_port);

    // Create the selected interface with the given port
    if (sel_interface == "serial")
        interface = new ComSerial(sel_port, 38400, 8, 1, 'n', 'h', 1000);
    else if (sel_interface == "socket")
    {
        interface = new ComSocket("", atoi(sel_port.c_str()), 1000);

        // Give it 10 seconds to accept incoming connections
        static_cast<ComSocket*>(interface)->SetOpenTimeout(10000);
    }
    else
    {
        std::cout << "Invalid interface.\n"
                  << "Press Enter for exit." << std::endl;

        // Wait until enter is pressed
        std::cin.ignore();

        return 0;
    }

    // Try to open the interface
    if (interface->Open())
    {
        std::string command;        // Command to send
        char receive_buffer[128];   // Buffer for the received data
        int transmitted, received;  // Number of bytes transmitted or received

        std::cout << "\nEnter your command or just press Enter without any input for exit.\n" << std::endl;

        while (true)
        {
            // Wait for the user input
            std::cout << "Tx: ";
            std::getline(std::cin, command);

            // if the user inputs Enter, the loop finishes
            if (command.empty())
                break;

            // Send data through the interface
            transmitted = interface->Write(command.c_str(), command.length());

            // Check the transmission
            if (transmitted == -1)
            {
                std::cout << "\nError transmitting data.\n"
                          << "Press Enter for exit." << std::endl;

                // Wait until enter is pressed
                std::cin.ignore();

                break;
            }
            else if (transmitted != static_cast<int>(command.length()))
                std::cout << "Incomplete data transmission." << std::endl;

            received = 0;

            std::cout << "Rx: ";

            // Receive data from the interface
            do
            {
                // In case of an event driven GUI you better use a non blocking
                // ReadSome(...) in your idle function. Here we have to wait for
                // the response before we send another user command
                received = interface->Read(receive_buffer, sizeof(receive_buffer) - 1);

                // Something received?
                if(received > 0)
                {
                    // Null terminate the received string
                    receive_buffer[received] = '\0';

                    // Print the received string
                    std::cout << receive_buffer;
                }
            } while (received > 0);

            std::cout << std::endl;

            // Check the reception
            if (received == -1)
            {
                std::cout << "\nError receiving data.\n"
                          << "Press Enter for exit." << std::endl;

                // Wait until enter is pressed
                std::cin.ignore();

                break;
            }
        }

        // Close the interface
        interface->Close();
    }
    else
    {
        std::cout << "\nError opening.\n"
                  << "Press Enter for exit." << std::endl;

        // Wait until enter is pressed
        std::cin.ignore();
    }

    // Free the interface resources
    delete interface;

    return 0;
}
