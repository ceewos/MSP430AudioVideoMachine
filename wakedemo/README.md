  //working switch code
  
  // if (switches & SW1 || switches & SW2 || switches & SW3 || switches & SW4) {
  //   if( data[songIndx].nPos == 4 )
  //     songIndx++;
  //   score ++;
  //   screen_update_score();
  //   buzzer_set_period(bp[ data[songIndx].song ]);
  //   songIndx++;
  //   if(songIndx >= songLength)
  //     songIndx = 0;
  // }
  // if (!switches) {
  //   buzzer_set_period(0);
  // }


  void checkPos(int button ){
  if( data[songIndx].nPos == 4 )
       songIndx++;
  buzzer_set_period(bp[ data[songIndx].song ]);
  songIndx++;
  score ++;
  screen_update_score();

  if(songIndx >= songLength)
      songIndx = 0;

}
