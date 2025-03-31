#include "server.h"

const char IAC_IP[3] = "\xff\xf4";
const char* file_prefix = "./csie_trains/train_";
const char* accept_read_header = "ACCEPT_FROM_READ";
const char* accept_write_header = "ACCEPT_FROM_WRITE";
const char* welcome_banner = "======================================\n"
                             " Welcome to CSIE Train Booking System \n"
                             "======================================\n";

const char* lock_msg = ">>> Locked.\n";
const char* exit_msg = ">>> Client exit.\n";
const char* cancel_msg = ">>> You cancel the seat.\n";
const char* full_msg = ">>> The shift is fully booked.\n";
const char* seat_booked_msg = ">>> The seat is booked.\n";
const char* no_seat_msg = ">>> No seat to pay.\n";
const char* book_succ_msg = ">>> Your train booking is successful.\n";
const char* invalid_op_msg = ">>> Invalid operation.\n";

#ifdef READ_SERVER
char* read_shift_msg = "Please select the shift you want to check [902001-902005]: ";
#elif defined WRITE_SERVER
char* write_shift_msg = "Please select the shift you want to book [902001-902005]: ";
char* write_seat_msg = "Select the seat [1-40] or type \"pay\" to confirm: ";
char* write_seat_or_exit_msg = "Type \"seat\" to continue or \"exit\" to quit [seat/exit]: ";
#endif

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request* reqP);
// initailize a request instance

static void free_request(request* reqP);
// free resources used by a request instance

//-----------------------------------------------------don't need to understand above this line

int accept_conn(void); //done
// accept connection

static void getfilepath(char* filepath, int extension); //done
// get record filepath

void setlock(int fd, int offset){
    struct flock fl = {
        .l_type = F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start = offset,
        .l_len = 1,
    };
    if (fcntl(fd, F_SETLK, &fl) == -1) {
        fprintf(stderr, "Error setting lock to %d\n", fd);
        close(fd);
        // return -1;
    }
    return;
}

void unlock(int fd, int offset){
    struct flock fl = {
        .l_type = F_UNLCK,
        .l_whence = SEEK_SET,
        .l_start = offset,
        .l_len = 1,
    };
    if (fcntl(fd, F_SETLK, &fl) == -1) {
        fprintf(stderr, "Error unlocking to %d\n", fd);
        close(fd);
        // return -1;
    }
    return;
}

int getlock(int fd, int offset){
    struct flock fl = {
        .l_type = F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start = offset,
        .l_len = 1,
    };
    if (fcntl(fd, F_GETLK, &fl) == -1) {
        fprintf(stderr, "Error getting lock to %d\n", fd);
        close(fd);
        // return -1;
    }

    if (fl.l_type != F_UNLCK) 
        return 1; //1 means locked
    return 0;
}

void unlock_all(int fd){
    for (int i = 0; i < SEAT_NUM; i++) 
        unlock(fd, i * 2);
    return;
}

//printing stuff
#ifdef READ_SERVER
int print_train_info(request *reqP, train_info trains[]) {
    int i;
    char buf[MAX_MSG_LEN];
    memset(buf, 0, sizeof(buf));
    //store output in buffer

    int shift_id = atoi(reqP->buf) - TRAIN_ID_START;
    lseek(trains[shift_id].file_fd, 0, SEEK_SET);
    read(trains[shift_id].file_fd, buf, SEAT_NUM * 2);

    // getlock here
    for (i = 0; i < SEAT_NUM; i++) 
        if (getlock(reqP->booking_info.train_fd, i * 2) == 1) {
            //write "2" to the seat in buf
            buf[i * 2] = '2';
        }
    
    write(reqP->conn_fd, buf, SEAT_NUM * 2);

    return 0;
}

