//////////////////
//Author: M.Stohn/
//////////////////

#include <stdlib.h>
#include <curses.h>
#include <signal.h>
#include <string.h>

#include <stdbool.h>
#include <stdint.h>

#include "up3d.h"

#define printw_b(...) {attron(A_BOLD);printw(__VA_ARGS__),attroff(A_BOLD);}

static void update_state(bool redrawall)
{
  int rows, cols;
  getmaxyx(stdscr,rows,cols);
  
  if( redrawall )
  {
    clear();

    attrset(COLOR_PAIR(1));
    attron(A_UNDERLINE);
    mvhline(0,0,' ',cols);
    attron(A_BOLD);
    mvprintw(0,(cols-11)/2,"UP!3D SHELL");
    attroff(A_UNDERLINE|A_BOLD);

    attrset(COLOR_PAIR(8));
    mvhline(rows-1,0,' ',cols);
    mvprintw(rows-1,(cols-21)/2,"Press ^C to quit.");
  }

  attrset(COLOR_PAIR(1));

  move(2,0);
  printw_b("Machine-State:");
  int32_t mstat = UP3D_GetMachineState();
  printw(" (%1d) %-16.16s", mstat, UP3D_STR_MACHINE_STATE[mstat] );

  move(2,(cols-40)/2);
  printw_b("Program-State:");
  int32_t pstat = UP3D_GetProgramState();
  printw(" (%1d) %-19.19s", pstat, UP3D_STR_PROGRAM_STATE[pstat] );

  move(2,cols-42);
  printw_b("System-State:");
  int32_t sstat = UP3D_GetSystemState();
  printw(" (%02d) %-22.22s", sstat, UP3D_STR_SYSTEM_STATE[sstat] );

  attron(A_UNDERLINE);mvhline(3,0,' ',cols);attroff(A_UNDERLINE);

  int m;
  for( m=1; m<5; m++ )
  {
    move(5+m-1,0);
    printw_b("Motor %d: State:",m );
    int32_t motstat = UP3D_GetMotorState(m);
    printw(" (%1d) %-9.9s", motstat, UP3D_STR_MOTOR_STATE[motstat] );
    printw(" | ");
    printw_b("Limit+:");
    printw(" (%08x)", UP3D_GetPositiveLimitState(m) );
    printw(" | ");
    printw_b("Limit-:");
    printw(" (%08x)", UP3D_GetNegativeLimitState(m) );
    printw(" | ");
    
    printw_b("Axis %s State:",UP3D_STR_AXIS_NAME[m-1]);
    int32_t astat = UP3D_GetAxisState(m);
    printw(" (%1d) %-12.12s", astat, UP3D_STR_AXIS_STATE[astat] );

    printw_b("Pos:");
    int32_t apos = UP3D_GetAxisPosition(m);
    printw(" %-12d (%-12.3f)", apos, (float)apos / 854.0 );
  }

  attron(A_UNDERLINE);mvhline(9,0,' ',cols);attroff(A_UNDERLINE);

  move(11,0);
  printw_b("Print-State:");
  printw(" %-12s ", UP3D_GetPrintState()?"Printing":"Not Printing" );

  printw_b("Layer:");
  printw(" %-4d ", UP3D_GetLayer() );

  printw_b("Height:");
  printw(" %-12.3f ", UP3D_GetHeight() );

  move(11,(cols-14)/2);
  printw_b("Progress:");
  printw(" %3d%%", UP3D_GetPercent() );

  move(11,cols-24);
  printw_b("Time Remaining:");
  int32_t tr = UP3D_GetTimeRemaining();
  int32_t trh = tr / (60*60); tr -= trh*60*60;
  int32_t trm = tr / 60; tr -= trm*60;
  int32_t trs = tr;
  printw(" %2d:%02d:%02d", trh,trm,trs );
  
  attron(A_UNDERLINE);mvhline(12,0,' ',cols);attroff(A_UNDERLINE);

  int h;
  for( h=1; h<5; h++ )
  {
    move(14+h-1,0);
    printw_b("Heater %d: ActualTemp:",h );
    printw(" %5.1f C ", UP3D_GetHeaterTemp(h) );

    printw_b("TargetTemp:" );
    printw(" %3d.0 C ", UP3D_GetHeaterTargetTemp(h) );

    int32_t sh = UP3D_GetHeaterSetTemp(h);
    if( sh>=0 )
    {
      printw_b("SetTemp:" );
      printw(" %3d.0 C ", sh );
    }

    int32_t rh = UP3D_GetHeaterTempReached(h);
    if( rh>=0 )
    {
      printw_b("Temp Reached:" );
      printw(" %-3s ", rh?"YES":"NO" );
    }
    else
      printw("                  ");

    int32_t ph = UP3D_GetHeaterState(h);
    if( ph>=0 )
    {
      printw_b("Heating:" );
      if( ph>1 )
      {
        ph*=2;
        int32_t phh = ph / (60*60); tr -= phh*60*60;
        int32_t phm = ph / 60; ph -= phm*60;
        int32_t phs = ph;

        printw(" %d:%02d:%02d", phh,phm,phs );
      }
      else
        printw(" %-15s ", ph?"On":"Off" );
    }

    move(14+h-1,cols-30);
    printw_b(" *** HF %d:",h );
    printw(" %-11d", UP3D_GetHF(h) );
  }

  attron(A_UNDERLINE);mvhline(18,0,' ',cols);attroff(A_UNDERLINE);

  move(20,0);
  printw_b("Have Support:");
  printw(" %-3s ", UP3D_GetSupportState()?"Yes":"No");
  
  printw_b("Feed Error Length:");
  printw(" %4d ", UP3D_GetFeedErrorLength() );

  printw_b("Feed Back Length:");
  printw(" %7.3f ", UP3D_GetFeedBackLength() );

  printw_b("Change Nozzle Time:");
  printw(" %7.3f ", UP3D_GetChangeNozzleTime() );

  printw_b("Jump Time:");
  printw(" %7.3f ", UP3D_GetJumpTime() );

  printw_b("Using Nozzle:");
  printw(" %2d ", UP3D_GetUsedNozzle() );

  attron(A_UNDERLINE);mvhline(21,0,' ',cols);attroff(A_UNDERLINE);

  move(23,0);
  printw_b("Check Door:");
  printw(" %4d ", UP3D_GetCheckDoor());
  printw_b("Check 24V:");
  printw(" %4d ", UP3D_GetCheck24V());
  printw_b("Check PowerKey:");
  printw(" %4d ", UP3D_GetCheckPowerKey());
  printw_b("Check LightKey:");
  printw(" %4d ", UP3D_GetCheckLightKey());
  printw_b("Check WorkRoomFan:");
  printw(" %4d ", UP3D_GetCheckWorkRoomFan());

  attron(A_UNDERLINE);mvhline(24,0,' ',cols);attroff(A_UNDERLINE);

  move(26,0);
  printw_b("Read From SD Card:");
  printw(" %1d ", UP3D_GetReadFromSD());
  printw_b("Write To SD Card:");
  printw(" %1d ", UP3D_GetWriteToSD());
  printw_b("PausePRG:");
  printw(" %1d ", UP3D_GetParameter(0x0c));
  printw_b("StopPRG:");
  printw(" %1d ", UP3D_GetParameter(0x0d));
  printw_b("InitPRG:");
  printw(" %1d ", UP3D_GetParameter(0x0e));

  printw_b("ISTATE:");
  printw(" %d ", UP3D_GetParameter(0x0f));

  attron(A_UNDERLINE);mvhline(27,0,' ',cols);attroff(A_UNDERLINE);

  int r=29; move(r++,0);  
  int g;
  for(g=0x3d;g<0x100;g++)
  {
    if( 0==g%12 ) 
      move(r++,0);  
    printw_b("%02X:",g);printw("%08x ", UP3D_GetParameter(g));
  }

/*
  move(r++,0);  
  for(g=0x50;g<0x54;g++)
  {
    uint32_t f = UP3D_GetParameter(g);
    printw_b("%02X:",g);printw("%f ", *((float*)&f) );
  }
*/

  move(r++,0);  
  for(g=0x84;g<0x8A;g++)
  {
    uint32_t f = UP3D_GetParameter(g);
    printw_b("%02X:",g);printw("%f ", *((float*)&f) );
  }

  move(r++,0);  
  for(g=0x97;g<0x9F;g++)
  {
    uint32_t f = UP3D_GetParameter(g);
    printw_b("%02X:",g);printw("%f ", *((float*)&f) );
  }

  //move(29,0);for(g=0x3d;g<0x45;g++){ printw_b("$%02X:",g);printw(" %08x ", UP3D_GetParameter(g));}
  //move(30,0);for(g=0x45;g<0x4c;g++){ printw_b("$%02X:",g);printw(" %08x ", UP3D_GetParameter(g));}
  
  refresh();
}

