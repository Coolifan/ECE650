//
//  potato.h
//  HW3
//
//  Created by Yifan Li on 2/17/18.
//  Copyright © 2018 Yifan Li. All rights reserved.
//

#ifndef potato_h
#define potato_h

struct client {
    int playerID;
    char hostname[128];
    int masterSocket;
    int listeningPort;
};


#endif /* potato_h */