#elif defined WRITE_SERVER
int print_train_info(request *reqP) {
    /*
     * Booking info
     * |- Shift ID: 902001
     * |- Chose seat(s): 1,2
     * |- Paid: 3,4
     */
    char chosen_seat[MAX_MSG_LEN];
    char paid[MAX_MSG_LEN];
    char buf[MAX_MSG_LEN*3];
    char seat[4];

    memset(chosen_seat, 0, sizeof(chosen_seat));
    memset(paid, 0, sizeof(paid));
    memset(buf, 0, sizeof(buf));


    bool first_choose = true, first_paid = true;
    for (int i = 0; i < SEAT_NUM; i++) {
        memset(seat, 0, sizeof(seat));
        if (reqP->booking_info.seat_stat[i] == CHOSEN) {
            if (first_choose) {
                sprintf(chosen_seat, "%d", i + 1);
                first_choose = false;
            }
            else 
                sprintf(seat, ",%d", i + 1);
            strcat(chosen_seat, seat);
        }
        else if (reqP->booking_info.seat_stat[i] == PAID) {
            if (first_paid) {
                sprintf(paid, "%d", i + 1);
                first_paid = false;
            }
            else 
                sprintf(seat, ",%d", i + 1);
            strcat(paid, seat);
        }
    }

    //store output in buffer
    sprintf(buf, "\nBooking info\n"
                 "|- Shift ID: %d\n"
                 "|- Chose seat(s): %s\n"
                 "|- Paid: %s\n\n"
                 , reqP->booking_info.shift_id, chosen_seat, paid);
    write(reqP->conn_fd, buf, strlen(buf));
    return 0;
}
#endif





int handle_read(request* reqP) {
    /*  Return value:
     *      1: read successfully
     *      0: read EOF (client down)
     *     -1: read failed
     *   TODO: handle incomplete input
     */

    // Send prompt to the client
#ifdef READ_SERVER
    if (write(reqP->conn_fd, read_shift_msg, strlen(read_shift_msg)) < 0) {
        fprintf(stderr, "Error sending read prompt to client (fd %d)\n", reqP->conn_fd);
        return -1;
    }
#elif defined WRITE_SERVER
    if (reqP->status == SHIFT) {
        if (write(reqP->conn_fd, write_shift_msg, strlen(write_shift_msg)) < 0) {
            fprintf(stderr, "Error sending write prompt to client (fd %d)\n", reqP->conn_fd);
            return -1;
        }
    }
    else if (reqP->status == SEAT) {
        if (write(reqP->conn_fd, write_seat_msg, strlen(write_seat_msg)) < 0) {
            fprintf(stderr, "Error sending write prompt to client (fd %d)\n", reqP->conn_fd);
            return -1;
        }
    }
    else if (reqP->status == BOOKED) {
        if (write(reqP->conn_fd, write_seat_or_exit_msg, strlen(write_seat_or_exit_msg)) < 0) {
            fprintf(stderr, "Error sending write prompt to client (fd %d)\n", reqP->conn_fd);
            return -1;
        }
    }
#endif

    int r;
    char buf[MAX_MSG_LEN];
    size_t len;

    memset(buf, 0, sizeof(buf));

    // Read in request from client
    r = read(reqP->conn_fd, buf, sizeof(buf));

    //check if message is legal
    if (r < 0) return -1;
    if (r == 0) return 0;

    //p1 is the end of the message(find the newline char)
    char* p1 = strstr(buf, "\015\012"); // \r\n
    if (p1 == NULL) {
        p1 = strstr(buf, "\012");   // \n
        if (p1 == NULL) {
            if (!strncmp(buf, IAC_IP, 2)) {
                // Client presses ctrl+C, regard as disconnection
                fprintf(stderr, "Client presses ctrl+C....\n");
                return 0;
            }
        }
    }

    // Copy the message from temporary buffer to request buffer
    len = p1 - buf + 1;
    memmove(reqP->buf, buf, len);
    reqP->buf[len - 1] = '\0';
    reqP->buf_len = len-1;

    return 1;
}





