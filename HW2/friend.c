#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "hw2.h"
#define ERR_EXIT(s) perror(s), exit(errno);

/*
If you need help from TAs,
please remember :
0. Show your efforts
    0.1 Fully understand course materials
    0.2 Read the spec thoroughly, including frequently updated FAQ section
    0.3 Use online resources
    0.4 Ask your friends while avoiding plagiarism, they might be able to understand you better, since the TAs know the solution, 
        they might not understand what you're trying to do as quickly as someone who is also doing this homework.
1. be respectful
2. the quality of your question will directly impact the value of the response you get.
3. think about what you want from your question, what is the response you expect to get
4. what do you want the TA to help you with. 
    4.0 Unrelated to Homework (wsl, workstation, systems configuration)
    4.1 Debug
    4.2 Logic evaluation (we may answer doable yes or no, but not always correct or incorrect, as it might be giving out the solution)
    4.3 Spec details inquiry
    4.4 Testcase possibility
5. If the solution to answering your question requires the TA to look at your code, you probably shouldn't ask it.
6. We CANNOT tell you the answer, but we can tell you how your current effort may approach it.
7. If you come with nothing, we cannot help you with anything.
*/

// somethings I recommend leaving here, but you may delete as you please
static char root[MAX_FRIEND_INFO_LEN] = "Not_Tako";     // root of tree
static char friend_info[MAX_FRIEND_INFO_LEN];   // current process info
static char friend_name[MAX_FRIEND_NAME_LEN];   // current process name
static int friend_value;    // current process value
FILE *Pread_fp = NULL, *Pwrite_fp = NULL; 
friend friends[8];    //at most 8
int friend_count;
pid_t process_pid;

// Is Root of tree
static inline bool is_Not_Tako() {
    return (strcmp(friend_name, root) == 0);
}

// a bunch of prints for you
void print_direct_meet(char *friend_name) {
    fprintf(stdout, "Not_Tako has met %s by himself\n", friend_name);
}

void print_indirect_meet(char *parent_friend_name, char *child_friend_name) {
    fprintf(stdout, "Not_Tako has met %s through %s\n", child_friend_name, parent_friend_name);
}

void print_fail_meet(char *parent_friend_name, char *child_friend_name) {
    fprintf(stdout, "Not_Tako does not know %s to meet %s\n", parent_friend_name, child_friend_name);
}

void print_fail_check(char *parent_friend_name){
    fprintf(stdout, "Not_Tako has checked, he doesn't know %s\n", parent_friend_name);
}

void print_success_adopt(char *parent_friend_name, char *child_friend_name) {
    fprintf(stdout, "%s has adopted %s\n", parent_friend_name, child_friend_name);
}

void print_fail_adopt(char *parent_friend_name, char *child_friend_name) {
    fprintf(stdout, "%s is a descendant of %s\n", parent_friend_name, child_friend_name);
}

void print_compare_gtr(char *friend_name){
    fprintf(stdout, "Not_Tako is still friends with %s\n", friend_name);
}

void print_compare_leq(char *friend_name){
    fprintf(stdout, "%s is dead to Not_Tako\n", friend_name);
}

void print_final_graduate(){
    fprintf(stdout, "Congratulations! You've finished Not_Tako's annoying tasks!\n");
}

/* terminate child pseudo code
void clean_child(){
    close(child read_fd);
    close(child write_fd);
    call wait() or waitpid() to reap child; // this is blocking
}
*/

void fully_write(int write_fd, void *write_buf, int write_len){
    int written_bytes = 0, ret;
    while(written_bytes < write_len){
        if((ret = write(write_fd, write_buf + written_bytes, write_len - written_bytes)) < 0){
            ERR_EXIT("write");
        }
        written_bytes += ret;
    }
}

// void fully_read(int read_fd, void *read_buf, int read_len){
//     int read_bytes = 0;
//     while(read_bytes < read_len){
//         int ret = read(read_fd, read_buf + read_bytes, read_len - read_bytes);
//         if(ret < 0){
//             ERR_EXIT("read");
//         }
//         read_bytes += ret;
//     }
// }

