//============================================================================
// Name        : example-cominterface.cpp
// Author      : Juan Manuel Fernández Muñoz
// Date        : January, 2017
// Description : Example for the Serial Port and Socket interfaces
//============================================================================

#include <iostream>
#include <string>

#include "comserial.h"
#include "comsocket.h"

int main()
{
    ComInterface *interface;    // Interface handler
    std::string selection;      // Interface selection

    std::cout << "Select the interface (serial / socket): ";

    // Wait for the user selection
    std::getline(std::cin, selection);

    // Create the selected interface
    if (selection == "serial")
        interface = new ComSerial("COM1", 38400, 8, 1, 'n', 'h', 1000);
    else if (selection == "socket")
        interface = new ComSocket("192.168.1.100", 3444, 1000);
    else
        return 0;

    // Try to open the interface
    if (interface->Open())
    {
        std::string command;        // Command to send
        char receive_buffer[128];   // Buffer for the received data

        std::cout << "Enter your command or just press Enter without any input for exit!\n" << std::endl;

        while (true)
        {
            // Wait for the user input
            std::getline(std::cin, command);

            // if the user inputs Enter, the loop finishes
            if (command.empty())
                break;

            // Send data through the interface
            if (interface->Write(command.c_str(), command.length()) != static_cast<int>(command.length()))
                std::cout << "Incomplete data transmission" << std::endl;

            int received = 0;

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
        }

        // Close the interface
        interface->Close();
    }

    // Free the interface resources
    delete interface;

    return 0;
}
