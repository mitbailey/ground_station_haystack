/**
 * @file main.cpp
 * @author Mit Bailey (mitbailey99@gmail.com)
 * @brief 
 * @version See Git tags for version information.
 * @date 2021.08.04
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include "rxmodem.h"
#include "meb_debug.hpp"
#include "gs_haystack.hpp"

int main(int argc, char **argv)
{
    // Ignores broken pipe signal, which is sent to the calling process when writing to a nonexistent socket (
    // see: https://www.linuxquestions.org/questions/programming-9/how-to-detect-broken-pipe-in-c-linux-292898/
    // and
    // https://github.com/sunipkmukherjee/example_imgui_server_client/blob/master/guimain.cpp
    // Allows manual handling of a broken pipe signal using 'if (errno == EPIPE) {...}'.
    // Broken pipe signal will crash the process, and it caused by sending data to a closed socket.
    signal(SIGPIPE, SIG_IGN);

    // Set up global data.
    global_data_t global[1] = {0};
    global->network_data = new NetDataClient(NetPort::HAYSTACK, SERVER_POLL_RATE);

    // Create Ground Station Network thread IDs.
    pthread_t net_polling_tid, net_rx_tid, xband_rx_tid, xband_status_tid;

    // Start the RX threads, and restart them should it be necessary.
    // Only gets-out if a thread declares an unrecoverable emergency and sets its status to -1.
    while (global->network_data->thread_status > -1)
    {
        // 1 = All good, 0 = recoverable failure, -1 = fatal failure (close program)
        global->network_data->thread_status = 1;
        global->network_data->recv_active = true;

        // Initialize and begin socket communication to the server.
        if (!global->network_data->connection_ready)
        {
            // TODO: Check if this is the desired functionality.
            // Currently, the program will not proceed past this point if it cannot connect to the server.
            // NOTE: Loss of connection to server is regained via gs_polling_thread's constant connection_ready check.
            while (gs_connect_to_server(global->network_data) != 1)
            {
                dbprintlf(RED_FG "Failed to establish connection to server.");
                usleep(5 SEC);
            }
        }

        // Start the threads.
        pthread_create(&net_polling_tid, NULL, gs_polling_thread, global->network_data);
        pthread_create(&net_rx_tid, NULL, gs_network_rx_thread, global);
        pthread_create(&xband_rx_tid, NULL, gs_xband_rx_thread, global);
        pthread_create(&xband_status_tid, NULL, xband_status_thread, global);

        void *thread_return;
        pthread_join(net_polling_tid, &thread_return);
        pthread_join(net_rx_tid, &thread_return);
        pthread_join(xband_rx_tid, &thread_return);
        pthread_join(xband_status_tid, &thread_return);

        dbprintlf(RED_BG "thread_status: %d, recv_active: %d", global->network_data->thread_status, global->network_data->recv_active);

        usleep(5 SEC);
        // Loop will begin again, restarting the threads.
    }

    // Shutdown the X-Band radio.
    rxmodem_stop(global->rx_modem);
    rxmodem_destroy(global->rx_modem);
    adf4355_pw_down(global->PLL);
    adf4355_destroy(global->PLL);
    adradio_destroy(global->radio);

    // Destroy other things.
    close(global->network_data->socket);

    int retval = global->network_data->thread_status;
    delete global->network_data;
    return retval;
}