void meet(char full_cmd[MAX_CMD_LEN]){//Meet someone someone_value
    //reading command
    char full_cmd_forsend[MAX_CMD_LEN], cmd[MAX_CMD_LEN], parent_name[MAX_FRIEND_NAME_LEN]
    , child_info[MAX_FRIEND_INFO_LEN], child_info_forcut[MAX_FRIEND_INFO_LEN], child_name[MAX_FRIEND_NAME_LEN];
    int child_value = 0;
    memset(full_cmd_forsend, 0, MAX_CMD_LEN);
    memset(cmd, 0, MAX_CMD_LEN);
    memset(parent_name, 0, MAX_FRIEND_NAME_LEN);
    memset(child_info, 0, MAX_FRIEND_INFO_LEN);
    memset(child_info_forcut, 0, MAX_FRIEND_INFO_LEN);
    memset(child_name, 0, MAX_FRIEND_NAME_LEN);

    strcpy(full_cmd_forsend, full_cmd);
    full_cmd[strcspn(full_cmd, "\r")] = '\0';
    full_cmd[strcspn(full_cmd, "\n")] = '\0';
    //forsend才是真的command，full_cmd是用來切割的

    //splitting parts of the command
    char *token = strtok(full_cmd, " ");
    if (token != NULL) {
        strcpy(cmd, token);           
        token = strtok(NULL, " ");   
    }
    if (token != NULL) {
        strcpy(parent_name, token);    //did not cut in Not_Tako  
        token = strtok(NULL, " ");    
    }
    if (token != NULL){
        strcpy(child_info, token);
        strcpy(child_info_forcut, child_info);
    }
    char *token1 = strtok(child_info_forcut, "_");
    if (token1 != NULL) {
        strcpy(child_name, token1);           
        token1 = strtok(NULL, "_");   
    }
    if (token1 != NULL) 
        child_value = atoi(token1);
    //parent寫名字而已
    //child寫名字跟數字

    if (is_Not_Tako()){
        if (strcmp(parent_name, friend_name)==0){
            int downfd[2], upfd[2];
            if (pipe(downfd) < 0) ERR_EXIT("root down pipe");
            if (pipe(upfd) < 0) ERR_EXIT("root up pipe");

            // int flags = fcntl(downfd[0], F_GETFL, 0);
            // fcntl(downfd[0], F_SETFL, flags & ~O_NONBLOCK);
            // flags = fcntl(downfd[1], F_GETFL, 0);
            // fcntl(downfd[1], F_SETFL, flags & ~O_NONBLOCK);
            // flags = fcntl(upfd[0], F_GETFL, 0);
            // fcntl(upfd[0], F_SETFL, flags & ~O_NONBLOCK);
            // flags = fcntl(upfd[1], F_GETFL, 0);
            // fcntl(upfd[1], F_SETFL, flags & ~O_NONBLOCK);

            if ((process_pid = fork()) < 0){
                ERR_EXIT("fork");
            } 
            else if (process_pid == 0){//child
                if (close(downfd[1]) < 0) ERR_EXIT("close\n");
                if (close(upfd[0]) < 0 ) ERR_EXIT("close\n");

                // printf("child %s reads from %d writes to %d\n", child_name, downfd[0], upfd[1]);

                if (dup2(downfd[0], TEMP_READ_FD) < 0) ERR_EXIT("dup2\n");
                if (dup2(upfd[1], TEMP_WRITE_FD) < 0) ERR_EXIT("dup2\n");
                // printf("Not_Tako read_fd %d write_fd %d\n", downfd[0], upfd[1]);

                for (int i = 0; i < friend_count; i++){
                    if (close(friends[i].read_fd) < 0) ERR_EXIT("close\n");
                    if (close(friends[i].write_fd) < 0) ERR_EXIT("close\n");
                    // printf("closed %d and %d\n", friends[i].read_fd, friends[i].write_fd);
                    friends[i].read_fp = NULL;
                }

                if (dup2(TEMP_READ_FD, PARENT_READ_FD) < 0) ERR_EXIT("dup2\n");
                if (dup2(TEMP_WRITE_FD, PARENT_WRITE_FD) < 0) ERR_EXIT("dup2\n");

                if (close(TEMP_READ_FD) < 0) ERR_EXIT("close\n");
                if (close(TEMP_WRITE_FD) < 0) ERR_EXIT("close\n");

                execlp("./friend", "./friend", child_info, NULL);
            }
            else{//parent
                if (close(downfd[0]) < 0) ERR_EXIT("close\n");
                if (close(upfd[1]) <0 ) ERR_EXIT("close\n");

                friends[friend_count].pid = process_pid;
                friends[friend_count].read_fd = upfd[0];
                friends[friend_count].read_fp = fdopen(upfd[0], "r");
                friends[friend_count].write_fd = downfd[1];

                strcpy(friends[friend_count].info, child_info);
                strcpy(friends[friend_count].name, child_name);
                friends[friend_count].value = child_value;

                print_direct_meet(friends[friend_count].name);
                friend_count++;
            }
        }
        else{//send command down
            bool found = false;
            char response[10];
            // full_cmd[strcspn(full_cmd, "\0")] = '\n';
            for (int i = 0; i < friend_count; i++){
                memset(response, 0, 10);
                // printf("In Not_Tako sending to %s\n", friends[i].name);
                fully_write(friends[i].write_fd, full_cmd_forsend, strlen(full_cmd_forsend));

                fgets(response, 10, friends[i].read_fp);//blocking read
                response[strcspn(response, "\n")] = '\0';
                // printf("In %s received response -%s- from %s\n", friend_name, response, friends[i].name);
                if (strcmp(response, "found") == 0){
                    found = true;
                    break;
                }
            }
            if (!found)
                print_fail_meet(parent_name, child_name);
        }   
    }
    else{//not root
        //如果是自己，回傳found
        //每個點如果找完全部孩子都沒有找到，就回傳no
        //root如果每個孩子都沒找到，就印出失敗
        if (strcmp(parent_name, friend_name) == 0){
            int downfd[2], upfd[2];
            if (pipe(downfd) < 0) ERR_EXIT("root down pipe");
            if (pipe(upfd) < 0) ERR_EXIT("root up pipe");

            if ((process_pid = fork()) < 0){
                ERR_EXIT("fork");
            } 
            else if (process_pid == 0){//child

                if (close(downfd[1]) < 0) ERR_EXIT("close\n");
                if (close(upfd[0]) < 0 ) ERR_EXIT("close\n");

                // printf("child %s reads from %d writes to %d\n", child_name, downfd[0], upfd[1]);

                if (dup2(downfd[0], TEMP_READ_FD) < 0) ERR_EXIT("dup2\n");
                if (dup2(upfd[1], TEMP_WRITE_FD) < 0) ERR_EXIT("dup2\n");
                // printf("Not_Tako read_fd %d write_fd %d\n", downfd[0], upfd[1]);

                for (int i = 0; i < friend_count; i++){
                    if (close(friends[i].read_fd) < 0) ERR_EXIT("close\n");
                    if (close(friends[i].write_fd) < 0) ERR_EXIT("close\n");
                    // printf("closed %d and %d\n", friends[i].read_fd, friends[i].write_fd);
                    friends[i].read_fp = NULL;
                }

                if (dup2(TEMP_READ_FD, PARENT_READ_FD) < 0) ERR_EXIT("dup2\n");
                if (dup2(TEMP_WRITE_FD, PARENT_WRITE_FD) < 0) ERR_EXIT("dup2\n");

                if (close(TEMP_READ_FD) < 0) ERR_EXIT("close\n");
                if (close(TEMP_WRITE_FD) < 0) ERR_EXIT("close\n");

                execlp("./friend", "./friend", child_info, NULL);
            }
            else{//parent
                if (close(downfd[0]) < 0) ERR_EXIT("close\n");
                if (close(upfd[1]) <0 ) ERR_EXIT("close\n");

                friends[friend_count].pid = process_pid;//非root的meet還沒改好
                friends[friend_count].read_fd = upfd[0];
                friends[friend_count].read_fp = fdopen(upfd[0], "r");
                friends[friend_count].write_fd = downfd[1];

                strcpy(friends[friend_count].info, child_info);
                strcpy(friends[friend_count].name, child_name);
                friends[friend_count].value = child_value;

                // printf("In %s sending found\n", friend_name);
                fully_write(PARENT_WRITE_FD, "found\n", 6);
                print_indirect_meet(parent_name, child_name);

                friend_count++;
            }
        }
        else{//send command down
            bool found = false;
            char response[10];
            for (int i = 0; i < friend_count; i++){
                memset(response, 0, 10);
                // printf("In %s sending to %s\n", friend_name, friends[i].name);
                fully_write(friends[i].write_fd, full_cmd_forsend, strlen(full_cmd_forsend));

                fgets(response, 10, friends[i].read_fp);//blocking read
                response[strcspn(response, "\n")] = '\0';
                // printf("In %s received response -%s- from %s\n", friend_name, response, friends[i].name);
                if (strcmp(response, "found") == 0){
                    found = true;
                    fully_write(PARENT_WRITE_FD, "found\n", 6);
                    // printf("Sent %s in %s\n", "found", friend_name);
                    break;
                }
            }
            if (!found){
                fully_write(PARENT_WRITE_FD, "no\n", 3);
                // printf("Sent %s in %s\n", "no", friend_name);
            }
        }
    }
    return;
}

