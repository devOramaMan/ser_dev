
#include "fcp_term.h"
#include "fcp_frame_protocol.h"
#include "util_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


typedef enum
{
    FCP_WRITE_REGISTER,
    FCP_WRITE_FAMEID,
    FCP_WRITE_MOTOR,
    FCP_WRITE_VALUE,
    FCP_WRITE_DATATYPE,
    FCP_WRITE_SIZE
} FCP_WRITE_STEP;

typedef enum
{
    FCP_READ_REGISTER,
    FCP_READ_FAMEID,
    FCP_READ_MOTOR,
    FCP_READ_DATATYPE,
    FCP_READ_SIZE
} FCP_READ_STEP;

void write_register(void);
void read_register(void);
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
		printf("1. Write Register\n");
		printf("2. Read Register\n");
        printf("3. Poll Register\n");
        printf("4. Read File\n");
        printf("5. Make Read Write Sequence\n");

		printf("9. Exit\n");


		// Get user choice.
		scanf("%s", char_choice);

		// Convert string to int for switch statement.
		int_choice = atoi(char_choice);

		CLEAR_SCREEN();
		switch (int_choice)
		{
		case 1:
            write_register();
			break;
        case 2:
            read_register();
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

void write_register(void)
{
    char char_choice[50];
	int32_t choice;
    int32_t input[FCP_WRITE_SIZE];
    uint32_t i;
    uint32_t step = 0;
    uint32_t err = 0;
    char q = 'r';
    
    

    static const char WriteRegTypeStr[FCP_WRITE_SIZE][23] = {"Register:            \0", "FrameId:            \0", "Motor Select [0/1/2]:\0", "DataType:           \0", "Value:               \0"};

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

        if (step == FCP_WRITE_MOTOR)
            printf("FYI motor selection:\n0: Same motor as last\n1: Motor 1\n2: Motor 2\n");
        for(i = 0; i < step; i++ )
        {
            printf("%s %d\n", WriteRegTypeStr[i], input[i]);
        }
        if ( step < FCP_WRITE_SIZE )
        {
            printf("%s _\n", WriteRegTypeStr[step]);
        }
        
        for(i = step + 1; i < FCP_WRITE_SIZE; i++ )
        {
            printf("%s\n", WriteRegTypeStr[i]);
        }

        

        if (step >= FCP_WRITE_SIZE)
        {
            printf("Write Register (y/[n/q])? > ");
            scanf("%s", char_choice);
            choice = (int32_t) char_choice[0];
            switch(choice) 
            {
                case 'y':
                case 'Y':
                    printf("write...");
                    fcp_write_register_async(input[FCP_WRITE_REGISTER], input[FCP_WRITE_FAMEID], input[FCP_WRITE_MOTOR], input[FCP_WRITE_VALUE], 1 , (Data_Type)input[FCP_READ_DATATYPE] );
                    break;
                case 'n':
                case 'N':
                case 'q':
                    printf("cancel");
                    q = 'q';
                default:
                    err = 1;
            }
        }
        else
        {
            printf("Type input (exit:q)) > ");
            scanf("%s", char_choice);
            
            if(isNumber(char_choice, sizeof(char_choice)-1) )
            {
                if(sscanf(char_choice, "%d", &choice) != EOF)
                {
                    input[step] = choice;
                    step++;
                }
                else
                    err = 1;
            }
            else
            {
                q = char_choice[0];
                err = 2;
            }
        }
        CLEAR_SCREEN();
        

    } while (q != 'q');
}

void read_register(void)
{
    char char_choice[50];
	int32_t choice;
    int32_t input[FCP_READ_SIZE];
    uint32_t i;
    uint32_t step = 0;
    uint32_t err = 0;
    char q = 'r';    
    

    static const char ReadRegTypeStr[FCP_READ_SIZE][23] = {"Register:            \0", "FrameId:             \0", "Motor Select [0/1/2]:\0", "DataType:            \0"};

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

        if (step == FCP_READ_MOTOR)
            printf("FYI motor selection:\n0: Same motor as last\n1: Motor 1\n2: Motor 2\n");
        for(i = 0; i < step; i++ )
        {
            printf("%s %d\n", ReadRegTypeStr[i], input[i]);
        }
        if ( step < FCP_READ_SIZE )
        {
            printf("%s _\n", ReadRegTypeStr[step]);
        }
        
        for(i = step + 1; i < FCP_READ_SIZE; i++ )
        {
            printf("%s\n", ReadRegTypeStr[i]);
        }

        

        if (step >= FCP_READ_SIZE)
        {
            do
	        {
                printf("Read Register (y/[n/q])? \n > ");
                scanf("%s", char_choice);
                choice = (int32_t) char_choice[0];
                switch(choice) 
                {
                    case 'y':
                    case 'Y':
                        if(!fcp_read_register_async(input[FCP_READ_REGISTER], input[FCP_READ_FAMEID], input[FCP_READ_MOTOR], (Data_Type)input[FCP_READ_DATATYPE] ))
                            err = 1;
                        //printf("read...");
                        break;
                    case 'n':
                    case 'N':
                    case 'q':
                        //printf("cancel");
                        q = 'q';
                    default:
                        err = 1;
                }
            }while (q != 'q');
        }
        else
        {
            printf("Type input (exit:q)) > ");
            scanf("%s", char_choice);
            
            if(isNumber(char_choice, sizeof(char_choice)-1) )
            {
                if(sscanf(char_choice, "%d", &choice) != EOF)
                {
                    input[step] = choice;
                    step++;
                }
                else
                    err = 1;
            }
            else
            {
                q = char_choice[0];
                err = 2;
            }
            CLEAR_SCREEN();
        }
        
        

    } while (q != 'q');
}


void make_sequence(void)
{

}

