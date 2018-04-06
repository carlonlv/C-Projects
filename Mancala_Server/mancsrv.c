#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXNAME 80  /* maximum permitted name size, not including \0 */
#define NPITS 6  /* number of pits on a side, not including the end pit */
#define NPEBBLES 4 /* initial number of pebbles per pit */
#define MAXMESSAGE (MAXNAME + 50) /* initial number of pebbles per pit */

int port = 30845;
int listenfd;

struct player {
    int fd;
    char name[MAXNAME+1];
    int pits[NPITS+1];  // pits[0..NPITS-1] are the regular pits
                        // pits[NPITS] is the end pit
    int is_my_turn;
    char input[MAXMESSAGE];
    int input_status;
    struct player *next;
};
struct player *playerlist = NULL;

extern void parseargs(int argc, char **argv);
extern void makelistener();
extern int compute_average_pebbles();
extern int game_is_over();  /* boolean */
extern void distribute_pebbles(struct player *current_player, int *pit, int *pebbles);
extern int move_pebbles(struct player *current_player, int pit);
extern void broadcast_to_single_person(int fd, char *msg);
extern void broadcast(char *s);  /* you need to write this one */
extern int promp_to_new_connection(int listenfd);
extern int add_new_player(int new_player_fd);
extern int read_from_current_turn_player(struct player *current_turn_player);
extern int reread_name_from_player(struct player *player);
extern void valid_next_turn_player(struct player *player);
extern void display();
extern int read_from_other_player(int fd);
extern void fix_connection(int fd);
extern int validate_username(char *user_name);
extern void display_turn();
extern int parse_client_input_name(char *client_input);
extern int has_valid_player();
extern void free_all_player(struct player *player);


int main(int argc, char **argv) {
    char msg[MAXMESSAGE];
    struct player *current_player;
    int current_turn_player_break_line = 1;

    parseargs(argc, argv);
    makelistener();

    fd_set all_player_fds;
    FD_ZERO(&all_player_fds);
    FD_SET(listenfd, &all_player_fds);
    int max_fd = listenfd;

    while (!game_is_over()) {
        fd_set copy_fds = all_player_fds;

        int ready = select(max_fd + 1, &copy_fds, NULL, NULL, NULL);
        if (ready == -1) {
            perror("server: select");
            exit(1);
        }

        //sock_fd is ready to write, meaning there is new connection on hold
        int success;
        if (FD_ISSET(listenfd, &copy_fds)) {
            success = promp_to_new_connection(listenfd);
            if (success != -1) {
                if (success > max_fd) {
                    max_fd = success;
                }
                FD_SET(success, &all_player_fds);
                printf("Accepted connection from new_player with fd %d\n", success);
                int status = add_new_player(success);
                if (status > 0) {
                    FD_CLR(status, &all_player_fds);
                    char msg[MAXMESSAGE];
                    sprintf(msg, "A player with fd %d has disconnected.\r\n", status);
                    printf("A player with fd %d has disconnected.\n", status);
                }
            } else {
                exit(1);
            }
        }

        current_player = playerlist;
        while (current_player != NULL) {
            if (FD_ISSET(current_player->fd, &copy_fds)) {
                //The case where the player types in other player's turn
                if (current_player->is_my_turn == 0) {
                    int status = read_from_other_player(current_player->fd);
                    if (status > 0){
                        char msg[MAXMESSAGE];
                        sprintf(msg, "%s has leaved the session.\r\n", current_player->name);
                        printf("%s has leaved the session.\n", current_player->name);
                        fix_connection(status);
                        FD_CLR(status, &all_player_fds);
                        free(current_player);
                        broadcast(msg);
                    } else {
                        char *message = "It's not your turn. Please wait.\r\n";
                        broadcast_to_single_person(current_player->fd, message);
                    }
                } else if (current_player->is_my_turn == -1 || current_player->is_my_turn == -2) {
                    int status = reread_name_from_player(current_player);
                    if (status > 0) {
                        printf("A player with fd %d has disconnected.\n", status);
                        fix_connection(status);
                        FD_CLR(status, &all_player_fds);
                        free(current_player);
                    }
                } else {
                    char *message;
                    char msg[MAXMESSAGE];
                    int pit_num = read_from_current_turn_player(current_player);
                    if (pit_num >= 0) {
                        current_turn_player_break_line = 1;
                        int result = move_pebbles(current_player, pit_num);
                        if (result == -1) {
                            message = "The pit you selected is empty. Please select from a different pit\r\n";
                            broadcast_to_single_person(current_player->fd, message);
                        } else if (result == 1) {
                            message = "You get an extra turn.\r\n";
                            sprintf(msg, "%s has cleared out %d pit and gets an extra turn!\r\n", current_player->name, pit_num);
                            printf("%s has cleared out %d pit and gets an extra turn!\r\n", current_player->name, pit_num);
                            broadcast(msg);
                            broadcast_to_single_person(current_player->fd, message);
                        } else {
                            current_player->is_my_turn = 0;
                            sprintf(msg, "%s has cleared out %d pit\r\n", current_player->name, pit_num);
                            printf("%s has cleared out %d pit\r\n", current_player->name, pit_num);
                            broadcast(msg);
                            valid_next_turn_player(current_player->next);
                        }
                    } else if (pit_num == -2) {
                        current_turn_player_break_line = 1;
                        char msg[MAXMESSAGE];
                        sprintf(msg, "%s has leaved the session in this turn.\r\n", current_player->name);
                        printf("%s has leaved the session in this turn.\n", current_player->name);
                        fix_connection(current_player->fd);
                        valid_next_turn_player(current_player->next);
                        FD_CLR(current_player->fd, &all_player_fds);
                        free(current_player);
                        broadcast(msg);
                    } else if (pit_num == -1) {
                        current_turn_player_break_line = 1;
                        message = "Please enter a pit number from 0 to 5\r\n";
                        broadcast_to_single_person(current_player->fd, message);
                    } else {
                        current_turn_player_break_line = 0;
                    }
                }
            }
            current_player = current_player->next;
        }

        if (has_valid_player() && current_turn_player_break_line) {
            display_turn();
            display();
        }
    }

    broadcast("Game over!\r\n");
    printf("Game over!\n");
    for (struct player *p = playerlist; p; p = p->next) {
        int points = 0;
        for (int i = 0; i <= NPITS; i++) {
            points += (p->pits)[i];
        }
        printf("%s has %d points\r\n", p->name, points);
        snprintf(msg, MAXMESSAGE, "%s has %d points\r\n", p->name, points);
        broadcast(msg);
    }
    free_all_player(playerlist);
    return 0;
}


