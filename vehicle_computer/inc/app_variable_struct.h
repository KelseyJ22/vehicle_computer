// Author: Gawan Fiore (gfiore@cs.stanford.edu)
// Copyright (C) Stanford Solar Car Project 2016
// TODO(gawan): file description


#ifndef VEHICLE_COMPUTER_APP_VARIABLE_STRUCT_H_
#define VEHICLE_COMPUTER_APP_VARIABLE_STRUCT_H_

void InitStruct() {
  SimpleMessage message = SimpleMessage_init_zero;

  /* Create a stream that will write to our buffer. */
  pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

  /* Fill in the lucky number */
  message.lucky_number = 13;
}


#endif // VEHICLE_COMPUTER_APP_VARIABLE_STRUCT_H_