int main(int argc, char** argv) {

    // Parse args.
    //argv[0] is read_server, argv[1] is port number
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    int conn_fd;  // fd for file that we open for reading
    char buf[MAX_MSG_LEN*2], filename[FILE_LEN];

    int i,j;

    // open all train files
    for (i = TRAIN_ID_START, j = 0; i <= TRAIN_ID_END; i++, j++) {
        getfilepath(filename, i);

#ifdef READ_SERVER
        trains[j].file_fd = open(filename, O_RDONLY);
#elif defined WRITE_SERVER
        trains[j].file_fd = open(filename, O_RDWR);
#else
        trains[j].file_fd = -1;
#endif

        //error opening file
        if (trains[j].file_fd < 0) {
            ERR_EXIT("open");
        }
    }

    // Initialize server, atoi: convert port from string to integer
    init_server((unsigned short) atoi(argv[1]));

    //print out server info
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);

    // Loop for handling connections
    // 裡面全部都在做同一件事，
    while (1) {
        // TODO: Add IO multiplexing

        // Check new connection
        conn_fd = accept_conn();
        if (conn_fd < 0)
            continue;

        while(1){//reading from the same client
            int ret = handle_read(&requestP[conn_fd]);

            // check if Error or client disconnected
            if (ret < 0) {
                fprintf(stderr, "bad request from %s\n", requestP[conn_fd].host);
                continue;
            }else if (ret == 0){
                fprintf(stderr, "client disconnected\n");
                break; 
            }

            //check if exit at any point
            if (strcmp(requestP[conn_fd].buf, "exit") == 0) {
                write(requestP[conn_fd].conn_fd, exit_msg, strlen(exit_msg));

                // unlock here
                if (requestP[conn_fd].booking_info.train_fd != -1)
                    unlock_all(requestP[conn_fd].booking_info.train_fd);

                break;
            }

/*
char* endptr;
    seat_id = strtol(requestP[conn_fd].buf, &endptr, 10);
    if (*endptr != '\0' || seat_id < 1 || seat_id > SEAT_NUM) {
        write(conn_fd, invalid_op_msg, strlen(invalid_op_msg));
        close_connection(fds, &nfds, i); i--;
        continue;
    }
*/

#ifdef READ_SERVER   
            // Check if the input is a valid shift number
            // bool valid = true;
            // for (int i = 0; i < 7 && requestP[conn_fd].buf[i] != '\0'; i++) {
            //     if (requestP[conn_fd].buf[i] < '0' || requestP[conn_fd].buf[i] > '9') {
            //         valid = false;
            //         break;
            //     }
            // }
            // if (!valid) {
            //     write(requestP[conn_fd].conn_fd, invalid_op_msg, strlen(invalid_op_msg));
            //     break;
            // }

            // char* endptr;
            // int shift_num = strtol(requestP[conn_fd].buf, &endptr, 10);
            // if (*endptr != '\0' || shift_num < 9020001 || shift_num > 902005) {
            //     write(conn_fd, invalid_op_msg, strlen(invalid_op_msg));
            //     // close_connection(fds, &nfds, i); i--;
            //     break;
            // }

            int shift_num = atoi(requestP[conn_fd].buf);
            if (shift_num < 902001 || shift_num > 902005) {
                write(requestP[conn_fd].conn_fd, invalid_op_msg, strlen(invalid_op_msg));
                // requestP[conn_fd].status = INVALID;
                break;
            }

            requestP[conn_fd].booking_info.shift_id = shift_num;
            requestP[conn_fd].booking_info.train_fd = trains[shift_num - TRAIN_ID_START].file_fd;
            requestP[conn_fd].booking_info.num_of_chosen_seats = 0;
            print_train_info(&requestP[conn_fd], trains);

/*
char* endptr;
    seat_id = strtol(requestP[conn_fd].buf, &endptr, 10);
    if (*endptr != '\0' || seat_id < 1 || seat_id > SEAT_NUM) {
        write(conn_fd, invalid_op_msg, strlen(invalid_op_msg));
        close_connection(fds, &nfds, i); i--;
        continue;
    }
*/


#elif defined WRITE_SERVER
            //deal with different states:

            if (requestP[conn_fd].status == SHIFT){
                // Check if the input is a valid shift number
                // bool valid = true;
                // for (int i = 0; i < 7 && requestP[conn_fd].buf[i] != '\0'; i++) {
                //     if (requestP[conn_fd].buf[i] < '0' || requestP[conn_fd].buf[i] > '9') {
                //         valid = false;
                //         break;
                //     }
                // }
                // if (!valid) {
                //     write(requestP[conn_fd].conn_fd, invalid_op_msg, strlen(invalid_op_msg));
                //     requestP[conn_fd].status = INVALID;
                //     break;
                // }

                // write(requestP[conn_fd].conn_fd, invalid_op_msg, strlen(invalid_op_msg));

                // char* endptr;
                // int shift_num = strtol(requestP[conn_fd].buf, &endptr, 10);
                // if (*endptr != '\0' || shift_num < 9020001 || shift_num > 902005) {
                //     write(conn_fd, invalid_op_msg, strlen(invalid_op_msg));
                //     // close_connection(fds, &nfds, i); i--;
                //     break;
                // }


                int shift_num = atoi(requestP[conn_fd].buf);
                if (shift_num < 902001 || shift_num > 902005) {
                    write(requestP[conn_fd].conn_fd, invalid_op_msg, strlen(invalid_op_msg));
                    requestP[conn_fd].status = INVALID;
                    break;
                }

                // Check if the shift is fully booked
                char buf[SEAT_NUM * 2];
                memset(buf, 0, sizeof(buf));
                bool full = true;
                lseek(trains[shift_num - TRAIN_ID_START].file_fd, 0, SEEK_SET);
                read(trains[shift_num - TRAIN_ID_START].file_fd, buf, SEAT_NUM * 2);
                for (int i = 0; i < SEAT_NUM; i++)
                    if (buf[i] == '0') {
                        full = false;
                        break;
                    }
                    
                if (full) {
                    write(requestP[conn_fd].conn_fd, full_msg, strlen(full_msg));
                    requestP[conn_fd].status = SHIFT;
                    continue;
                }

                // Set shift id, train fd, num_of_chosen_seats
                requestP[conn_fd].booking_info.shift_id = shift_num;
                requestP[conn_fd].booking_info.train_fd = trains[shift_num - TRAIN_ID_START].file_fd;
                requestP[conn_fd].booking_info.num_of_chosen_seats = 0;

                // Set all seats to UNKNOWN
                for (int i = 0; i < SEAT_NUM; i++)
                    requestP[conn_fd].booking_info.seat_stat[i] = UNKNOWN;
                print_train_info(&requestP[conn_fd]);

                //change status to SEAT
                requestP[conn_fd].status = SEAT;
            }

            else if (requestP[conn_fd].status == SEAT){
                // Check if the input is a valid seat number or "pay"
                if (strcmp(requestP[conn_fd].buf, "pay") == 0) {
                    // Check if there is any seat to pay
                    if (requestP[conn_fd].booking_info.num_of_chosen_seats == 0) {
                        write(requestP[conn_fd].conn_fd, no_seat_msg, strlen(no_seat_msg));
                        print_train_info(&requestP[conn_fd]);
                        continue;
                    }
                    else{
                        for (int i = 0; i < SEAT_NUM; i++) 
                            if (requestP[conn_fd].booking_info.seat_stat[i] == CHOSEN){
                                requestP[conn_fd].booking_info.seat_stat[i] = PAID;
                                lseek(requestP[conn_fd].booking_info.train_fd, i * 2, SEEK_SET);
                                write(requestP[conn_fd].booking_info.train_fd, "1", 1);

                                // unlock here
                                unlock(requestP[conn_fd].booking_info.train_fd, i * 2);
                            }

                        //reset the number of chosen seats, switch status to BOOKED
                        write(requestP[conn_fd].conn_fd, book_succ_msg, strlen(book_succ_msg));
                        requestP[conn_fd].booking_info.num_of_chosen_seats = 0;
                        requestP[conn_fd].status = BOOKED;
                    }

                    // Print the booking info
                    print_train_info(&requestP[conn_fd]);
                }
                else {
                    // Check if the input is a valid seat number

                    // bool valid = true;
                    // for (int i = 0; i < 3 && requestP[conn_fd].buf[i] != '\0'; i++) {
                    //     // fprintf(stderr, "debug message %d", i);
                    //     if (requestP[conn_fd].buf[i] < '0' || requestP[conn_fd].buf[i] > '9') {
                    //         valid = false;
                    //         break;
                    //     }
                    // }
                    // if (!valid) {
                    //     write(requestP[conn_fd].conn_fd, invalid_op_msg, strlen(invalid_op_msg));
                    //     requestP[conn_fd].status = INVALID;
                    //     break;
                    // }

                    int seat_num = atoi(requestP[conn_fd].buf);
                    if (seat_num < 1 || seat_num > 40) {
                        write(requestP[conn_fd].conn_fd, invalid_op_msg, strlen(invalid_op_msg));
                        requestP[conn_fd].status = INVALID;
                        break;
                    }

                    // char* endptr;
                    // int seat_num = strtol(requestP[conn_fd].buf, &endptr, 10);
                    // if (*endptr != '\0' || seat_num < 9020001 || seat_num > 902005) {
                    //     write(conn_fd, invalid_op_msg, strlen(invalid_op_msg));
                    //     // close_connection(fds, &nfds, i); i--;
                    //     break;
                    // }

                    // Check if the seat has been booked，如果已經被選過了就直接不理
                    char seat_buf[3];
                    memset(seat_buf, 0, sizeof(seat_buf));
                    lseek(trains[requestP[conn_fd].booking_info.shift_id - TRAIN_ID_START].file_fd, (seat_num - 1) * 2, SEEK_SET);
                    read(trains[requestP[conn_fd].booking_info.shift_id - TRAIN_ID_START].file_fd, seat_buf, 1);
                    if (seat_buf[0] == '1') {
                        write(requestP[conn_fd].conn_fd, seat_booked_msg, strlen(seat_booked_msg));
                        print_train_info(&requestP[conn_fd]);
                        continue;
                    }

                    // Check if the seat has been reserved (getlock here)
                    if (getlock(requestP[conn_fd].booking_info.train_fd, (seat_num - 1) * 2) == 1) {
                        write(requestP[conn_fd].conn_fd, lock_msg, strlen(lock_msg));
                        print_train_info(&requestP[conn_fd]);
                        continue;
                    }


                    //cancel the seat
                    if (requestP[conn_fd].booking_info.seat_stat[seat_num - 1] == CHOSEN){
                        requestP[conn_fd].booking_info.seat_stat[seat_num - 1] = UNKNOWN;
                        requestP[conn_fd].booking_info.num_of_chosen_seats--;

                        //unlock here
                        unlock(requestP[conn_fd].booking_info.train_fd, (seat_num - 1) * 2);
                    }
                    //choose the seat
                    else{ 
                        requestP[conn_fd].booking_info.seat_stat[seat_num - 1] = CHOSEN;
                        requestP[conn_fd].booking_info.num_of_chosen_seats++;

                        //setlock here
                        setlock(requestP[conn_fd].booking_info.train_fd, (seat_num - 1) * 2);
                    }

                    print_train_info(&requestP[conn_fd]);
                }
            }
            else if (requestP[conn_fd].status == BOOKED){//After payment
                if (strcmp(requestP[conn_fd].buf, "seat") == 0) {
                    requestP[conn_fd].status = SEAT;
                    print_train_info(&requestP[conn_fd]);
                }                
                else {
                    write(requestP[conn_fd].conn_fd, invalid_op_msg, strlen(invalid_op_msg));
                    break;
                }
            }
#endif
        }
        close(requestP[conn_fd].conn_fd);
        free_request(&requestP[conn_fd]);
    }      

    free(requestP);
    close(svr.listen_fd);
    for (i = 0;i < TRAIN_NUM; i++)
        close(trains[i].file_fd);

    return 0;
}





