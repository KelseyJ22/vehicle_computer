/* Automatically generated nanopb header */
/* Generated by nanopb-0.3.6 at Tue Nov 15 11:16:47 2016. */

#ifndef PB_TEST2_PB_H_INCLUDED
#define PB_TEST2_PB_H_INCLUDED
#include <pb.h>

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Struct definitions */
typedef struct _NestedMessage {
    pb_callback_t variable;
    bool has_timestamp;
    uint32_t timestamp;
/* @@protoc_insertion_point(struct:NestedMessage) */
} NestedMessage;

typedef struct _NestedMessage_Variable {
    bool has_val;
    uint32_t val;
    bool has_devid;
    uint32_t devid;
    bool has_varid;
    uint32_t varid;
/* @@protoc_insertion_point(struct:NestedMessage_Variable) */
} NestedMessage_Variable;

/* Default values for struct fields */

/* Initializer values for message structs */
#define NestedMessage_init_default               {{{NULL}, NULL}, false, 0}
#define NestedMessage_Variable_init_default      {false, 0, false, 0, false, 0}
#define NestedMessage_init_zero                  {{{NULL}, NULL}, false, 0}
#define NestedMessage_Variable_init_zero         {false, 0, false, 0, false, 0}

/* Field tags (for use in manual encoding/decoding) */
#define NestedMessage_variable_tag               1
#define NestedMessage_timestamp_tag              2
#define NestedMessage_Variable_val_tag           1
#define NestedMessage_Variable_devid_tag         2
#define NestedMessage_Variable_varid_tag         3

/* Struct field encoding specification for nanopb */
extern const pb_field_t NestedMessage_fields[3];
extern const pb_field_t NestedMessage_Variable_fields[4];

/* Maximum encoded size of messages (where known) */
/* NestedMessage_size depends on runtime parameters */
#define NestedMessage_Variable_size              18

/* Message IDs (where set with "msgid" option) */
#ifdef PB_MSGID

#define TEST2_MESSAGES \


#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
/* @@protoc_insertion_point(eof) */

#endif
