#ifndef ENUMS_H
#define ENUMS_H

typedef enum {
    ASSEMBLE = 1,
    LINK = 2,
    MERGE = 4,
} jobs_t;

typedef enum {
    OBJECT,
    EXECUTABLE
} output_type_t;

#endif
