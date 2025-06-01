// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __UTIL_H__ 
#define __UTIL_H__

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>


/*
#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while(0)
    
#define handle_error_en(en, msg) \
    do { errno = en; perror(msg); exit(EXIT_FAILURE); } while(0)
*/

namespace tinyrpc {

typedef void (*SigFunc)(int);

class Util {
public:
    static sighandler_t registerSignal(int signo, sighandler_t func, 
                                       bool is_restart = false);

    // set file flags by fcntl, eg: set_fl(sockfd, O_NONBLOCK);
    static int set_fl(int fd, int flags);

    // clear file flags by fcntl, eg: clr_fl(sockfd, O_NONBLOCK);
    static int clear_fl(int fd, int flags);
};

} // namespace tinyrpc

#endif /*__UTIL_H__*/