void print_self(char full_cmd[MAX_CMD_LEN]){//Print 1 1
    // printf("in %s received %s\n", friend_name, full_cmd);
    full_cmd[strcspn(full_cmd, "\r")] = '\0';
    full_cmd[strcspn(full_cmd, "\n")] = '\0';
    int level = 0, left_most = 0;
    char cmd[MAX_CMD_LEN];
    memset(cmd, 0, MAX_CMD_LEN);

    // fprintf(stdout, "In %s received -%s-\n", friend_info, full_cmd);

    char *token = strtok(full_cmd, " ");
    if (token != NULL) {
        strcpy(cmd, token);           
        token = strtok(NULL, " ");   
    }
    if (token != NULL) {
        level = atoi(token) - 1;           
        token = strtok(NULL, " ");   
    }
    if (token != NULL) {
        left_most = atoi(token);           
    }

    // fprintf(stdout, "In %s level = %d", friend_info, level);

    if (level == 0){
        if (left_most == 1) fprintf(stdout, "%s", friend_info);
        else fprintf(stdout, " %s", friend_info);
        fully_write(PARENT_WRITE_FD, "found\n", 6);
    }
    else{ 
        bool there_are_no_children_on_this_level = true;
        char message[50], response[10];
        for (int i = 0; i < friend_count; i++){
            memset(message, 0, 50);
            memset(response, 0, 10);
            if (left_most == 1 && there_are_no_children_on_this_level) sprintf(message, "Print %d 1\n", level);
            else sprintf(message, "Print %d 0\n", level);
            fully_write(friends[i].write_fd, message, strlen(message));

            fgets(response, 10, friends[i].read_fp);//blocking read
            response[strcspn(response, "\n")] = '\0';
            // printf("In %s received response -%s- from %s\n", friend_name, response, friends[i].name);
            if (strcmp(response, "found") == 0)
                there_are_no_children_on_this_level = false;
        }
        if (there_are_no_children_on_this_level) fully_write(PARENT_WRITE_FD, "no\n", 3);
        else fully_write(PARENT_WRITE_FD, "found\n", 6);
    }
}

