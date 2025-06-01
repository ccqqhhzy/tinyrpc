// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#include "util.h"
#include "log.h"

using namespace tinyrpc;

int Util::set_fl(int fd, int flags) {
    int oldflag = fcntl(fd, F_GETFL);

    if (-1 == oldflag)
        return -1;

    int newflag = oldflag | flags; // or oldflag |= flags;

    return fcntl(fd, F_SETFL, newflag);
}

int Util::clear_fl(int fd, int flags) {
    int oldflag = fcntl(fd, F_GETFL);

    if (-1 == oldflag)
        return -1;

    int newflag = oldflag & ~flags; // or oldflag &= ~flags;

    return fcntl(fd, F_SETFL, newflag);
}

sighandler_t Util::registerSignal(int signo, sighandler_t func, 
                                  bool is_restart) {
    struct sigaction newsig, oldsig;

    sigemptyset(&newsig.sa_mask);
    //sigaddset(&newsig.sa_mask, signo); //set block mask
    newsig.sa_handler = func;
    newsig.sa_flags = 0;

    if (is_restart) {
#ifdef SA_RESTART    
        newsig.sa_flags |= SA_RESTART;
#endif
    }
        
    if (-1 == sigaction(signo, &newsig, &oldsig)) {
        LOG(Error, "sigaction failed");
        return SIG_ERR;
    }
        
    return oldsig.sa_handler;
}
