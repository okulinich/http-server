#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <bits/sigaction.h>

/**
 * @desc checks specified path for 
 * wildcards, because the user is not 
 * allowed to go out from server 
 * working directory
 * @params path - c-string with path
 * @return true if string don't include
 * wildcards, false otherwise
*/
bool checkForWildcards(const char *uri) {
    if(!uri || strlen(uri) == 0) 
        return false;
    if(strcmp(uri, "/") == 0)
        return true;
    for(int i = 1; i < strlen(uri); i++) {
        if ((uri[i] == '.' && uri[i - 1] == '.') 
            || uri[i] == '~')
            return false;
    }
    return true;
}

/**
 * @desc stores bool variable that defines if
 * signal INTERRUPT arrived
 * @params flag - bool value, pass false to
 * find out wether sigint arrived or not,
 * pass true to indicate that signal arrived
 * @return true if sigint arrived, false
 * otherwise
*/
bool sigintSent(bool flag) {
    static bool sigint = false;

    if(flag)
        sigint = true;
    return sigint;
}

void sigintHandler(int num) {
    sigintSent(true);
}

void handleSignalInt() {
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigintHandler;

    sigaction(SIGINT, &sa, NULL);
}
