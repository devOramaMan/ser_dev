MOTOR_BIT = (0, 1, 2)

FRAME_ID = {'SET_REG': 0x01,
            'GET_REG': 0x02,
            'EXECUTE_CMD': 0x03,
            'STORE_TOADDR': 0x04,
            'LOAD_FROMADDR': 0x05,
            'GET_BOARD_INFO': 0x06,
            'SET_RAMP': 0x07,
            'GET_REVUP_DATA': 0x08,
            'SET_REVUP_DATA': 0x09,
            'SET_CURRENT_REF': 0x0A,
            'GET_MP_INFO': 0x0B,
            'GET_FW_VERSION': 0x0C,
            'SET_TORQUE_RAMP': 0x0D,
            'SET_POSITION_CMD': 0x12,
            'KE_GET_FW_CONFIG': 0x13,
            'KE_GET_HW_CONFIG': 0x14,
            'KE_GET_LOG_VAR_LIST': 0x15,
            'KE_SET_LOG_VARIABLES': 0x16,
            'KE_SET_LOG_CONFIG': 0x17,
            'KE_DUMP_LOG': 0x18,
            'KE_GET_LOG_STATUS': 0x19
            }

CMD_ID = {'START_MOTOR': 1,
          'STOP_MOTOR': 2,
          'STOP_RAMP': 3,
          'RESET': 4,
          'PING': 5,
          'START_STOP': 6,
          'FAULT_ACK': 7,
          'ENCODER_ALIGN': 8,
          'IQDREF_CLEAR': 9,
          'PFC_ENABLE': 10,
          'PFC_DISABLE': 11,
          'PFC_FAULT_ACK': 12,
          'SC_START': 13,
          'SC_STOP': 14}

# '''
# Structure: {regName: (address, signed flag, bits, read/write flag)}
# Example for normal name-address access: REG_ID['TARGET_MOTOR'][0]
# '''
REG_ID = {
'TARGET_MOTOR': 0,
'FLAGS': 1,
'STATUS': 2,
'CONTROL_MODE': 3,
'SPEED_REF': 4,
'SPEED_KP': 5,
'SPEED_KI': 6,
'SPEED_KD': 7,
'TORQUE_REF': 8,
'TORQUE_KP': 9,
'TORQUE_KI': 10,
'TORQUE_KD': 11,
'FLUX_REF': 12,
'FLUX_KP': 13,
'FLUX_KI': 14,
'FLUX_KD': 15,
'OBSERVER_C1': 16,
'OBSERVER_C2': 17,
'OBSERVER_CR_C1': 18,
'OBSERVER_CR_C2': 19,
'PLL_KI': 20,
'PLL_KP': 21,
'FLUXWK_KP': 22,
'FLUXWK_KI': 23,
'FLUXWK_BUS': 24,
'BUS_VOLTAGE': 25,
'HEATS_TEMP': 26,
'MOTOR_POWER': 27,
'DAC_OUT1': 28,
'DAC_OUT2': 29,
'SPEED_MEAS': 30,
'TORQUE_MEAS': 31,
'FLUX_MEAS': 32,
'FLUXWK_BUS_MEAS': 33,
'RUC_STAGE_NBR': 34,
'I_A': 35,
'I_B': 36,
'I_ALPHA': 37,
'I_BETA': 38,
'I_Q': 39,
'I_D': 40,
'I_Q_REF': 41,
'I_D_REF': 42,
'V_Q': 43,
'V_D': 44,
'V_ALPHA': 45,
'V_BETA': 46,
'MEAS_EL_ANGLE': 47,
'MEAS_ROT_SPEED': 48,
'OBS_EL_ANGLE': 49,
'OBS_ROT_SPEED': 50,
'OBS_I_ALPHA': 51,
'OBS_I_BETA': 52,
'OBS_BEMF_ALPHA': 53,
'OBS_BEMF_BETA': 54,
'OBS_CR_EL_ANGLE': 55,
'OBS_CR_ROT_SPEED': 56,
'OBS_CR_I_ALPHA': 57,
'OBS_CR_I_BETA': 58,
'OBS_CR_BEMF_ALPHA': 59,
'OBS_CR_BEMF_BETA': 60,
'DAC_USER1': 61,
'DAC_USER2': 62,
'MAX_APP_SPEED': 63,
'MIN_APP_SPEED': 64,
'IQ_SPEEDMODE': 65,
'EST_BEMF_LEVEL': 66,
'OBS_BEMF_LEVEL': 67,
'EST_CR_BEMF_LEVEL': 68,
'OBS_CR_BEMF_LEVEL': 69,
'FF_1Q': 70,
'FF_1D': 71,
'FF_2': 72,
'FF_VQ': 73,
'FF_VD': 74,
'FF_VQ_PIOUT': 75,
'FF_VD_PIOUT': 76,
'PFC_STATUS': 77,
'PFC_FAULTS': 78,
'PFC_DCBUS_REF': 79,
'PFC_DCBUS_MEAS': 80,
'PFC_ACBUS_FREQ': 81,
'PFC_ACBUS_RMS': 82,
'PFC_I_KP': 83,
'PFC_I_KI': 84,
'PFC_I_KD': 85,
'PFC_V_KP': 86,
'PFC_V_KI': 87,
'PFC_V_KD': 88,
'PFC_STARTUP_DURATION': 89,
'PFC_ENABLED': 90,
'RAMP_FINAL_SPEED': 91,
'RAMP_DURATION': 92,
'HFI_EL_ANGLE': 93,
'HFI_ROT_SPEED': 94,
'HFI_CURRENT': 95,
'HFI_INIT_ANG_PLL': 96,
'HFI_INIT_ANG_SAT_DIFF': 97,
'HFI_PI_PLL_KP': 98,
'HFI_PI_PLL_KI': 99,
'HFI_PI_TRACK_KP': 100,
'HFI_PI_TRACK_KI': 101,
'SC_CHECK': 102,
'SC_STATE': 103,
'SC_RS': 104,
'SC_LS': 105,
'SC_KE': 106,
'SC_VBUS': 107,
'SC_MEAS_NOMINALSPEED': 108,
'SC_STEPS': 109,
'SPEED_KP_DIV': 110,
'SPEED_KI_DIV': 111,
'UID': 112,
'HWTYPE': 113,
'CTRBDID': 114,
'PWBDID': 115,
'SC_PP': 116,
'SC_CURRENT': 117,
'SC_SPDBANDWIDTH': 118,
'SC_LDLQRATIO': 119,
'SC_NOMINAL_SPEED': 120,
'SC_CURRBANDWIDTH': 121,
'SC_J': 122,
'SC_F': 123,
'SC_MAX_CURRENT': 124,
'SC_STARTUP_SPEED': 125,
'SC_STARTUP_ACC': 126,
'SC_PWM_FREQUENCY': 127,
'SC_FOC_REP_RATE': 128,
'PWBDID2': 129,
'SC_COMPLETED': 130,
'CURRENT_POSITION': 131,
'TARGET_POSITION': 132,
'MOVE_DURATION': 133,
'POSITION_KP': 134,
'POSITION_KI': 135,
'POSITION_KD': 136,
'I_C': 137,
'I_A2': 138,
'I_B2': 139,
'I_C2': 140,
'I_ALPHA2': 141,
'I_BETA2': 142,
'I_Q2': 143,
'I_D2': 144,
'I_Q_REF2': 145,
'I_D_REF2': 146,
'V_Q2': 147,
'V_D2': 148,
'V_ALPHA2': 149,
'V_BETA2': 150
 }

