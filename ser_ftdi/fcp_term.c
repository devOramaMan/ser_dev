
#include "fcp_term.h"
#include "fcp_frame_protocol.h"
#include "util_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include "protocol_common.h"


typedef enum
{
    FCP_REQ_REGISTER,
    FCP_REQ_FAMEID,
    FCP_REQ_MOTOR,
    FCP_REQ_VALUE,
    FCP_REQ_SIZE
} FCP_REQ_STEP;

typedef enum
{
    FCP_DATA_TYPE,
    FCP_DATA_VALUE,
    FCP_DATA_SIZE
} FCP_DATA_STEP;


void make_fcp_req(void);
void poll_register(void);
void read_file(void);
void make_sequence(void);

uint32_t add_values(uint8_t * pData)
{
  int32_t choice;
  uint32_t data_size = 0;
  float fval;
  bool isTrue;
  Data_Type datatype;
  char *ptr;
  char q;
  uint32_t step = 0, i;
  int32_t err = 0;
  char input[FCP_DATA_SIZE][23] = {"-\0","-\0"};

  static const char FcpDataReqTypeStr[FCP_DATA_SIZE][23] = {"DataType:            \0", "Data:                \0"};

  do
  {
    CLEAR_SCREEN();
    if (err == 1)
    {
      printf("\nIllegal input... Try Again\n\n");
    }
    else if (err == 2)
    {
      printf("\nIllegal input... Must be a digit\n\n");
    }
    else if(err == 3)
    {
      printf("\nValue not matching datatype\n\n");
    }
    err = 0;

    if(step == FCP_DATA_TYPE)
    {
      for(i=0;i<UNSIGNED32;i++) 
        printf("%s\n", dataTypeStr[i]);
    }

    for (i = 0; i < FCP_DATA_SIZE; i++)
    {
      printf("%s %s\n", FcpDataReqTypeStr[i], input[i]);
    }

    printf("Type input (exit:q)) > ");
    scanf("%s", input[step]);
    q = input[step][0];
    if(q != 'q')
    {
      switch (step)
      {
      case FCP_DATA_TYPE:
          if (isNumber(input[step], sizeof(input[step]) - 1))
          {
            if (sscanf(input[step], "%d", &choice) != EOF)
            {
              datatype = (Data_Type)choice;
              step++;
            }
            else
              err = 1;
          }
          else
          {
            err = 2;
          }
        break;
      case FCP_DATA_VALUE:
        ptr = strtok(input[step], ",");
        while(ptr != NULL)
        {
          printf("'%s'\n", ptr);
          if(datatype == REAL32)
          {
            fval = isFloat(ptr, &isTrue);
            if( isTrue )
            {
              data_size += setData(REAL32, pData,(uint8_t*)&fval);
              q ='q';
            }
            else
              err = 3;
          }
          else
          {
            if (isNumber(input[step], sizeof(input[step]) - 1))
            {
              if (sscanf(input[step], "%d", &choice) != EOF)
              {
                data_size += setData(datatype, pData,(uint8_t*)&choice);
                q ='q';
              }
              else
                err = 1;
            }
            else
              err = 2;
          }
          ptr = strtok(NULL, ",");
        }
        break;
        default:
          break;
      }
    }
      
    
  } while (q != 'q' );
  
  
  return data_size;
}

void fcp_menu(void)
{
    char char_choice[50];
    int int_choice = 0;

  do
	{
		printf("FCP Menu: \n");
		printf("1. Send FCP Frame\n");
    printf("2. Read File\n");
    printf("3. Make Read Write Sequence\n");

		printf("9. Exit\n");


		// Get user choice.
		scanf("%s", char_choice);

		// Convert string to int for switch statement.
		int_choice = atoi(char_choice);

		CLEAR_SCREEN();
		switch (int_choice)
		{
		case 1:
            make_fcp_req();
			break;
        case 2:
            printf("Not Implemented \n");
            break;
        case 3:
            printf("Not Implemented \n");
            break;
        case 9:
			int_choice = 99;
			break;

		default:
			printf(""
				   "Unknown command"
				   "");
			break;
		}
	} while (int_choice != 99);
}

void make_fcp_req(void)
{
  char char_choice[200];
  int32_t choice;
  int32_t input[FCP_REQ_SIZE] = {0,0,0,0};
  uint8_t data[FCP_MAX_PAYLOAD_SIZE];
  uint32_t data_size = 0;
  uint32_t i;
  uint32_t step = 0;
  uint32_t err = 0;
  char q = 'r';

  static const char FcpReqTypeStr[FCP_REQ_SIZE][23] = {"Register:            \0", "FrameId:             \0", "Motor Select [0/1/2]:\0", "Data:                \0"};

  do
  {
    if (err == 1)
    {
      printf("\nIllegal input... Try Again\n\n");
      err = 0;
    }
    if (err == 2)
    {
      printf("\nIllegal input... Must be a digit\n\n");
      err = 0;
    }

    if (step == FCP_REQ_MOTOR)
      printf("FYI motor selection:\n0: Same motor as last\n1: Motor 1\n2: Motor 2\n");
    
    for (i = 0; i < step; i++)
    {
      if (i != FCP_REQ_VALUE)
        printf("%s %d\n", FcpReqTypeStr[i], input[i]);
    }
    if(step >= FCP_REQ_VALUE)
    {
      if(data_size)
      {
        printf("%s ", FcpReqTypeStr[FCP_REQ_VALUE]);
        for(i=0; i < data_size; i++ )
          printf("0x%02X ", data[i]);
        if(step == FCP_REQ_VALUE)
          printf("_\n");
        else
          printf("\n");
      }
    }
    if (step < FCP_REQ_VALUE)
    {
      printf("%s _\n", FcpReqTypeStr[step]);
    } 

    for (i = step + 1; i < FCP_REQ_SIZE; i++)
    {
      printf("%s\n", FcpReqTypeStr[i]);
    }

    if (step >= FCP_REQ_SIZE)
    {
      do
      {
        printf("Send FCP (y/[n/q])? > ");
        scanf("%s", char_choice);
        choice = (int32_t)char_choice[0];
        switch (choice)
        {
        case 'y':
        case 'Y':
          fcp_send_async(input[FCP_REQ_REGISTER], input[FCP_REQ_FAMEID], input[FCP_REQ_MOTOR], data, data_size);
          break;
        case 'n':
        case 'N':
        case 'q':
          printf("cancel");
          q = 'q';
          choice = 'q';
          sleep(1);
        default:
          err = 1;
        }
        /* code */
      } while (choice != 'q');
    }
    else
    {
      if (step == FCP_REQ_VALUE)
      {    
        printf("Add data? (exit:q [y/n]) > ");
        scanf("%s", char_choice);
        q = char_choice[0];
        switch(q)
        {
          case 'n':
            step++;
            break;
          case 'y':
            data_size += add_values(&data[data_size]);
            break;
          default:
            err = 1;
        }
      }
      else
      {
        printf("Type input (exit:q)) > ");
        scanf("%s", char_choice);
        q = char_choice[0];
      
        if (isNumber(char_choice, sizeof(char_choice) - 1))
        {
          if (sscanf(char_choice, "%d", &choice) != EOF)
          {
            input[step] = choice;
            step++;
          }
          else
            err = 1;
        }
        else
        {
          err = 2;
        }
      }
      
        
    }
    CLEAR_SCREEN();
    

  } while (q != 'q');
}



void make_sequence(void)
{

}

