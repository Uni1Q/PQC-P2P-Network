//
// Created by rokas on 02/09/2024.
//

#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include "../../DataStructures/Dictionary/Dictionary.h"


struct HTTPRequest
{
    struct Dictionary request_line;
    struct Dictionary header_fields;
    struct Dictionary body;
};

struct HTTPRequest http_request_constructor(char *request_string);
void http_request_destructor(struct HTTPRequest *request);


#endif //HTTPREQUEST_H
