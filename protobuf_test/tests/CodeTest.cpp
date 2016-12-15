extern "C"
{
#include "test.pb.h"
#include "test2.pb.h"
#include "lib_pb.h"
}

#include "CppUTest/TestHarness.h"

NestedMessage_Variable raw_variables[] = {
  {true, 10, true, 20, true, 30},
  {false, 0, false, 0, false, 0},
  {true, 100, false, 0, true, 100}
};

void* variable_refs[] = {
  (void *) &raw_variables[0],
  (void *) &raw_variables[1],
  (void *) &raw_variables[2]
};
  
PBInnerField input_field = {
  NestedMessage_Variable_fields,
  variable_refs,
  3
};

NestedMessage_Variable new_variables[] = {
  NestedMessage_Variable_init_default,
  NestedMessage_Variable_init_default,
  NestedMessage_Variable_init_default
};

void* new_refs[] = {
  (void *) &new_variables[0],
  (void *) &new_variables[1],
  (void *) &new_variables[2],
};
  
PBInnerField output_field = {
  NestedMessage_Variable_fields,
  new_refs,
  0
};

TEST_GROUP(ProtobufTestGroup)
{
  void setup() {}
  void teardown() {}
};

/* SimpleProtobufEncodeDecodeTest
 *
 * Takes a message, encodes, decodes, and checks the result.
 */
TEST(ProtobufTestGroup, SimpleProtobufEncodeDecodeTest)
{
  char buffer[128];
  size_t message_length;

  TestMessage message = TestMessage_init_default;
  message.a = 2016;
  message.has_b = true;
  message.b = 6102;
  message_length = SerializeProtobuf(&message, TestMessage_fields, buffer, sizeof(buffer));
  
  // Send buffer and message_length over the wire...
  
  TestMessage received = TestMessage_init_default;
  bool status = DeserializeProtobuf(&received, TestMessage_fields, buffer, sizeof(buffer));
  if (status) {
    CHECK_EQUAL(received.a, 2016);
    CHECK_EQUAL(received.b, 6102);
  } else {
    FAIL("Error: Received test message did not match expected\n");
  }
}

/* ComplexProtobufEncodeDecodeTest
 *
 * Takes a complex message, encodes, decodes, and checks the result.
 * Nested fields are annoying to deal with...
 */
TEST(ProtobufTestGroup, ComplexProtobufEncodeDecodeTest)
{
  char buffer[128];
  size_t message_length;

  NestedMessage message = NestedMessage_init_default;
  message.has_timestamp = true;
  message.timestamp = 2016;
  message.variable.funcs.encode = &PB_encode_callback;
  message.variable.arg = (void *) &input_field;
  
  message_length = SerializeProtobuf(&message, NestedMessage_fields, buffer, sizeof(buffer));
  
  // Send buffer and message_length over the wire...
  
  NestedMessage received = NestedMessage_init_default;
  received.variable.funcs.decode = &PB_decode_callback;
  received.variable.arg = (void *) &output_field;
  
  bool status = DeserializeProtobuf(&received, NestedMessage_fields, buffer, sizeof(buffer));
  if (status) {
    CHECK(received.has_timestamp);
    CHECK_EQUAL(received.timestamp, 2016);
    CHECK_EQUAL(output_field.length, 3);
    
    for (int i = 0; i < 3; ++i) {
      NestedMessage_Variable * raw_var = (NestedMessage_Variable *) output_field.messages[i];
      CHECK_EQUAL(raw_var->has_val, raw_variables[i].has_val);
      CHECK_EQUAL(raw_var->val, raw_variables[i].val);
      CHECK_EQUAL(raw_var->has_devid, raw_variables[i].has_devid);
      CHECK_EQUAL(raw_var->devid, raw_variables[i].devid);
      CHECK_EQUAL(raw_var->has_varid, raw_variables[i].has_varid);
      CHECK_EQUAL(raw_var->varid, raw_variables[i].varid);
    }
  } else {
    FAIL("Failed during decoding!\n");
  }
}