bool check(char full_cmd[MAX_CMD_LEN]){//Check someone
    char full_cmd_forsend[MAX_CMD_LEN], cmd[MAX_CMD_LEN], root_of_check_name[MAX_FRIEND_NAME_LEN];
    memset(full_cmd_forsend, 0, MAX_CMD_LEN);
    memset(cmd, 0, MAX_CMD_LEN);
    memset(root_of_check_name, 0, MAX_FRIEND_NAME_LEN);

    strcpy(full_cmd_forsend, full_cmd);
    full_cmd[strcspn(full_cmd, "\r")] = '\0';
    full_cmd[strcspn(full_cmd, "\n")] = '\0';

    char *token = strtok(full_cmd, " ");
    if (token != NULL) {
        strcpy(cmd, token);           
        token = strtok(NULL, " ");   
    }
    if (token != NULL) {
        strcpy(root_of_check_name, token);
        token = strtok(NULL, " ");    
    }

    if (is_Not_Tako()){
        if (strcmp(root_of_check_name, friend_name) == 0){
            //continue checking
            fprintf(stdout, "Not_Tako\n");
            char message[50], response[10];
            for (int level = 1; level < 7; level++){
                bool there_are_no_children_on_this_level = true;
                for (int i = 0; i < friend_count; i++){
                    memset(message, 0, 50);
                    memset(response, 0, 10);
                    if (there_are_no_children_on_this_level) sprintf(message, "Print %d 1\n", level);
                    else sprintf(message, "Print %d 0\n", level);
                    fully_write(friends[i].write_fd, message, strlen(message));
                    
                    fgets(response, 10, friends[i].read_fp);//blocking read
                    response[strcspn(response, "\n")] = '\0';
                    // fprintf(stdout, "In Not_Tako received response -%s- from %d\n", response, friends[i].read_fd);
                    if (strcmp(response, "found") == 0)
                        there_are_no_children_on_this_level = false;    
                }
                if (there_are_no_children_on_this_level)
                    break;
                fprintf(stdout, "\n");
            }
        }
        else{//pass down
            bool found_check_root = false;
            char response[10];
            for (int i = 0; i < friend_count; i++){
                memset(response, 0, 10);
                fully_write(friends[i].write_fd, full_cmd_forsend, strlen(full_cmd_forsend));

                fgets(response, 10, friends[i].read_fp);//blocking read
                response[strcspn(response, "\n")] = '\0';
                if (strcmp(response, "found") == 0){
                    found_check_root = true;
                    break;
                }
            }
            if (!found_check_root){
                print_fail_check(root_of_check_name);
                return false;
            }
        }
    }
    else{
        if (strcmp(root_of_check_name, friend_name) == 0){
            //continue checking
            fprintf(stdout, "%s\n", friend_info);
            char message[50], response[10];
            for (int level = 1; level < 7; level++){
                bool there_are_no_children_on_this_level = true;
                for (int i = 0; i < friend_count; i++){
                    memset(message, 0, 50);
                    memset(response, 0, 10);
                    if (there_are_no_children_on_this_level) sprintf(message, "Print %d 1\n", level);
                    else sprintf(message, "Print %d 0\n", level);
                    fully_write(friends[i].write_fd, message, strlen(message));
                    
                    fgets(response, 10, friends[i].read_fp);//blocking read
                    response[strcspn(response, "\n")] = '\0';
                    if (strcmp(response, "found") == 0)
                        there_are_no_children_on_this_level = false;    
                }
                if (there_are_no_children_on_this_level)
                    break;
                fprintf(stdout, "\n");
            }

            fully_write(PARENT_WRITE_FD, "found\n", 6);
        }
        else{//pass down
            bool found_check_root = false;
            char response[10];
            for (int i = 0; i < friend_count; i++){
                memset(response, 0, 10);
                fully_write(friends[i].write_fd, full_cmd_forsend, strlen(full_cmd_forsend));

                fgets(response, 10, friends[i].read_fp);//blocking read
                response[strcspn(response, "\n")] = '\0';
                if (strcmp(response, "found") == 0){
                    found_check_root = true;
                    fully_write(PARENT_WRITE_FD, "found\n", 6);
                    break;
                }
            }
            if (!found_check_root)
                fully_write(PARENT_WRITE_FD, "no\n", 3);
        }
    }
    return true;
}

