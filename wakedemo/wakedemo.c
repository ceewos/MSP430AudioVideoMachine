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
#define THEME  COLOR_DEEP
#define NOTE COLOR_GOLD

#define SWITCHES 15
#define NoteWidth 11
#define StartPos 0
// //   sixteenth, eight, dotted eight, quarter, dotted quarter, half
//int end[] =  {150, 300, 450, 600, 900,1200};
//int hold[] = {600, 1200,1800,2400, 3600,4800};
//buzzer pressure
unsigned int bp[] = { 3822, 3608, 3405, 3214, 3034, 2863, 2703, 2551, 2408, 2272, 2145, 2025, 1911, 1804, 1703,
                      1911, 1804, 1703, 1607, 1517, 1431, 1350, 1273, 1201, 1132, 1067, 1004,  948,  895,  845,
                      0 };


                      // eight, quarter, dotted quarter, half, whole
unsigned short htm[]=  {3, 6, 9, 12, 24}; // unsigned short ht[]= {1200, 2400, 4800};
unsigned short htme[]= {1, 2, 3, 4, 8}; // unsigned short hte[]= {300, 600, 1200};

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
  { 7, 0, 1 }, { 7, 0, 1 }, { 9, 0, 2 }, { 7, 0, 1 },
  //32

  { 9, 0, 2 }, { 9, 0, 2 }, { 5, 0, 1 }, { 2, 0, 0 },
  {30, 0, 4 }, { 2, 0, 0 }, { 30, 0, 4 }, { 7, 0, 1 }, 

  { 30, 0, 4 }, { 7, 0, 1 }, { 30, 0, 4 }, { 7, 0, 1 },
  { 11, 0, 2 }, { 11, 0, 2 }, { 12, 0, 3 }, { 14, 0, 3 },

  { 12, 0, 3 }, { 12, 0, 3 }, { 12, 0, 3 }, { 9, 0, 1 },
  {30, 0, 4 },{ 7, 0, 1 }, { 30, 0, 4 }, { 9, 0, 2 }, 

  {30, 0, 4 },{ 9, 0, 2 }, { 30, 0, 4 }, { 9, 0, 2 },
  { 7, 1, 0},{ 7, 1, 0},
  //30

  { 5, 2, 3},{ 5, 1, 3},{4,0,2},{2,1,1},
  {30,1,4},{30,1,4},{30,1,4},{30,1,4},
  //8

  {4,0,2}, { 4, 1, 2}, {4,2,2}, {0,1,0},
  //4
  {30,0,4},{9,0,3},{30,0,4},{9,0,3},
  { 9, 1, 3},{ 7, 1, 2},
  //6
  { 5, 2, 1},{ 5, 0, 1},{ 5, 0, 1},{4,1,0},
  {2,2,0}, {30,1,4}, {30,1,4}, {2,0,0},
  {4,1,1}
  //9
};
char songLength = 89;

// axis zero for col, axis 1 for row
unsigned char notePositions[5] = {
  screenWidth/8 - NoteWidth/2 + 1 ,
  screenWidth/4 + screenWidth/8 - NoteWidth/2 + 1 ,
  screenWidth/2 + screenWidth/8 - NoteWidth/2 + 1 ,
  screenWidth - screenWidth/8 - NoteWidth/2 + 1,
  screenWidth
};

unsigned char rowVelocity = 5; // note speed
char noteHeight[5] = { NoteWidth * 3 , NoteWidth * 6, NoteWidth * 12, NoteWidth * 24, NoteWidth/2 }; //Note Size

char noteHitTarget[2] = {screenHeight - NoteWidth*2 - NoteWidth/2, screenHeight + 5 };

static char switch_update_interrupt_sense(){
  char p2val = P2IN;
  /* update switch interrupt to detect changes from current buttons */
  P2IES |= (p2val & SWITCHES);	/* if switch up, sense down */
  P2IES &= (p2val | ~SWITCHES);	/* if switch down, sense up */
  return p2val;
}
void switch_init(){  
  P2REN |= SWITCHES;		/* enables resistors for switches */
  P2IE |= SWITCHES;		/* enable interrupts from switches */
  P2OUT |= SWITCHES;		/* pull-ups for switches */
  P2DIR &= ~SWITCHES;		/* set switches' bits for input */
  switch_update_interrupt_sense();
}

