//
//  PPEncoder_C_Interface
//  PPEncoder
//
//  Created by 邱开禄 on 2019/01/05.
//  Copyright © 2019年 邱开禄. All rights reserved.
//

#ifndef PPEncoder_C_Interface_H
#define PPEncoder_C_Interface_H
#include "Message.h"
NS_MEDIA_BEGIN
typedef void (*msg_loop) (void* playerInstance, Message &msg);
NS_MEDIA_END
#endif