void die(char full_cmd[MAX_CMD_LEN]){//tell all children to die, close all fds to children, close 3,4 and exit
    for (int i = 0; i < friend_count; i++){
        fully_write(friends[i].write_fd, full_cmd, strlen(full_cmd));
        waitpid(friends[i].pid, NULL, 0);

        if (close(friends[i].read_fd) < 0) ERR_EXIT("close\n");
        if (close(friends[i].write_fd) < 0) ERR_EXIT("close\n");
        friends[i].read_fp = NULL;
    }
    if (close(PARENT_READ_FD) < 0) ERR_EXIT("close\n");
    if (close(PARENT_WRITE_FD) < 0) ERR_EXIT("close\n");
    // fprintf(stdout, "I am %s, I am dying\n", friend_name);
    exit(0);
}

void graduate(char full_cmd[MAX_CMD_LEN]){//graduate someone
    char full_cmd_forsend[MAX_CMD_LEN], cmd[MAX_CMD_LEN], root_of_graduate_name[MAX_FRIEND_NAME_LEN];
    memset(full_cmd_forsend, 0, MAX_CMD_LEN);
    memset(cmd, 0, MAX_CMD_LEN);
    memset(root_of_graduate_name, 0, MAX_FRIEND_NAME_LEN);

    strcpy(full_cmd_forsend, full_cmd);
    full_cmd[strcspn(full_cmd, "\r")] = '\0';
    full_cmd[strcspn(full_cmd, "\n")] = '\0';
    // printf("in %s received %s\n", friend_name, full_cmd);

    char *token = strtok(full_cmd, " ");
    if (token != NULL) {
        strcpy(cmd, token);           
        token = strtok(NULL, " ");   
    }
    if (token != NULL) {
        strcpy(root_of_graduate_name, token);
        token = strtok(NULL, " ");    
    }

    // printf("cmd is %s\n", cmd);
    // printf("%s is about to be graduated\n", root_of_graduate_name);

    //不會有要被graduate的人不存在的狀況
    if (is_Not_Tako()){
        if (strcmp(friend_name, root_of_graduate_name) == 0){//graduate everything, code ends here
            char message[50];
            memset(message, 0, 50);
            sprintf(message, "Die 1\n");
            for (int i = 0; i < friend_count; i++){
                fully_write(friends[i].write_fd, message, strlen(message));
                waitpid(friends[i].pid, NULL, 0);

                if (close(friends[i].read_fd) < 0) ERR_EXIT("close\n");
                if (close(friends[i].write_fd) < 0) ERR_EXIT("close\n");
                friends[i].read_fp = NULL;
            }

            print_final_graduate();
            exit(0);
        }
        else{//pass command down
            char response[10];
            for (int i = 0; i < friend_count; i++){
                memset(response, 0, 10);
                // fprintf(stdout, "Sending -%s- to %s\n", full_cmd_forsend, friends[i].name);
                fully_write(friends[i].write_fd, full_cmd_forsend, strlen(full_cmd_forsend));

                fgets(response, 10, friends[i].read_fp);//blocking read
                response[strcspn(response, "\n")] = '\0';
                // printf("Received -%s- from %s\n", response, friends[i].name);
                // fprintf(stdout, "Received -%s- from %s\n", response, friends[i].name);
                if (strcmp(response, "Idie") == 0){//direct child died, clean up
                    friends[i].read_fp = NULL;
                    if (close(friends[i].read_fd) < 0) ERR_EXIT("close\n");
                    if (close(friends[i].write_fd) < 0) ERR_EXIT("close\n");
                    for (int j = i; j < friend_count - 1; j++){
                        // friends[j - 1] = friends[j];
                        friends[j].read_fp = friends[j + 1].read_fp;
                        friends[j].pid = friends[j + 1].pid;
                        friends[j].read_fd = friends[j + 1].read_fd;
                        friends[j].write_fd = friends[j + 1].write_fd;
                        strcpy(friends[j].info, friends[j + 1].info);
                        strcpy(friends[j].name, friends[j + 1].name);
                        friends[j].value = friends[j + 1].value;
                    }
                    // friends[friend_count - 1].read_fp = NULL;
                    // if (close(friends[friend_count - 1].read_fd) < 0) ERR_EXIT("close\n");
                    // if (close(friends[friend_count - 1].write_fd) < 0) ERR_EXIT("close\n");
                    friend_count--;
                    break;
                }
                else if (strcmp(response, "found") == 0)
                    break;
            }
        }
    }
    else{//some random guy
        if (strcmp(friend_name, root_of_graduate_name) == 0){//graduate myself and children below, send found to parent and die myself
            // fprintf(stdout, "I am %s about to be graduated\n", friend_name);
            char message[50];
            memset(message, 0, 50);
            sprintf(message, "Die 1\n");
            // fprintf(stdout, "I am %s, I am killing all my children\n", friend_name);
            for (int i = 0; i < friend_count; i++){
                fully_write(friends[i].write_fd, message, strlen(message));
                waitpid(friends[i].pid, NULL, 0);

                if (close(friends[i].read_fd) < 0) ERR_EXIT("close\n");
                if (close(friends[i].write_fd) < 0) ERR_EXIT("close\n");
                friends[i].read_fp = NULL;
                // for (int j = i; j < friend_count - 1; j++){
                //     // friends[j - 1] = friends[j];
                //     friends[j].read_fp = friends[j + 1].read_fp;
                //     friends[j].pid = friends[j + 1].pid;
                //     friends[j].read_fd = friends[j + 1].read_fd;
                //     friends[j].write_fd = friends[j + 1].write_fd;
                //     strcpy(friends[j].info, friends[j + 1].info);
                //     strcpy(friends[j].name, friends[j + 1].name);
                //     friends[j].value = friends[j + 1].value;
                // }
            }
            fully_write(PARENT_WRITE_FD, "Idie\n", 5);
            exit(0);
        }
        else{//continue passing command down
            char response[10];
            bool found_guy_to_graduate = false;
            for (int i = 0; i < friend_count; i++){
                memset(response, 0, 10);
                fully_write(friends[i].write_fd, full_cmd_forsend, strlen(full_cmd_forsend));

                fgets(response, 10, friends[i].read_fp);//blocking read
                response[strcspn(response, "\n")] = '\0';
                if (strcmp(response, "Idie") == 0){//direct child died, clean up
                    friends[i].read_fp = NULL;
                    if (close(friends[i].read_fd) < 0) ERR_EXIT("close\n");
                    if (close(friends[i].write_fd) < 0) ERR_EXIT("close\n");
                    for (int j = i; j < friend_count - 1; j++){
                        // friends[j - 1] = friends[j];
                        friends[j].read_fp = friends[j + 1].read_fp;
                        friends[j].pid = friends[j + 1].pid;
                        friends[j].read_fd = friends[j + 1].read_fd;
                        friends[j].write_fd = friends[j + 1].write_fd;
                        strcpy(friends[j].info, friends[j + 1].info);
                        strcpy(friends[j].name, friends[j + 1].name);
                        friends[j].value = friends[j + 1].value;
                    }
                    // friends[friend_count - 1].read_fp = NULL;
                    // if (close(friends[friend_count - 1].read_fd) < 0) ERR_EXIT("close\n");
                    // if (close(friends[friend_count - 1].write_fd) < 0) ERR_EXIT("close\n");
                    friend_count--;
                    found_guy_to_graduate = true;
                    break;
                }
                else if (strcmp(response, "found") == 0){
                    found_guy_to_graduate = true;
                    break;
                }
            }
            if (!found_guy_to_graduate)
                fully_write(PARENT_WRITE_FD, "no\n", 3);
            else
                fully_write(PARENT_WRITE_FD, "found\n", 6);
        }
    }
    return;
}