void parseargs(int argc, char **argv) {
    int c, status = 0;
    while ((c = getopt(argc, argv, "p:")) != EOF) {
        switch (c) {
        case 'p':
            port = strtol(optarg, NULL, 0);
            break;
        default:
            status++;
        }
    }
    if (status || optind != argc) {
        fprintf(stderr, "usage: %s [-p port]\n", argv[0]);
        exit(1);
    }
}


void makelistener() {
    struct sockaddr_in r;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    int on = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
               (const char *) &on, sizeof(on)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    memset(&r, '\0', sizeof(r));
    r.sin_family = AF_INET;
    r.sin_addr.s_addr = INADDR_ANY;
    r.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr *)&r, sizeof(r))) {
        perror("bind");
        exit(1);
    }

    if (listen(listenfd, 5)) {
        perror("listen");
        exit(1);
    }
}



/* call this BEFORE linking the new player in to the list */
int compute_average_pebbles() {
    struct player *p;
    int i;

    if (playerlist == NULL) {
        return NPEBBLES;
    }

    int nplayers = 0, npebbles = 0;
    for (p = playerlist; p; p = p->next) {
        nplayers++;
        for (i = 0; i < NPITS; i++) {
            npebbles += (p->pits)[i];
        }
    }
    return ((npebbles - 1) / nplayers / NPITS + 1);  /* round up */
}


int game_is_over() { /* boolean */
    int i;

    if (!playerlist) {
       return 0;  /* we haven't even started yet! */
    }

    for (struct player *p = playerlist; p; p = p->next) {
        int is_all_empty = 1;
        for (i = 0; i < NPITS; i++) {
            if (p->pits[i]) {
                is_all_empty = 0;
            }
        }
        if (is_all_empty) {
            return 1;
        }
    }
    return 0;
}


void distribute_pebbles(struct player *current_player, int *pit, int *pebbles){
    while (*pebbles > 0 && *pit <= NPITS) {
        (current_player->pits)[*pit] += 1;
        *pebbles -= 1;
        *pit += 1;
    }
}


