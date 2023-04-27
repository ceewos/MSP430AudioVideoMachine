#include <msp430.h>
#include <libTimer.h>
#include "lcdutils.h"
#include "lcddraw.h"
#include "buzzer.h"

// WARNING: LCD DISPLAY USES P1.0.  Do not touch!!! 

#define LED BIT6		/* note that bit zero req'd for display */

#define SW1 1
#define SW2 2
#define SW3 4
#define SW4 8

#define BACKGROUND  rgb2bgr(0x1002)

#define SWITCHES 15
#define NoteWidth 11
#define StartPos 0

// //   sixteenth, eight, dotted eight, quarter, dotted quarter, half
//int end[] =  {150, 300, 450, 600, 900,1200};
//int hold[] = {600, 1200,1800,2400, 3600,4800};
//char song[] = {9, 9, 5, 2,    -1, 2, -1, 7,    -1, 7, -1, 7,    11, 11, 12, 14,    12, 12, 12, 7 ,    -1, 7, -1, 9,    -1, 9, -1, 9,    7, 7, 9, 7};
//char song[]  = { 9,    9,    5,    2,    2,    7,    7,    7,   11,   11,   12,   14,   12,   12,    12,    7,    7,    9,    9,     9,    7,    7,    9,    7};
// char  hold[] = { 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,     0,    0,    0,    0,    0,     0,    0,    0,    0,    0};
// char  nPos[] = { 2,    2,    1,    0,    0,    1,    1,    1,    2,    2,    3,    3,    3,    3,     3,    1,    1,    2,    2,     2,    1,    1,    2,    1};


//buzzer pressure
unsigned int bp[] = { 3822, 3608, 3405, 3214, 3034, 2863, 2703, 2551, 2408, 2272, 2145, 2025, 1911, 1804, 1703,
                      1911, 1804, 1703, 1607, 1517, 1431, 1350, 1273, 1201, 1132, 1067, 1004,  948,  895,  845,
                         0 };

//divided by 300                    //hold time in milis
// unsigned short htm[]=  {4, 8, 16}; // unsigned short ht[]= {1200, 2400, 4800};
// unsigned short htme[]= {1, 2, 4}; // unsigned short hte[]= {300, 600, 1200};

unsigned short htm[]=  {2, 4, 8}; // unsigned short ht[]= {1200, 2400, 4800};
unsigned short htme[]= {1, 2, 4}; // unsigned short hte[]= {300, 600, 1200};


struct {
  unsigned song : 5; // 5 bits for song (0-32) // 30 = silent
  unsigned hold : 3; // 3 bits for hold (0-7)
  unsigned nPos : 3; // 3 bits for nPos (0-7) // 4 = no note played
} data[] = {

  { 9, 0, 2 }, { 9, 0, 2 }, { 5, 0, 1 }, { 2, 0, 0 },
  
  {30, 0, 4 }, { 2, 0, 0 }, { 30, 0, 4 }, { 7, 0, 1 }, 
  
  { 30, 0, 4 }, { 7, 0, 1 }, { 30, 0, 4 }, { 7, 0, 1 },
  
  { 11, 0, 2 }, { 11, 0, 2 }, { 12, 0, 3 }, { 14, 0, 3 },
  
  { 12, 0, 3 }, { 12, 0, 3 }, { 12, 0, 3 }, { 7, 0, 1 },
  
  {30, 0, 4 },{ 7, 0, 1 }, { 30, 0, 4 }, { 9, 0, 2 }, 
   
  {30, 0, 4 },{ 9, 0, 2 }, { 30, 0, 4 }, { 9, 0, 2 },
  
  { 7, 0, 1 }, { 7, 0, 1 }, { 9, 0, 2 }, { 7, 0, 1 }

};

volatile char songIndx = 0;
char songLength = 32;


static char 
switch_update_interrupt_sense()
{
  char p2val = P2IN;
  /* update switch interrupt to detect changes from current buttons */
  P2IES |= (p2val & SWITCHES);	/* if switch up, sense down */
  P2IES &= (p2val | ~SWITCHES);	/* if switch down, sense up */
  return p2val;
}

void 
switch_init()			/* setup switch */
{  
  P2REN |= SWITCHES;		/* enables resistors for switches */
  P2IE |= SWITCHES;		/* enable interrupts from switches */
  P2OUT |= SWITCHES;		/* pull-ups for switches */
  P2DIR &= ~SWITCHES;		/* set switches' bits for input */
  switch_update_interrupt_sense();
}

int switches = 0;
char expected =  4;

void switch_interrupt_handler() {
  char p2val = switch_update_interrupt_sense();
  switches = ~p2val & SWITCHES;

  if (switches & SW1 || switches & SW2 || switches & SW3 || switches & SW4) {
    while( data[songIndx].nPos == 4 ){
      songIndx++;
    }
    buzzer_set_period(bp[ data[songIndx].song ]);
    songIndx++;
    if(songIndx >= songLength)
      songIndx = 0;
  }
  if (!switches) {
    buzzer_set_period(0);
  }
}


// void
// switch_interrupt_handler()
// {
//   char p2val = switch_update_interrupt_sense();
//   switches = ~p2val & SWITCHES ;

//   if ( p2val & SW1 ) {
//     buzzer_set_period(0);
//   }else{
//     // while( data[songIndx].nPos == 4 ){
//     //   songIndx++;
//     // }
//     buzzer_set_period(bp[ data[songIndx].song ]);
//     songIndx++;
//     if(songIndx >= songLength)
//       songIndx = 0;
//   }

// }