'''
Structure: {Error code: (Error designator, Description)}
'''
ERROR_CODE = {0x00: ('NONE', 'No error'),
 0x01: ('BAD_FRAME_ID', 'BAD Frame ID. The Frame ID has not been recognized by the firmware.'),
 0x02: ('CODE_SET_READ_ONLY', 'Write on read'),
 0x03: ('CODE_GET_WRITE_ONLY', 'Read not allowed. The value cannot be read. '),
 0x04: ('CODE_NO_TARGET_DRIVE', 'Bad target drive. The target motor is not supported by the firmware.'),
 0x05: ('CODE_WRONG_SET', 'Value used in the frame is out of range expected by the FW.'),
 0x06: ('CODE_CMD_ID', 'NOT USED'),
 0x07: ('CODE_WRONG_CMD', 'Bad command ID. The command ID has not been recognized.'),
 0x08: ('CODE_OVERRUN', 'Overrun error. Transmission speed too fast, frame not received correctly'),
 0x09: ('CODE_TIMEOUT', 'Timeout error. Timeout waiting for response'),
 0x0a: ('CODE_BAD_CRC', 'The computed CRC is not equal to the received CRC byte.'),
 0x0b: ('BAD_MOTOR_SELECTED', 'Bad target drive. The target motor is not supported by the firmware.'),
 0x0c: ('ERROR_MP_NOT_ENABLED', 'Motor Profiler not enabled.'),
 0x0d: ('ERROR_CONFIG_DATA_NOT_AVAILABLE', 'Configuration data entry not available'),
 0x0e: ('ERROR_ARGUMENTS', 'Wrong number or value of arguments'),
 0x0f: ('ERROR_RAM_LOG_ID', 'Wrong log ID'),
 0x10: ('ERROR_RAM_LOG_RECORD', 'Wrong record sequence'),
 0x11: ('ERROR_RAM_LOG_BOUNDS', 'Log dump out of bounds'),
 0x80: ('RES_BAD_CRC','Low level driver - Bad CRC in the response from target'),
 0x81: ('RES_TIMEOUT','Low level driver - Wait for response timeout'), 
 0x82: ('RES_UNPACK', 'Failed to unpack the message'),
 99: ('UNKNOWN', 'Error source unknown')}