int move_pebbles(struct player *current_player, int pit) {
    int pebbles = current_player->pits[pit];
    if (pebbles == 0) {
        //The case where user chooses an empty pit
        return -1;
    }
    (current_player->pits)[pit] = 0;
    pit = pit + 1;
    while (pebbles > 0) {
        distribute_pebbles(current_player, &pit, &pebbles);
        if (pebbles == 0) {
            pit -= 1;
            break;
        }
        if (current_player-> next != NULL) {
            current_player = current_player->next;
        }else {
            current_player = playerlist;
        }
        pit = 0;
    }
    if (pit == NPITS) {
        //The case where user gets an extra turn
        return 1;
    } else {
        //The case where user moves normally
        return 0;
    }
}


/*Send message to a particular fd.*/
void broadcast_to_single_person(int fd, char *msg) {
    if (write(fd, msg, strlen(msg)) == -1) {
        perror("write in broadcast_to_single_person");
    }
}


/*Broadcast message to all valid players.*/
void broadcast(char* s) {
    struct player *current = playerlist;
    while (current != NULL) {
        if (current->is_my_turn >= 0) {
            if (write(current->fd, s, strlen(s)) == -1) {
                perror("write in broadcast");
            }
        }
        current = current->next;
    }
}


/*Accept new connections and send greetings signal and ask for their name.
    returns -1 if there is a problem in accept
    returns the new player's fd upon success
*/
int promp_to_new_connection(int listenfd) {
    int new_player_fd = accept(listenfd, NULL, NULL);
    if (new_player_fd < 0) {
        perror("accept in accept_connection_from_new_player");
        close(listenfd);
        return -1;
    }
    char *message = "Welcome to Mancala. What is your name?\r\n";
    broadcast_to_single_person(new_player_fd, message);
    return new_player_fd;
}


/*Record the name of the new player and initialize the new player.
    returns the new player's fd if the new player is disconnected
    return 0 otherwise.
*/
int add_new_player(int new_player_fd) {
    char buf[MAXNAME + 2];
    memset(buf, 0, (MAXNAME + 2) * sizeof(char));
    int n = read(new_player_fd, buf, MAXNAME + 2);
    if (n == 0) {
        char msg[MAXMESSAGE];
        sprintf(msg, "A player with fd %d has disconnected.\r\n", new_player_fd);
        printf("A player with fd %d has disconnected.\n", new_player_fd);
        return new_player_fd;
    }
    struct player *new_player = malloc(sizeof(struct player));
    int location = parse_client_input_name(buf);
    if (location == -1) {
        if (n <= MAXNAME) {
            buf[n] = '\0';
            new_player->is_my_turn = -2;
            strcpy(new_player->name, buf);
        } else {
            free(new_player);
            printf("Player with fd %d is forced to disconnect: name too long.\n", new_player_fd);
            close(new_player_fd);
            return new_player_fd;
        }
    } else {
        buf[location] = '\0';
        if (location <= MAXNAME) {
            if(validate_username(buf) == 1){
                new_player->is_my_turn = -1;
                char *message = "Name is taken, plase re-enter your name.\r\n";
                broadcast_to_single_person(new_player_fd, message);
            } else {
                if (!has_valid_player()) {
                    new_player->is_my_turn = 1;
                } else {
                    new_player->is_my_turn = 0;
                }
                strcpy(new_player->name, buf);
                char buffer[MAXMESSAGE];
                sprintf(buffer, "Player %s has joined the session.\r\n", buf);
                broadcast(buffer);
                printf("Player %s has joined the session.\n", buf);
            }
        } else {
            free(new_player);
            printf("Player with fd %d is forced to disconnect: name too long.\n", new_player_fd);
            close(new_player_fd);
            return new_player_fd;
        }
    }
    int average_pebbles_for_new_player = compute_average_pebbles();
    for (int i = 0; i < NPITS; i++) {
        (new_player->pits)[i] = average_pebbles_for_new_player;
    }
    (new_player->pits)[NPITS] = 0;
    new_player->fd = new_player_fd;
    new_player->next = playerlist;
    new_player->input_status = 0;
    playerlist = new_player;
    return 0;
}


/*Read from the current turn player
    returns -2 if the current turn player disconnects
    returns -1 if the current turn player input is invalid
    returns the pit number upon success
    */