static void sigwinch(int sig)
{
  endwin();
  refresh();
  update_state(true);
}

static void sigfinish(int sig)
{
	UP3D_Close();
  endwin();
  exit(0);
}

int main(int argc, char *argv[])
{
  if( !UP3D_Open() )
    return -1;

  signal(SIGINT, sigfinish);   // set sigint handler
  signal(SIGWINCH, sigwinch);  // set sigint handler

  initscr();                   // initialize the curses library
  raw();                       // line buffering disabled
  keypad(stdscr, TRUE);        // enable keyboard mapping
  nonl();                      // tell curses not to do NL->CR/NL on output
  cbreak();                    // take input chars one at a time, no wait for \n
  noecho();                    // getch no echo
  nodelay(stdscr, TRUE);       // getch nonblocking
  curs_set(0);                 // no visible cursor
  
  if( has_colors() )
  {
    start_color();
    use_default_colors();
    init_pair(1, -1, -1);
    init_pair(2, COLOR_RED,     -1);
    init_pair(3, COLOR_GREEN,   -1);
    init_pair(4, COLOR_YELLOW,  -1);
    init_pair(5, COLOR_BLUE,    -1);
    init_pair(6, COLOR_CYAN,    -1);
    init_pair(7, COLOR_MAGENTA, -1);
    init_pair(8, COLOR_WHITE,   COLOR_BLACK);

    bkgd(COLOR_PAIR(1));
  }

  //initial draw
  update_state(true);

  //the loop
  for(;;)
  {
    int c = getch();

    switch( c )
    {
      case 0x12: update_state(true); break; // CTRL-R

      case 'p':
       {
         UP3D_BLK blk;
         UP3D_BeginWrite();
         UP3D_PROG_BLK_Power(&blk,true);UP3D_WriteBlock(&blk);
         UP3D_PROG_BLK_Stop(&blk);UP3D_WriteBlock(&blk);
         UP3D_Execute();
         UP3D_SetParameter(0x94,999); //set best accuracy for reporting position
       }
       break;
      case 'q':
       {
         UP3D_BLK blk;
         UP3D_BeginWrite();
         UP3D_PROG_BLK_Power(&blk,false);UP3D_WriteBlock(&blk);
         UP3D_PROG_BLK_Stop(&blk);UP3D_WriteBlock(&blk);
         UP3D_Execute();
         sigfinish(0);
       }
       break;

      case '0':
       {
         UP3D_BeginWrite();
         UP3D_InsertRomProgram(0);
         UP3D_Execute();
       }
       break;

      case 'b':
       {
         UP3D_BLK blk;
         UP3D_BeginWrite();
         UP3D_PROG_BLK_Beeper(&blk,true);UP3D_WriteBlock(&blk);
         UP3D_PROG_BLK_Pause(&blk,100);UP3D_WriteBlock(&blk);
         UP3D_PROG_BLK_Beeper(&blk,false);UP3D_WriteBlock(&blk);
         UP3D_PROG_BLK_Pause(&blk,100);UP3D_WriteBlock(&blk);
         UP3D_PROG_BLK_Beeper(&blk,true);UP3D_WriteBlock(&blk);
         UP3D_PROG_BLK_Pause(&blk,100);UP3D_WriteBlock(&blk);
         UP3D_PROG_BLK_Beeper(&blk,false);UP3D_WriteBlock(&blk);
         UP3D_PROG_BLK_Stop(&blk);UP3D_WriteBlock(&blk);
         UP3D_Execute();
       }
       break;

      case 'h':
       {
         UP3D_BLK blk;
         UP3D_BLK blksHome[2];
         UP3D_BeginWrite();
         UP3D_PROG_BLK_Home( blksHome, UP3DAXIS_Z ); UP3D_WriteBlocks(blksHome,2);
         UP3D_PROG_BLK_Home( blksHome, UP3DAXIS_Y ); UP3D_WriteBlocks(blksHome,2);
         UP3D_PROG_BLK_Home( blksHome, UP3DAXIS_X ); UP3D_WriteBlocks(blksHome,2);
         UP3D_PROG_BLK_Stop(&blk);UP3D_WriteBlock(&blk);
         UP3D_Execute();
       }
       break;

      case '1':
       {
         UP3D_BLK blk;
         UP3D_BLK blksMoveF[2];
         UP3D_BeginWrite();
         UP3D_PROG_BLK_MoveF( blksMoveF,-150,-60.0,-150,60.0,0,0,0,0);
         UP3D_WriteBlocks(blksMoveF,2);
         UP3D_PROG_BLK_Stop(&blk);UP3D_WriteBlock(&blk);
         UP3D_Execute();
       }
       break;

      case '2':
       {
         UP3D_BLK blk;
         UP3D_BeginWrite();
         UP3D_PROG_BLK_Stop(&blk);UP3D_WriteBlock(&blk);
         UP3D_Execute();
       }
       break;

      case '3':
       {
         UP3D_BLK blk;
         UP3D_BeginWrite();
UP3D_PROG_BLK_MoveL(&blk,21,23809,-7,0,0,-495,0,0);UP3D_WriteBlock(&blk);
UP3D_PROG_BLK_MoveL(&blk,1581,24984,-10924,0,0,0,0,0);UP3D_WriteBlock(&blk);
UP3D_PROG_BLK_MoveL(&blk,21,23809,-10403,0,0,495,0,0);UP3D_WriteBlock(&blk);
UP3D_PROG_BLK_MoveL(&blk,1,100,512,0,0,0,0,0);UP3D_WriteBlock(&blk);
UP3D_PROG_BLK_MoveL(&blk,21,23809,7,0,0,495,0,0);UP3D_WriteBlock(&blk);
UP3D_PROG_BLK_MoveL(&blk,1581,24984,10924,0,0,0,0,0);UP3D_WriteBlock(&blk);
UP3D_PROG_BLK_MoveL(&blk,1,100,512,0,0,0,0,0);UP3D_WriteBlock(&blk);
UP3D_PROG_BLK_MoveL(&blk,21,23809,10403,0,0,-495,0,0);UP3D_WriteBlock(&blk);
UP3D_PROG_BLK_MoveL(&blk,1,100,512,0,0,0,0,0);UP3D_WriteBlock(&blk);
         UP3D_PROG_BLK_Stop(&blk);UP3D_WriteBlock(&blk);
         UP3D_Execute();
       }
       break;

      case '4':
       {
         UP3D_BLK blk;
         UP3D_BeginWrite();
/*        
UP3D_PROG_BLK_MoveL(&blk,21,23809,-7,0,0,-495,0,0);UP3D_WriteBlock(&blk);
UP3D_PROG_BLK_MoveL(&blk,1581,24984,-10924,0,0,0,0,0);UP3D_WriteBlock(&blk);
//UP3D_PROG_BLK_MoveL(&blk,21,23809,-10403,0,0,495,0,0);UP3D_WriteBlock(&blk);
//UP3D_PROG_BLK_MoveL(&blk,1,100,512,0,0,0,0,0);UP3D_WriteBlock(&blk);
UP3D_PROG_BLK_MoveL(&blk,21,23809,-10403+24,0,0,495,0,0);UP3D_WriteBlock(&blk);
*/
UP3D_PROG_BLK_MoveL(&blk,21,23809,5,0,0,-495,0,0);UP3D_WriteBlock(&blk);
UP3D_PROG_BLK_MoveL(&blk,1581,24984,-10924,0,0,0,0,0);UP3D_WriteBlock(&blk);
UP3D_PROG_BLK_MoveL(&blk,21,23809,-10391,0,0,495,0,0);UP3D_WriteBlock(&blk);

         UP3D_PROG_BLK_Stop(&blk);UP3D_WriteBlock(&blk);
         UP3D_Execute();
       }
       break;

      case '5':
       {
         UP3D_BLK blk;
         UP3D_BeginWrite();
/*
UP3D_PROG_BLK_MoveL(&blk,21,23809,7,0,0,495,0,0);UP3D_WriteBlock(&blk);
UP3D_PROG_BLK_MoveL(&blk,1581,24984,10924,0,0,0,0,0);UP3D_WriteBlock(&blk);
//UP3D_PROG_BLK_MoveL(&blk,1,100,512,0,0,0,0,0);UP3D_WriteBlock(&blk);
//UP3D_PROG_BLK_MoveL(&blk,21,23809,10403,0,0,-495,0,0);UP3D_WriteBlock(&blk);
//UP3D_PROG_BLK_MoveL(&blk,1,100,512,0,0,0,0,0);UP3D_WriteBlock(&blk);
UP3D_PROG_BLK_MoveL(&blk,21,23809,10403+48,0,0,-495,0,0);UP3D_WriteBlock(&blk);
*/
UP3D_PROG_BLK_MoveL(&blk,21,23809,19,0,0,495,0,0);UP3D_WriteBlock(&blk);
UP3D_PROG_BLK_MoveL(&blk,1581,24984,10924,0,0,0,0,0);UP3D_WriteBlock(&blk);
UP3D_PROG_BLK_MoveL(&blk,21,23809,10440,0,0,-495,0,0);UP3D_WriteBlock(&blk);

         UP3D_PROG_BLK_Stop(&blk);UP3D_WriteBlock(&blk);
         UP3D_Execute();
       }
       break;

      case '6':
       {
         UP3D_BLK blk;
         UP3D_BeginWrite();

         UP3D_PROG_BLK_Stop(&blk);UP3D_WriteBlock(&blk);
         UP3D_Execute();
       }
       break;

      case '7':
       {
         UP3D_BLK blk;
         UP3D_BeginWrite();

         UP3D_PROG_BLK_Stop(&blk);UP3D_WriteBlock(&blk);
         UP3D_Execute();
       }
       break;

      case '8':
       {
         UP3D_BLK blk;
/*
         UP3D_BeginWrite();

  UP3D_SetProgramID( 3, true );
  
  UP3D_BLK blksHome[2];
  UP3D_BeginWrite();
  UP3D_PROG_BLK_Home( blksHome, UP3DAXIS_Z ); UP3D_WriteBlocks(blksHome,2);
  UP3D_PROG_BLK_Home( blksHome, UP3DAXIS_Y ); UP3D_WriteBlocks(blksHome,2);
  UP3D_PROG_BLK_Home( blksHome, UP3DAXIS_X ); UP3D_WriteBlocks(blksHome,2);

  UP3D_BLK blks[2];
  UP3D_PROG_BLK_MoveF( blks,-1000,0,-1000,0,-1000,-100,-1000,0);
  UP3D_WriteBlocks(blks,2);
  
         UP3D_PROG_BLK_Stop(&blk);UP3D_WriteBlock(&blk);
*/
  UP3D_BeginWrite();
  UP3D_SetProgramID( 3, false );
         UP3D_Execute();
       }
       break;

      case '9':
       {
         UP3D_BLK blk;
         UP3D_BeginWrite();
         UP3D_SetProgramID( 9, true );

         UP3D_PROG_BLK_SetParameter(&blk,PARA_BED_TEMP,100);UP3D_WriteBlock(&blk);
         UP3D_PROG_BLK_SetParameter(&blk,PARA_HEATER_BED_ON,1);UP3D_WriteBlock(&blk);

         UP3D_PROG_BLK_Stop(&blk);UP3D_WriteBlock(&blk);

         UP3D_BeginWrite();
         UP3D_SetProgramID( 9, false );
         UP3D_Execute();
       }
       break;


      case 'a':
       UP3D_SetParameter(0x94,99); //set smaller accuracy
       break;


      case 't':
       UP3D_SetParameter(0x39,65);  //NOZZLE1 SET TEMP
       UP3D_SetParameter(0x3A,65);  //NOZZLE2 SET TEMP
       UP3D_SetParameter(0x3B,102); //BED SET TEMP
       UP3D_SetParameter(0x3C,101); //TEMP4 SET TEMP 
       break;

      case 's':
       UP3D_SetParameter(0x10,2);
       break;

      case 'x':
       UP3D_SetParameter(0x14,0); //NOZZLE1 OFF
       UP3D_SetParameter(0x15,0); //NOZZLE1 OFF
       UP3D_SetParameter(0x16,0); //BED OFF
       break;
 
      case 'n':
       UP3D_SetParameter(0x14,1); //NOZZLE1 ON
       break;
      case 'm':
       UP3D_SetParameter(0x16,1); //BED ON
       break;

    }

    update_state(false);

    napms(20);
  }

  sigfinish(0);
  return 0;
}


