#include "globals.h"
#include <IRrecv.h>
#include <IRutils.h>
decode_results results;

IRrecv ir(IR_PIN);

//DENN Remote
#define POWER_CODE 0x33B800FF
#define CODE_MUTE 0x33B8807F
#define CODE_1 0x33B88877
#define CODE_2 0x33B848B7
#define CODE_3 0x33B8C837
#define CODE_4 0x33B828D7
#define CODE_5 0x33B8A857
#define CODE_6 0x33B86897
#define CODE_7 0x33B8E817
#define CODE_8 0x33B818E7
#define CODE_9 0x33B89867
#define CODE_0 0x33B808F7
#define CODE_GOTO 0x33B8D827
#define CODE_RECALL 0x33B858A7
#define CODE_INFO 0x33B8906F
#define CODE_SUBTITLE 0x33B850AF
#define CODE_OK 0x33B820DF
#define CODE_LEFT 0x33B8E01F
#define CODE_RIGHT 0x33B810EF
#define CODE_UP 0x33B8A05F
#define CODE_DOWN 0x33B8609F
#define CODE_EXIT 0x33B8C03F
#define CODE_MENU 0x33B840BF


void beginIR() {
  ir.enableIRIn();
}

void handleIR() {
  if (!ir.decode(&results)) return;
  
  //Serial.print("0x");
  //Serial.println(results.value, HEX);

  switch (results.value) {
  case POWER_CODE:
    newPowerState = !newPowerState;
    break;
  case CODE_LEFT:
    if (newBright == 0) return;
    else newBright-=2;
    break;
  case CODE_RIGHT:
    if (newBright >= 255) return;
    else newBright+=2;
    break;
  case CODE_UP:
    if (data.mode >= TOTAL_MODES) return;
    else data.mode++;
    break;
  case CODE_DOWN:
    if (data.mode == 0) return;
    else data.mode--;
    break;
  case CODE_RECALL:
    lowpass_trigger = true;
    break;
  case CODE_GOTO:
    data.portAux = !data.portAux;
    break;
  case CODE_EXIT:
    ESP.restart();
    break;
  default:
    ir.resume();
    return;
    break;
  }

  ir.resume();
  settings.update();
}