int read_from_current_turn_player(struct player *current_turn_player) {
    if (current_turn_player->input_status == 0) {
        char move[MAXMESSAGE];
        memset(move, 0, MAXMESSAGE * sizeof(char));
        int n = read(current_turn_player->fd, move, MAXMESSAGE);
        if (n == 0) {
            return -2;
        }
        int location = parse_client_input_name(move);
        if (location == -1) {
            move[n] = '\0';
            current_turn_player->input_status = 1;
            strcpy(current_turn_player->input, move);
            return -3;
        } else {
            move[location] = '\0';
            int pit_num = strtol(move, NULL, 10);
            if (pit_num < 0 || pit_num > 5) {
                return -1;
            } else {
                return pit_num;
            }
        }
    } else {
        char move[MAXMESSAGE];
        memset(move, 0, MAXMESSAGE * sizeof(char));
        int n = read(current_turn_player->fd, move, MAXMESSAGE);
        if (n == 0) {
            return -2;
        }
        int location = parse_client_input_name(move);
        if (location == -1) {
            move[n] = '\0';
            strcat(current_turn_player->input, move);
            return -3;
        } else {
            move[location] = '\0';
            current_turn_player->input_status = 0;
            strcat(current_turn_player->input, move);
            int pit_num = strtol(current_turn_player->input, NULL, 10);
            if (pit_num < 0 || pit_num > 5) {
                return -1;
            } else {
                return pit_num;
            }
        }
    }
}


/*Stores the input from client into their name based on two conditions:
    1.The client did not provide a valid input last time so that they need to provide a new name
    2.The client did not finish inputting their name last time so that they need to provide the remaining or part of the remaining name
    returns the client's fd if the client disconnects
    returns 0 otherwise.
*/
int reread_name_from_player(struct player *player) {
    char buf[MAXNAME + 2];
    memset(buf, 0, (MAXNAME + 2) * sizeof(char));
    int n = read(player->fd, buf, MAXNAME + 2);
    if (n == 0) {
        return player->fd;
    }
    int location = parse_client_input_name(buf);
    if (player->is_my_turn == -1) {
        //The case where the player's input name is illegal
        if (location != -1) {
            //The case where the player's new input is finished
            buf[location] = '\0';
            char *message = "Name is taken, plase re-enter your name.\r\n";
            if(validate_username(buf) == 1){
                broadcast_to_single_person(player->fd, message);
            } else {
                if (!has_valid_player()) {
                    player->is_my_turn = 1;
                } else {
                    player->is_my_turn = 0;
                }
                strcpy(player->name, buf);
                char msg[MAXMESSAGE];
                sprintf(msg, "Player %s has joined the session.\r\n", player->name);
                broadcast(msg);
                printf("Player %s has joined the session.\n", player->name);
            }
        } else {
            //The case where the player is not finished inputting
            if (n <= MAXNAME) {
                player->is_my_turn = -2;
                buf[n] = '\0';
                strcpy(player->name, buf);
            } else {
                //The case the player exceeds max length and name
                printf("Player with fd %d is forced to disconnect: name too long.\n", player->fd);
                close(player->fd);
                return player->fd;
            }
        }
    } else {
        //The case where the player is not done inputting name
        char buffer[MAXNAME + 2];
        memset(buffer, 0, (MAXNAME + 2) * sizeof(char));
        strcpy(buffer, player->name);
        if (location != -1) {
            //The case where the player's new input is finished
            location += strlen(buffer);
            strcat(buffer, buf);
            if (location <= MAXNAME) {
                buffer[location] = '\0';
            } else {
                printf("Player with fd %d is forced to disconnect: name too long.\n", player->fd);
                close(player->fd);
                return player->fd;
            }
            if (validate_username(buffer) == 1) {
                //The case where the player's new input is not legal
                char *message = "Name is taken, plase re-enter your name.\r\n";
                broadcast_to_single_person(player->fd, message);
                player->is_my_turn = -1;
            } else {
                if (!has_valid_player()) {
                    player->is_my_turn = 1;
                } else {
                    player->is_my_turn = 0;
                }
                strcpy(player->name, buffer);
                char msg[MAXMESSAGE];
                sprintf(msg, "Player %s has joined the session.\r\n", player->name);
                broadcast(msg);
                printf("Player %s has joined the session.\n", player->name);
            }

        } else {
            //The case where the player is still not finished inputting
            if ((strlen(buffer) + n) <= MAXNAME) {
                strcat(buffer, buf);
                buffer[strlen(buffer) + n] = '\0';
                strcpy(player->name, buffer);
            } else {
                //The case the player exceeds max length and name
                printf("Player with fd %d is forced to disconnect: name too long.\n", player->fd);
                close(player->fd);
                return player->fd;
            }
        }
    }
    return 0;
}


