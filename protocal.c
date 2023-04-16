#include <stdint.h>
#include <stdio.h>
#include <string.h>

const int MAX_BROADCAST_LENGTH = 100;
const int MIN_LENGTH = 12;

const int BROADCAST_DATA_LENGTH = 4;

const int PREAMBLE_LENGTH = 2;
const int INTREPID_PREAMBLE_BE = 0xFEEF;
const int INTREPID_PREAMBLE_LE = 0xEFFE;

const int MIN_DATA_LENGTH = 4;
const int LENGTH_LENGTH = 4;

const int MSG_TYPE_LENGTH = 2;

const int MSG_TYPE_OFFSET = PREAMBLE_LENGTH;
const int LENGTH_OFFSET = MSG_TYPE_OFFSET + MSG_TYPE_LENGTH;
const int DATA_OFFSET = LENGTH_OFFSET + LENGTH_LENGTH;

// typedef struct IntrepidMsg {
//   u_int16_t header;
//   u_int16_t msgType;
//   u_int32_t length;
//   u_int8_t *data;

// } IntrepidMsg;

typedef enum IntrepidMsgType {
  Broadcast = 0,
  Data = 1,
} IntrepidMsgType;

void printBuf(char *b, int length) {
  for (int i = 0; i < length; i++) {
    printf(" %hhx ", b[i]);
  }
  printf("\n");
}

// encodes name into an Intrepid broadcast message,
// return value is an error value
// function writes to the buf field
int encodeBroadcast(int name, char *buf, int buf_size) {

  if (buf_size < MIN_LENGTH || buf_size > MAX_BROADCAST_LENGTH) {
    return -1;
  }
  // clean buffer
  memset(buf, 0, buf_size);

  // IntrepidMsgType t = Broadcast;
  uint16_t t = 0;

  memcpy(buf, &(INTREPID_PREAMBLE_LE), PREAMBLE_LENGTH);
  memcpy(buf + MSG_TYPE_OFFSET, &(t), MSG_TYPE_LENGTH);
  memcpy(buf + LENGTH_OFFSET, &(BROADCAST_DATA_LENGTH), LENGTH_LENGTH);
  memcpy(buf + DATA_OFFSET, &name, BROADCAST_DATA_LENGTH);

  return 0;
}

// int main() {

//   int s = 12;
//   char buf[s];
//   int name = 32;

//   encodeBroadcast(name, buf, s);
//   printBuf(buf, s);

//   char c[100];
//   memset(c, 0, 100);
//   memcpy(c, buf, s);
//   printBuf(c,100);
// }