// notesOnScreen is a queue that holds an array with info on {}
char notesOnScreen[16][4] = {}, noteCount = 0, front = 0, end = 0, max = 15;
void push( char yPos, unsigned char xPos ,unsigned char size, char songPos){
  if (noteCount < max){
    notesOnScreen[end][0] = yPos; // add the note to the end of the queue
    notesOnScreen[end][1] = xPos;
    notesOnScreen[end][2] = size;
    notesOnScreen[end][3] = songPos;
    end = (end + 1) % max; // update the end position
    noteCount++;
  }
}
void pop(){
  if (noteCount > 0){
    notesOnScreen[front][0] = 0; // remove the note from the front of the queue
    notesOnScreen[front][1] = 0;
    notesOnScreen[front][2] = 0;
    notesOnScreen[front][3] = 0;
    front = (front + 1) % max; // update the top position
    noteCount--;
  }
}

int switches = 0;
char expected =  4;
void switch_interrupt_handler() {
  char p2val = switch_update_interrupt_sense();
  switches = ~p2val & SWITCHES;
  switch (switches) {
    case SW1:
      checkPos(0);
      break;
    case SW2:
      checkPos(1);
      break;
    case SW3:
      checkPos(2);
      break;
    case SW4:
      checkPos(3);
      break;
    default:
      buzzer_set_period(0);
      break;
  }
}

char score = 0;
  // Indx, Column
char noteInPos[2] = {0,0};// hold musical note info, and 

void checkPos(int button ){
  if(noteInPos[1] == button ){
    buzzer_set_period(bp[ data[ noteInPos[ 0] ].song ]);
    score ++;
    screen_update_score();
  }

}

char drawNoteIndx = 0, mili300 = 0, goal = 4, dequeue = 0, onEnd = 0 ;

void screen_update_notes(){
  if( goal == mili300 ){
    mili300 = 0;
    if( onEnd ){
      push( StartPos, data[drawNoteIndx].nPos , data[drawNoteIndx].hold , drawNoteIndx );
      goal = htm [ data[drawNoteIndx].hold ] ;
      if( drawNoteIndx >= songLength )
        drawNoteIndx = 0;
      onEnd = 0;
    }else{
      goal = htme[ data[drawNoteIndx].hold ] ;
      onEnd = 1;
      drawNoteIndx++ ;
    }
  }
  for(char i = 0, x = front ; i < noteCount ; i ++, x++){ //traversing queue
    if(x >= max)
      x = 0; 
    draw_note( notePositions[ notesOnScreen[x][1] ], notesOnScreen[x][0] - noteHeight[5], 5 ,BACKGROUND); /* erase */

    if ( notesOnScreen[x][0] >= screenHeight ){ 
      draw_note( notePositions[ notesOnScreen[x][1] ], screenHeight - 20 , 1 ,BACKGROUND); /* erase at END */
      dequeue = 1;
    }else{
      if( notesOnScreen[x][0] + noteHeight[notesOnScreen[x][2]] >= noteHitTarget[0] && notesOnScreen[x][0] <= noteHitTarget[1]){
        noteInPos[0] = notesOnScreen[x][3];
        noteInPos[1] = notesOnScreen[x][1];
      }
      draw_note( notePositions[ notesOnScreen[x][1] ],  notesOnScreen[x][0], notesOnScreen[x][2] , NOTE ); // else if( notesOnScreen[x][1] == 4 ){ draw_ball( notePositions[ 0 ],  notesOnScreen[x][0], notesOnScreen[x][2] ,BACKGROUND);
    }
    notesOnScreen[x][0] += rowVelocity;    
  }

  if ( dequeue ){
    pop();
    dequeue = 0;
  }

}
void draw_note(char col, char row, char indx, unsigned short color){
  fillRectangle(col-1, row-1 , NoteWidth, noteHeight[indx], color);
}
  