class ErrorCode():
  BAD_FRAME_ID				= 0x01
  CODE_SET_READ_ONLY			= 0x02
  CODE_GET_WRITE_ONLY			= 0x03
  CODE_NO_TARGET_DRIVE			= 0x04
  CODE_WRONG_SET			= 0x05
  CODE_CMD_ID				= 0x06
  CODE_WRONG_CMD			= 0x07
  CODE_OVERRUN				= 0x08
  CODE_TIMEOUT				= 0x09
  CODE_BAD_CRC				= 0x0a
  BAD_MOTOR_SELECTED			= 0x0b
  ERROR_MP_NOT_ENABLED			= 0x0c
  ERROR_CONFIG_DATA_NOT_AVAILABLE	= 0x0d
  ERROR_ARGUMENTS			= 0x0e
  RES_BAD_CRC = 0x80
  RES_TIMEOUT = 0x81
  RES_UNPACK = 0x82
  UNKNOWN				=  99

''' States of motor control firmware '''
STATUS = {
    12: 'ICLWAIT',
    0: 'IDLE',
    1: 'IDLE_ALIGNMENT',
    13: 'ALIGN_CHARGE_BOOT_CAP',
    14: 'ALIGN_OFFSET_CALIB',
    15: 'ALIGN_CLEAR',
    2: 'ALIGNMENT',
    3: 'IDLE_START',
    16: 'CHARGE_BOOT_CAP',
    17: 'OFFSET_CALIB',
    18: 'CLEAR',
    4: 'START',
    19: 'SWITCH_OVER',
    5: 'START_RUN',
    6: 'RUN',
    7: 'ANY_STOP',
    8: 'STOP',
    9: 'STOP_IDLE',
    10: 'FAULT_NOW',
    11: 'FAULT_OVER',
    20: 'WAIT_STOP_MOTOR'
}

RAM_LOG_TYPE = {
  'SINGLE_SHOT':0, 
  'RING_BUFFER':1, 
  'RUN':2
  }

# define from frame_communication_protocol.h
# TODO readout from FW
FCP_MAX_PAYLOAD_SIZE = 128

FW_PARAM = {
   0:'AVAILABLE' #OBSOLETE
  ,1:'FW_PRODUCT_NAME' 
  ,2:'FW_BRANCH_NAME' 
  ,3:'PWM_FREQUENCY' 
  ,4:'LOG_BUFFER_SIZE' 
  ,5:'NU_OF_MOTORS' 
  ,6:'FW_SW_VERSION' 
  ,7:'COMPILE_TIME'
  ,8:'RUN_TIME' 
  ,9:'BOOT_COUNTER'
  ,10:'SELF_TEST'
}

FW_CONFIG = {
  1: 'FW_NAME',
  2: 'FW_BRANCH',
  3: 'PWM_FREQUENCY',
  4: 'LOG_BUFFER_SIZE',
  5: 'NU_OF_MOTORS',
  6: 'FW_VERSION',
  7: 'COMPILE_TIME'
}

HW_CONFIG = {
  1: 'HW_NAME',
	2: 'HW_VERSION',
	3: 'MCU_UID',
}

#STM Protocol idx
ACK_IDX = 0
PAYLOADLEN_IDX = 1
ERRCODE_IDX = 2
PAYLOAD_IDX = 2
CRC_OFFSET = 2

ENABLE_LOGGING = 0xFF

WORKER_RANGE=1000
WORKER_GET_FILE=0
WORKER_SET_FILE=1000
WORKER_RECONNECT = 10001

SER_STM_INTERNAL_ADR=4000
SER_STM_CMD_ADR=4000
SER_STM_LOGVAR_ADR=5000
SER_STM_LOGVAR_FILE=0
SER_STM_LOGVAR_DECI=2
SER_STM_LOGVAR_TYPE=3
SER_STM_LOGVAR_SET=4 #4 - AND Beyond - +1000
SER_STM_LOGVAR_STATUS=5
SER_STM_LOGVAR_ENA_SET=6

SER_STM_HW_PARAM_ADR=6000
SER_STM_FW_PARAM_ADR=6200
SER_STM_HW_CONFIG=0
SER_STM_FW_PARAM=20

MAX_LOG_VARS = 8





# refer to CRC calculation formula in STM32 PMSM MC Library User Manual
def _calcCrc(frame):
  total = sum(frame)
  highbyte = (total & 0xff00) >> 8
  lowbyte = total & 0x00ff
  crc = (lowbyte + highbyte) & 0xff  # crop to 8 bit
  return crc

import string
string.printable = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ()#!+-.<=>*\n\r\t~ \x0b\0c'
printable_chars = bytes(string.printable, 'ascii')

def isascii(data):
  return all(char in printable_chars for char in data)

def isascii_in_buffer(data):
  end = 0
  for char in data:
    if( char > 8 and char < 128):
      end += 1
    else:
      break;

  return data[:end], data[end:]

