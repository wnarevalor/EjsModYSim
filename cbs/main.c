#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "lcgrand.h"

#define Q_LIMIT 1000
#define BUSY 1
#define IDLE 0

// Model settings variables
float meanInterarrival, meanService, timeEnd;
int caseofsimulation; //1 para clientes pacientes, 2 para clientes impacientes, 3 para probabilidad 0.5
// Simulation clock
float simulationTime;

// State variables
int numInQueue, serverStatus[4];
float timeOfArrival[ Q_LIMIT + 1 ], timeOfNextEvent[ 4 ], timeOfLastEvent, departureTimes[4], deptim[2];

// Statistical counters
int numCustsDelayed;
float  totalOfDelays, totalCompletedSales, totalWastedSales;

int nextEventType, numEvents;

FILE *inFile, *outFile;

void initialize ( void );
void timing ( void );
void arrive ( void );
void depart ( void );
void report ( void );
float exponentialDistribution ( float mean );
int poisson2 ( float mean );
int checkifempty( void );
int probability( void );
void serverForDeparture(float array[], float departureTimes[]);

int main ( void ) {

   inFile  = fopen( "mm1.in",  "r" );
   outFile = fopen( "mm1.out", "w" );

   numEvents = 3;

   //fscanf( inFile, "%f %f %f %d", &meanInterarrival, &meanService, &timeEnd, &caseofsimulation);
   fscanf( inFile, "%f %f %f", &meanInterarrival, &meanService, &timeEnd);

   fprintf( outFile, "Single-server queueing system\n\n" );
   fprintf( outFile, "Mean interarrival time%11.3f minutes\n\n", meanInterarrival );
   fprintf( outFile, "Mean service time%16.3f minutes\n\n", meanService );
   fprintf( outFile, "Length of the simulation%9.3f minutes\n\n", timeEnd );
   //fprintf( outFile, "Simulation case %d \n\n", caseofsimulation );
   //fprintf(outFile, "Caso %d \n", caseofsimulation);

   for (int i=1; i<=3; ++i){
      caseofsimulation = i;
      fprintf(outFile, "Caso %d \n", caseofsimulation);
      initialize();
      do {
         timing();
         //updateTimeAvgStats();

         switch ( nextEventType ) {
            case 1:
               arrive();
               break;
            case 2:
               depart();
               break;
            case 3:
               report();
               break;
         }
      } while ( nextEventType != 3 );
   }

   fclose( inFile );
   fclose( outFile );
   return 0;

}

void initialize ( void ) {

   simulationTime  = 0.0;

   serverStatus[0] = IDLE;
   serverStatus[1] = IDLE;
   serverStatus[2] = IDLE;
   serverStatus[3] = IDLE;
   numInQueue      = 0.0;
   timeOfLastEvent = 0.0;
   deptim[0] = 0;
   deptim[1] = 0;
   //currentBusyServerToDispatch = 0;
   numCustsDelayed  = 0;
   totalOfDelays    = 0;
   totalCompletedSales = 0.0;
   totalWastedSales = 0.0;

   departureTimes[0] = 1.0e+30;
   departureTimes[1] = 1.0e+30;
   departureTimes[2] = 1.0e+30;
   departureTimes[3] = 1.0e+30;

   timeOfNextEvent[1] = simulationTime + poisson2( meanInterarrival );
   timeOfNextEvent[2] = 1.0e+30;
   timeOfNextEvent[3] = timeEnd;
}

void timing ( void ) {

   float minTimeOfNextEvent = 1.0e+29;
   nextEventType = 0;

   for ( int i = 1; i <= numEvents; ++i ) {
      if ( timeOfNextEvent[ i ] < minTimeOfNextEvent ) {

         minTimeOfNextEvent = timeOfNextEvent[ i ];
         nextEventType = i;
      }
   }

   if ( nextEventType == 0 ) {

      fprintf( outFile, "\nEvent list empty at time %f", simulationTime );
      exit( 1 );
   }

   simulationTime = minTimeOfNextEvent;
}

void arrive ( void ) {

   timeOfNextEvent[1] = simulationTime + poisson2(meanInterarrival);
   if (caseofsimulation == 1){
      if ( serverStatus[0] == BUSY && serverStatus[1] == BUSY && serverStatus[2] == BUSY && serverStatus[3] == BUSY){
         ++numInQueue;
         timeOfArrival[ numInQueue ] = simulationTime;
      }else{
         int empty = checkifempty();
         float delay    = 0.0;
         totalOfDelays += delay;
         ++numCustsDelayed;
         serverStatus[empty] = BUSY;
         departureTimes[empty] = simulationTime + exponentialDistribution( meanService );
         serverForDeparture(deptim, departureTimes);
         timeOfNextEvent[ 2 ] = deptim[0];
         }
   }
   if (caseofsimulation == 2){
      if ( serverStatus[0] == BUSY && serverStatus[1] == BUSY && serverStatus[2] == BUSY && serverStatus[3] == BUSY){
         ++totalWastedSales;
      }else{
         int empty = checkifempty();
         float delay    = 0.0;
         totalOfDelays += delay;
         ++numCustsDelayed;
         serverStatus[empty] = BUSY;
         departureTimes[empty] = simulationTime + exponentialDistribution( meanService );
         serverForDeparture(deptim, departureTimes);
         timeOfNextEvent[ 2 ] = deptim[0];
      }
   }
   if (caseofsimulation == 3){
      if ( serverStatus[0] == BUSY && serverStatus[1] == BUSY && serverStatus[2] == BUSY && serverStatus[3] == BUSY ){
         if (probability() >= 1){ //cliente espera
            ++numInQueue;
            timeOfArrival[numInQueue] = simulationTime;
         }else{
            ++totalWastedSales;
         }
      }else{
         int empty = checkifempty();
         float delay    = 0.0;
         totalOfDelays += delay;
         ++numCustsDelayed;
         serverStatus[empty] = BUSY;
         departureTimes[empty] = simulationTime + exponentialDistribution( meanService );
         serverForDeparture(deptim, departureTimes);
         timeOfNextEvent[ 2 ] = deptim[0];
      }
   }
}