int accept_conn(void) {

    struct sockaddr_in cliaddr;
    size_t clilen;
    int conn_fd;  // fd for a new connection with client

    clilen = sizeof(cliaddr);
    conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
    if (conn_fd < 0) {
        if (errno == EINTR || errno == EAGAIN) return -1;  // try again
        if (errno == ENFILE) {
            (void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                return -1;
        }
        ERR_EXIT("accept");
    }
    
    requestP[conn_fd].conn_fd = conn_fd;
    strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
    fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);
    requestP[conn_fd].client_id = (svr.port * 1000) + num_conn;    // This should be unique for the same machine.
    num_conn++;
    
    if (write(conn_fd, welcome_banner, strlen(welcome_banner)) < 0) {
        fprintf(stderr, "Error sending welcome banner to client (fd %d)\n", conn_fd);
        close(conn_fd);
        return -1;
    }
    
#ifdef WRITE_SERVER
    // Initialize status
    requestP[conn_fd].status = SHIFT;
    requestP[conn_fd].booking_info.shift_id = -1;

#endif


    return conn_fd;
}






static void getfilepath(char* filepath, int extension) {
    char fp[FILE_LEN*2];
    
    memset(filepath, 0, FILE_LEN);
    sprintf(fp, "%s%d", file_prefix, extension);
    strcpy(filepath, fp);
}

// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>

static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->client_id = -1;
    reqP->buf_len = 0;
    reqP->status = INVALID;
    reqP->remaining_time.tv_sec = 5;
    reqP->remaining_time.tv_usec = 0;

    reqP->booking_info.num_of_chosen_seats = 0;
    reqP->booking_info.train_fd = -1;
    for (int i = 0; i < SEAT_NUM; i++)
        reqP->booking_info.seat_stat[i] = UNKNOWN;
}

static void free_request(request* reqP) {
    memset(reqP, 0, sizeof(request));
    init_request(reqP);
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    // Get file descripter table size and initialize request table
    maxfd = getdtablesize();
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (int i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    return;
}