/*ROMPROGS
+PROG00: INITIALIZE
PROG01: GO INIT POS X/Y
PROG02: BEFORE PRINT (BEEP, HEAT, BEEP, WAIT, BEEP)
PROG03: MOVE TO CENTER 10 OVER BED
PROG04: MOVE TO CENTER 10 OVER BED AND BEEP
+PROG05: FINALIZE+HOME (StopBuild)
+PROG06: FINALIZE+HOME AND TURN OFF
PROG07: EXTRUDE FILAMENT (HEATER-N1(14)=1, HEAT, BEEP, NOZZLE(17)=1, GOTO 0, NOZZLE(17)=2, EXTRUDE SLOW, RETRACT FAST, NOZZLE(17)=1, EXTRUDE SLOW, HEATER-N1(14)=0, BEEP )
PROG08: NOZZLE=1 EXTRUDE FILAMENT P17=1 (HEAT, BEEP, RETRACT FAST, EXTRUDE SLOW, BEEP)
PROG09: NOZZLE=2 EXTRUDE FILAMENT P17=2 (HEAT, BEEP, RETRACT FAST, EXTRUDE SLOW, BEEP)
PROG10: NOZZLE=1 REMOVE FILAMENT P17=1 (BEEP, HEAT, RETRACT-SLOW, RETRACT-FAST, BEEP)
PROG11: NOZZLE=2 REMOVE FILAMENT P17=2 (BEEP, HEAT, RETRACT-SLOW, RETRACT-FAST, BEEP)
PROG12: NOZZLE=1 RETRACT+EXTRUDE P17=1 (RETRACT FAST, EXTRUDE SLOW, EXTRUDE FAST)
PROG13: NOZZLE=2 RETRACT+EXTRUDE P17=2 (RETRACT FAST, EXTRUDE SLOW, EXTRUDE FAST)
*/