void depart ( void ) {
   int server;
   if (caseofsimulation == 1){
      if (numInQueue <= 0){
            for (int i=0; i<4; ++i){
               if(departureTimes[i] == timeOfNextEvent[2] && serverStatus[i] == BUSY)
                  server = i;
            }
            serverStatus[server] = IDLE;
            departureTimes[server] = 1.0e+29;
            serverForDeparture(deptim, departureTimes);
            timeOfNextEvent[2] = deptim[0];
            //timeOfNextEvent[2] = 1.0e+30;
            totalCompletedSales += 40;
      }else{
         --numInQueue;
         float delay = simulationTime - timeOfArrival[1];
         totalOfDelays += delay;
         ++numCustsDelayed;
         int empty = checkifempty();
         departureTimes[empty] = simulationTime + exponentialDistribution( meanService );
         serverForDeparture(deptim, departureTimes);
         timeOfNextEvent[ 2 ] = deptim[0];
         //totalCompletedSales += 40;
         for (int i=1; i<= numInQueue; ++i){
            timeOfArrival[i] = timeOfArrival[i+1];
         }
      }
   }
   if (caseofsimulation == 2){
      for (int i=0; i<4; ++i){
         if(departureTimes[i] == timeOfNextEvent[2])
                  server = i;
            }
         serverStatus[server] = IDLE;
         departureTimes[server] = 1.0e+29;
         totalCompletedSales += 40;
         serverForDeparture(deptim, departureTimes);
         timeOfNextEvent[2] = deptim[0];
         //timeOfNextEvent[2] = 1.0e+30;
   }
   if (caseofsimulation == 3){ //igual al caso 1 ya que depende del numero de personas que dicidieron quedarse o no en  la cola
      if (numInQueue <= 0){
            for (int i=0; i<4; ++i){
               if(departureTimes[i] == timeOfNextEvent[2])
                  server = i;
            }
            serverStatus[server] = IDLE;
            departureTimes[server] = 1.0e+29;
            serverForDeparture(deptim, departureTimes);
            timeOfNextEvent[2] = deptim[0];
            //timeOfNextEvent[2] = 1.0e+30;
            totalCompletedSales += 40;
      }else{
         --numInQueue;
         float delay = simulationTime - timeOfArrival[1];
         totalOfDelays += delay;
         ++numCustsDelayed;
         int empty = checkifempty();
         departureTimes[empty] = simulationTime + exponentialDistribution( meanService );
         serverForDeparture(deptim, departureTimes);
         timeOfNextEvent[ 2 ] = deptim[0];
         //totalCompletedSales += 40;
         for (int i=1; i<= numInQueue; ++i){
            timeOfArrival[i] = timeOfArrival[i+1];
         }
      }
   }
}

void report ( void ) {

   //fprintf( outFile, "Number of delays completed %7d\n\n", numCustsDelayed );
   fprintf( outFile, "TV: $%7f\n",
            totalCompletedSales );
   fprintf( outFile, "TP: %7f\n\n",
            totalWastedSales );
}

float exponentialDistribution ( float mean ) {
   return -mean * log( lcgrand( 10 ) );
}

int poisson2 (float mean)  /* Poisson variate generation function. */
{
    float a = exp(-mean);
    float b = 1;
    int i = 0;
    while(b>=a){
        float u = lcgrand( 11 );
        b = b*u;
        i++;
    }
    /* Return a Poisson random variate with mean "mean". */
    return i;
}

int checkifempty( void ){
   int empty = 0;
   for (int k=0; k<4; ++k){
      if (serverStatus[k] == IDLE){
         empty = k;
         break;
      }
   }
   return empty;
}

int probability( void ){
    return rand() & 1;
}

void serverForDeparture(float array[], float departureTimes[]){

   float minDepTime = 1.0e+30;
   for ( int i = 0; i < 4; ++i){
      if (departureTimes[i] < minDepTime){
         minDepTime = departureTimes[i];
         array[0] = minDepTime;
         array[1] = i * 1.0;
      }
   }
}
