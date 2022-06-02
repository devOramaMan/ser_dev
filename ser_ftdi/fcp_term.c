
#include "fcp_term.h"
#include "fcp_frame_protocol.h"
#include "util_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


typedef enum
{
    FCP_REQ_REGISTER,
    FCP_REQ_FAMEID,
    FCP_REQ_MOTOR,
    FCP_REQ_DATATYPE,
    FCP_REQ_VALUE,
    FCP_REQ_SIZE
} FCP_REQ_STEP;


void make_fcp_req(void);
void poll_register(void);
void read_file(void);
void make_sequence(void);

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
  char char_choice[50];
  int32_t choice;
  int32_t input[FCP_REQ_SIZE];
  uint8_t data[FCP_MAX_PAYLOAD_SIZE];
  uint32_t i;
  uint32_t step = 0;
  uint32_t err = 0;
  bool isWrite = true;
  char q = 'r';

  static const char FcpReqTypeStr[FCP_REQ_SIZE][23] = {"Register:            \0", "FrameId:             \0", "Motor Select [0/1/2]:\0", "DataType:            \0", "Data:                \0"};

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
      if(i == FCP_REQ_VALUE)
      {
        
      }
      else
      {
        printf("%s %d\n", FcpReqTypeStr[i], input[i]);
      }
    }
    if (step < FCP_REQ_SIZE)
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
          if (isWrite)
            fcp_write_register_async(input[FCP_REQ_REGISTER], input[FCP_REQ_FAMEID], input[FCP_REQ_MOTOR], input[FCP_REQ_VALUE], 1, (Data_Type)input[FCP_REQ_DATATYPE]);
          else
            fcp_read_register_async(input[FCP_REQ_REGISTER], input[FCP_REQ_FAMEID], input[FCP_REQ_MOTOR], (Data_Type)input[FCP_REQ_DATATYPE]);
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
      printf("Type input (exit:q)) > ");
      scanf("%s", char_choice);

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
        if (step == FCP_REQ_VALUE)
        {
          step++;
        }
        else
        {
          q = char_choice[0];
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