short redrawScreen = 1;
char mili100 = 0;
void wdt_c_handler(){
  static int secCount = 0;
  secCount ++;
  if (secCount >= 3) {		/* 10/sec      75 = .3 (300 milis), 25 = .1 seconds (100 milis), 250 = 1 seconds (1000 milis  ), 125 = .5 seconds (500 milis ), 5 = .02 seconds(20 milis)*/
    secCount = 0;
    mili100 += 1;
    if(mili100 == 3){
      mili100 = 0;
      mili300 += 1;
    }
    redrawScreen = 1;
    screen_update_notes();
  }else{
    redrawScreen = 0;
  }
}

void main(){
  buzzer_init();
  configureClocks();
  lcd_init();
  switch_init();
  clearScreen(BACKGROUND);
  screen_draw_columns();
  screen_update_score( );
  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */
}


char scoreStr[3] = {'\0', '\0', '\0'};
void screen_update_score(){
  itoa(scoreStr);
  fillRectangle( screenWidth/2 - 7 , 14, 15, 11, THEME );
  drawString5x7( screenWidth/2 - 5 ,  16 , scoreStr  , NOTE , THEME );
  //drawUpsideDownTriangle(screenWidth/2 - 7,14 + 11, 15, THEME );

}
void itoa( char *str ){
    if(score < 10){
        str[0] = '0';
        str[1] = score + '0';
        str[2] = '\0';
        return;
    }else if( score > 99){
      score = 0;
    }
    char scoreCpy = score;
    char digit = scoreCpy % 10;
    str[1] = digit + '0';
    scoreCpy /= 10;
    digit = scoreCpy % 10;
    str[0] = digit + '0';
    str[2] = '\0';
}

void screen_draw_columns(){
  char mid = screenWidth/2, midL = screenWidth/4, midR = screenWidth/2 + screenWidth/4; //Columns on screen
  fillRectangle( 0,0, 3 , screenHeight, COLOR_MAGENTA);
  fillRectangle(midL -  1,0, 3 , screenHeight, COLOR_MAGENTA);
  fillRectangle(mid - 1,0, 3, screenHeight, COLOR_MAGENTA);
  fillRectangle(midR - 1,0, 3, screenHeight, COLOR_MAGENTA);
  fillRectangle( screenWidth - 3,0, 3 , screenHeight, COLOR_MAGENTA);

  fillRectangle(0, noteHitTarget[0] , screenWidth , 3, THEME );
  fillRectangle(0, screenHeight - 2, screenWidth , 3, THEME );
}
   
void
__interrupt_vec(PORT2_VECTOR) Port_2(){
  if (P2IFG & SWITCHES) {	      /* did a button cause this interrupt? */
    P2IFG &= ~SWITCHES;		      /* clear pending sw interrupts */
    switch_interrupt_handler();	/* single handler for all switches */
  }
}

void drawChar8x12(u_char rcol, u_char rrow, char c,
     u_int fgColorBGR, u_int bgColorBGR)
{
  u_char col = 0;
  u_char row = 0;
  u_int bit = 0x02;
  u_char oc = c - 0x20;

  lcd_setArea(rcol, rrow, rcol + 7, rrow + 11); /* relative to requested col/row */
  while (row < 12) {
    while (col < 8) {
      u_int colorBGR = (font_8x12[oc][col] & bit) ? fgColorBGR : bgColorBGR;
      lcd_writeColor(colorBGR);
      col++;
    }
    col = 0;
    bit <<= 1;
    row++;
  }
}

void drawString8x12(u_char col, u_char row, char *string,
    u_int fgColorBGR, u_int bgColorBGR)
{
  u_char cols = col;
  while (*string) {
    drawChar8x12(cols, row, *string++, fgColorBGR, bgColorBGR);
    cols += 9;
  }
}