// axis zero for col, axis 1 for row
unsigned char notePositions[5] = {
  screenWidth/8 - NoteWidth/2 + 1 ,
  screenWidth/4 + screenWidth/8 - NoteWidth/2 + 1 ,
  screenWidth/2 + screenWidth/8 - NoteWidth/2 + 1 ,
  screenWidth - screenWidth/8 - NoteWidth/2 + 1,
  screenWidth
};



// notesOnScreen is a queue that holds an array with info on {}
char notesOnScreen[16][3] = {}, noteCount = 0, front = 0, end = 0, max = 12;
void push( char yPos, unsigned char xPos ,unsigned char size){
  if (noteCount >= max)
      return;
  notesOnScreen[end][0] = yPos; // add the note to the end of the queue
  notesOnScreen[end][1] = xPos;
  notesOnScreen[end][2] = size;
  end = (end + 1) % max; // update the end position
  noteCount++;
}

void pop(){
  if (noteCount == 0)
      return;
  notesOnScreen[front][0] = 0; // remove the note from the front of the queue
  notesOnScreen[front][1] = 0;
  notesOnScreen[front][2] = 0;
  front = (front + 1) % max; // update the top position
  noteCount--;
}

unsigned char rowVelocity = 5; //, colVelocity = 1, colLimits[2] = {1, screenWidth/2};
char noteHeight[5] = { NoteWidth * 2 , NoteWidth * 6, NoteWidth * 12, NoteWidth * 24, NoteWidth/2 };
//Columns on screen
char mid = screenWidth/2, midL = screenWidth/4, midR = screenWidth/2 + screenWidth/4;

char drawNoteIndx = 0;
char mili300 = 0;
char goal = 4;

char noteHitTarget[2] = {screenHeight - NoteWidth*2, screenHeight - 2 };

void
screen_update_ball(){
  if( goal == mili300 ){
    push( StartPos, data[drawNoteIndx].nPos , data[drawNoteIndx].hold);
    mili300 = 0;
    goal = htm [ data[drawNoteIndx].hold ] ;
    drawNoteIndx++ ;
    if( drawNoteIndx >= songLength )
      drawNoteIndx = 0;
  }

  //traversing queue
  for(char i = 0, x = front ; i < noteCount ; i ++, x++){
    if(x >= max){
      x = 0; 
    }
        
    // if( notesOnScreen[x][0] > noteHitTarget[0] && notesOnScreen[x][0] < noteHitTarget[1] ){
    //   expected  =  notesOnScreen[x][1];
    //   songIndx++;
    //   if(songIndx >= songLength)
    //     songIndx = 0;
    // }
    draw_ball( notePositions[ notesOnScreen[x][1] ], notesOnScreen[x][0] - noteHeight[5], 5 ,BACKGROUND); /* erase */
    if ( notesOnScreen[x][0] >= screenHeight ){
      draw_ball( notePositions[ notesOnScreen[x][1] ], screenHeight - 20 , 1 ,BACKGROUND); /* erase at END */
      pop();
    }else{
        draw_ball( notePositions[ notesOnScreen[x][1] ],  notesOnScreen[x][0], notesOnScreen[x][2] ,COLOR_PINK); // else if( notesOnScreen[x][1] == 4 ){ draw_ball( notePositions[ 0 ],  notesOnScreen[x][0], notesOnScreen[x][2] ,BACKGROUND);
    }
    notesOnScreen[x][0] += rowVelocity;    

  }
}

void
draw_ball(char col, char row, char indx, unsigned short color)
{
  fillRectangle(col-1, row-1 , NoteWidth, noteHeight[indx], color);
}
  

short redrawScreen = 1;
u_int controlFontColor = COLOR_GREEN;
char mili100 = 0;

void wdt_c_handler()
{
  static int secCount = 0;
  secCount ++;
  if (secCount >= 5) {		/* 10/sec      75 = .3 (300 milis), 25 = .1 seconds (100 milis), 250 = 1 seconds (1000 milis  ), 125 = .5 seconds (500 milis ), 5 = .02 seconds(20 milis)*/
    secCount = 0;
    mili100 += 1;
    if(mili100 == 5){
      mili100 = 0;
      mili300 += 1;
    }
    redrawScreen = 1;
    screen_update_ball();
  }else{
    redrawScreen = 0;
  }
}
  
void main()
{
  buzzer_init();
  P1DIR |= LED;		/**< Green led on when CPU on */
  P1OUT |= LED;

  configureClocks();
  lcd_init();
  switch_init();

  clearScreen(BACKGROUND);
  screen_draw_columns();

  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */

}

void
screen_draw_columns()
{
  fillRectangle( 0,0, 3 , screenHeight, COLOR_MAGENTA);
  fillRectangle(midL -  1,0, 3 , screenHeight, COLOR_MAGENTA);
  fillRectangle(mid - 1,0, 3, screenHeight, COLOR_MAGENTA);
  fillRectangle(midR - 1,0, 3, screenHeight, COLOR_MAGENTA);
  fillRectangle( screenWidth - 3,0, 3 , screenHeight, COLOR_MAGENTA);

  fillRectangle(0, noteHitTarget[0] , screenWidth , 3, COLOR_YELLOW );
  fillRectangle(0, noteHitTarget[1] , screenWidth , 3, COLOR_YELLOW );
}
   
void
__interrupt_vec(PORT2_VECTOR) Port_2(){
  if (P2IFG & SWITCHES) {	      /* did a button cause this interrupt? */
    P2IFG &= ~SWITCHES;		      /* clear pending sw interrupts */
    switch_interrupt_handler();	/* single handler for all switches */
  }
}
