fx fx/buggy_fire
{

{
   delay 0
   duration 0.1
   restart 0
   particle ff_smokesparks.prt 
   offset 0, 0, 2
   trackorigin 1
}


{
   delay 0
   name "lightspectrum"
   duration 0.1
   restart 0
   light "lights/squareblast_noShadows", 3, 2, 0, 60  //500
   fadeOut 0.1
   offset 5, 0, 10
   trackorigin 1
}


/*
{
   delay 0
   duration 2
   restart 1
   particle ff_smokesparks2.prt
   offset 0, 0, 2
   trackorigin 0
}
  */
}