void valid_next_turn_player(struct player *player) {
    if (player != NULL && player->is_my_turn >= 0) {
        player->is_my_turn = 1;
    } else if (player == NULL) {
        if (playerlist != NULL) {
            valid_next_turn_player(playerlist);
        }
    }else {
        valid_next_turn_player(player->next);
    }
}


void display() {
    struct player *i = playerlist;
    char board[MAXMESSAGE];
    char buffer[MAXMESSAGE];
    while ((i->next) != NULL) {
        if (i->is_my_turn >= 0) {
            sprintf(buffer, "%s: ", i->name);
            strcpy(board, buffer);
            for (int p = 0; p < NPITS; p++) {
                sprintf(buffer, "[%d]%d ", p, (i->pits)[p]);
                strcat(board, buffer);
            }
            sprintf(buffer, "[end pit]%d \n", (i->pits)[NPITS]);
            strcat(board, buffer);
            broadcast(board);
        }
        i = i->next;
    }
    if (i->is_my_turn >= 0) {
        sprintf(buffer, "%s: ", i->name);
        strcpy(board, buffer);
        for (int p = 0; p < NPITS; p++) {
            sprintf(buffer, "[%d]%d ", p, (i->pits)[p]);
            strcat(board, buffer);
        }
        sprintf(buffer, "[end pit]%d \r\n", (i->pits)[NPITS]);
        strcat(board, buffer);
        broadcast(board);
    }
}


int read_from_other_player(int fd) {
    char buf[MAXMESSAGE];
    memset(buf, 0, MAXMESSAGE * sizeof(char));
    int num_read = read(fd, &buf, MAXMESSAGE);
    buf[num_read] = '\0';
    if (num_read == 0) {
        return fd;
    } else {
        return 0;
    }
}


void fix_connection(int fd) {
    struct player *player = playerlist;
    if (player->fd != fd) {
        while ((player->next)->fd != fd) {
            player = player->next;
        }
        player->next = (player->next)->next;
    } else {
        //The case where the disconnected player is the first one of playerlist
        playerlist = player->next;
    }
}



/*Check if the username has already been provided by some other player
    returns 1 if the username is taken
    returns 0 otherwise
*/
int validate_username(char *user_name) {
    struct player *current = playerlist;
    int result = 0;
    while (current != NULL) {
        if (current->is_my_turn >= 0) {
            if (strcmp(user_name, current->name) == 0) {
                result = 1;
                break;
            }
        }
        current = current->next;
    }
    return result;
}


/*Broadcast whose turn it is.*/
void display_turn() {
    struct player *player = playerlist;
    int is_valid = 0;
    while (player != NULL) {
        if (player->is_my_turn == 1) {
            is_valid = 1;
            break;
        }
        player = player->next;
    }
    if (is_valid) {
        char *message = "It's your turn.\r\n";
        broadcast_to_single_person(player->fd, message);
        char msg[MAXMESSAGE];
        sprintf(msg, "It's %s's turn!\r\n", player->name);
        broadcast(msg);
    }
}


/*This function attempts to find /r, /n or /r/n in client's input
    returns the index of above terminator if any of those terminators exist
    returns -1 if none of those terminators are found
*/
int parse_client_input_name(char *client_input) {
    char new_line_1 = '\r';
    char new_line_2 = '\n';
    char buf[MAXMESSAGE];
    strcpy(buf, client_input);
    char *ptr;
    int location;
    if ((ptr = strchr(buf, new_line_1)) != NULL) {
        location = ptr - buf;
        return location;
    } else if ((ptr = strchr(buf, new_line_2)) != NULL) {
        location = ptr - buf;
        return location;
    } else {
        return -1;
    }
}


int has_valid_player() {
    struct player *player = playerlist;
    while (player != NULL) {
        if (player->is_my_turn >= 0) {
            return 1;
        }
        player = player->next;
    }
    return 0;
}


/*Free all player in struct recursively without accessing dangling pointer.*/
void free_all_player(struct player *player) {
    if (player != NULL) {
        if (player->next == NULL) {
            free(player);
        } else {
            free_all_player(player->next);
            free(player);
        }
    }
}