int main(int argc, char *argv[]) {
    // ERR_EXIT("write");
    process_pid = getpid(); // you might need this when using fork()
    friend_count = 0;
    if (argc != 2) {
        fprintf(stderr, "Usage: ./friend [friend_info]\n");
        return 0;
    }
    setvbuf(stdout, NULL, _IONBF, 0);  //prevent buffered I/O, equivalent to fflush() after each stdout, study this as you may need to do it for other friends against their parents
    
    // put argument one into friend_info
    char friend_info_forcut[MAX_FRIEND_INFO_LEN];
    memset(friend_info_forcut, 0, MAX_FRIEND_INFO_LEN);
    strncpy(friend_info, argv[1], MAX_FRIEND_INFO_LEN);
    strncpy(friend_info_forcut, argv[1], MAX_FRIEND_INFO_LEN);
    if(strcmp(argv[1], root) == 0){ // is Not_Tako
        strncpy(friend_name, friend_info, MAX_FRIEND_NAME_LEN);      // put name into friend_name
        friend_name[MAX_FRIEND_NAME_LEN - 1] = '\0';        // in case strcmp messes with you
        Pread_fp = stdin;        // takes commands from stdin
        friend_value = 100;     // Not_Tako adopting nodes will not mod their values
    }
    else{
        char *token = strtok(friend_info_forcut, "_");
        strcpy(friend_name, token);
        friend_name[MAX_FRIEND_NAME_LEN - 1] = '\0';
        token = strtok(NULL, "_");
        friend_value = atoi(token);
        Pread_fp = fdopen(PARENT_READ_FD, "r"); //write with fully_write
        // where do you read from?
        // anything else you have to do before you start taking commands?
    }

    while(1){//repeatedly waiting for commands
        char full_cmd[MAX_CMD_LEN], full_cmd_temp[MAX_CMD_LEN], cmd[MAX_CMD_LEN];
        memset(full_cmd, 0, MAX_CMD_LEN);
        memset(full_cmd_temp, 0, MAX_CMD_LEN);//full_cmd 是原本那個
        memset(cmd, 0, MAX_CMD_LEN);
        fgets(full_cmd, MAX_CMD_LEN, Pread_fp);
        strcpy(full_cmd_temp, full_cmd);

        // fprintf(stdout, "In %s received -%s-\n", friend_name, full_cmd);

        char *token = strtok(full_cmd_temp, " ");
        strcpy(cmd, token);
        token = strtok(NULL, " ");

        if (strcmp(cmd, "Hi") == 0 && is_Not_Tako()){
            for (int i = 0; i < friend_count; i++){
                printf("My %d child is %s\n", i, friends[i].info);
                printf("Read_fd %d Write_fd %d\n", friends[i].read_fd, friends[i].write_fd);
            }
        }
        

        if (strcmp(cmd, "Meet") == 0){
            meet(full_cmd);
        }
        else if (strcmp(cmd, "Check") == 0){
            check(full_cmd);
        }
        else if (strcmp(cmd, "Print") == 0){
            print_self(full_cmd);
        }
        else if (strcmp(cmd, "Graduate") == 0){
            // printf("Hi\n");
            char full_cmd_second_copy[MAX_CMD_LEN];
            memset(full_cmd_second_copy, 0, MAX_CMD_LEN);
            strcpy(full_cmd_second_copy, full_cmd);
            if (!check(full_cmd)) //不存在那個人
                continue;
            graduate(full_cmd_second_copy);    
        }
        else if (strcmp(cmd, "Die") == 0){
            die(full_cmd);
        }
        else if (strcmp(cmd, "Adopt") == 0){
            //等等寫
        }
        else{
            fprintf(stderr, "%s received error input : %s\n", friend_name, full_cmd); // use this to print out what you received
        }
    }

    // Hint: do not return before receiving the command "Graduate"
    // please keep in mind that every process runs this exact same program, so think of all the possible cases and implement them

    /* pseudo code
    if(Meet){
        create array[2]
        make pipe
        use fork.
            Hint: remember to fully understand how fork works, what it copies or doesn't
        check if you are parent or child
        as parent or child, think about what you do next.
            Hint: child needs to run this program again
    }
    else if(Check){
        obtain the info of this subtree, what are their info?
        distribute the info into levels 1 to 7 (refer to Additional Specifications: subtree level <= 7)
        use above distribution to print out level by level
            Q: why do above? can you make each process print itself?
            Hint: we can only print line by line, is DFS or BFS better in this case?
    }
    else if(Graduate){
        perform Check
        terminate the entire subtree
            Q1: what resources have to be cleaned up and why?
            Hint: Check pseudo code above
            Q2: what commands needs to be executed? what are their orders to avoid deadlock or infinite blocking?
            A: (tell child to die, reap child, tell parent you're dead, return (die))
    }
    else if(Adopt){
        remember to make fifo
        obtain the info of child node subtree, what are their info?
            Q: look at the info you got, how do you know where they are in the subtree?
            Hint: Think about how to recreate the subtree to design your info format
        A. terminate the entire child node subtree
        B. send the info through FIFO to parent node
            Q: why FIFO? will usin pipe here work? why of why not?
            Hint: Think about time efficiency, and message length
        C. parent node recreate the child node subtree with the obtained info
            Q: which of A, B and C should be done first? does parent child position in the tree matter?
            Hint: when does blocking occur when using FIFO?(mkfifo, open, read, write, unlink)
        please remember to mod the values of the subtree, you may use bruteforce methods to do this part (I did)
        also remember to print the output
    }
    else if(full_cmd[1] == 'o'){
        Bonus has no hints :D
    }
    else{
        there's an error, we only have valid commmands in the test cases
        fprintf(stderr, "%s received error input : %s\n", friend_name, full_cmd); // use this to print out what you received
    }a
    */

    // final print, please leave this in, it may bepart of the test case output
    // if (is_Not_Tako()){
    //     print_final_graduate();
    // }
    return 0;
}