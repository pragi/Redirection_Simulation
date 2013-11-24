
//INPUTS
// Lambda - argv[1]
// % Master is ON - argv[2]

//Credentials: Mehrgan and Dr.Christensen for their toolkit page

/*Sample output
************ BEGIN SIMULATION ************

=============================================================
==     *** M/D/1 Energy Save Project Simulation ***        ==
=============================================================
= Arrival rate                          = 500.000 reqs/sec
= Assistant Service rate        = 1000.000 reqs/sec
= Master Service rate           = 100.000 reqs/sec
= Total CPU time                        =  0.171 sec
= Simulation time                       = 1000.000 sec
= Epoch Period                          = 10.000
= Where Master is on  1.000 sec and off  9.000 sec
=============================================================
= >>> Efficiency Results                                    -
= Total master completions  = 4963 reqs
= Total master drops            = 53 reqs
= Total master reqs             = 5016 reqs
= % of Dropped requests =  1.057 % drops
=============================================================
==     *** Simulation Result for Master ***       ==
=============================================================
= >>> Simulation results for master server       -
=------------------------------------------------------------
= Utilization        = 92.822 %
= Mean throughput    =  4.963 reqs/sec
= Mean customers     = 20.217 reqs
= Mean delay    =  4.074 sec
= Total Completed = 4963 reqs
=============================================================

************ END SIMULATION ************
*/

#include "csim.h"  // For CSIM
#include <stdio.h> // For system

//Constants
#define SIM_TIME 10000.0  // Simulation time in seconds
 
FACILITY server_Assistant;		/* pointer for assistant facility */
FACILITY server_Master;			/* pointer for master facility */
EVENT on_event;					/* pointer for event on */
TABLE tbl;						/* pointer for table */

//Global variables
int epoch_time;					/* Entire On/Off cycle for master */
double master_serve;			/* Master service time */
double master_not_serve;		/* Master not service time */

int masterIsOn;					/* Master flag. Off(=0) or On(=1)*/

int total_reqs = 0;				/* Keeps track of total requests generated. Initialized to 0 */
int req_completed;				/* Total requests served by master */
int req_dropped;				/* Total requests not served by master */
int percent_on;				/* Keeps track of how much energy to save */


//Function declarations
void MasterController();
void Generate(double lambda, double mu_assistant, double mu_master);
void AssistantServerHandler(double service_time, double mu_master);
void MasterServerHandler(double service_time, double delay);
void ReportResults(double lambda, double service_rate_master, double service_rate_assistant);


void sim(int argc, char *argv[])
{
	double lambda;						/* Lambda input */
	double service_rate_master;			/* Constant service rate for master (reqs/sec) */
	double service_rate_assistant;		/* Mean service rate for assistant (reqs/sec) */
	
	// Initializing values based on given project information
	service_rate_master = 100.0;			/* Service rate of 10ms for master */
	service_rate_assistant = 1000.0;		/* Service rate of 1ms for assistant redirect */

	//Checks for the number of arguments. If incorrect prints help
	if (argc != 3)
	{
		printf("Error: Invalid Argument. The system accepts 2 arguments.\n");
		printf("Parameter 1: Master server lambda\n");
		printf("Parameter 2: Master server service percentage\n");
		printf("Example: 3 lambda with server on 80% of time :");
		printf("<...> 5 80\n");
		exit(1);
	}

	// Declare the values from command line inputs
	lambda = (double)atoi(argv[1]);
	percent_on = (double)atoi(argv[2])/100;

	//Generate simulation
	create("sim");

	//CSIM initialization
	server_Assistant = facility("Assistant");
	server_Master = facility("Master");
	on_event = event("on");
	
	//Calculate master serve and not serve time
	master_not_serve = 9;
	master_serve = 1;

	printf("************ BEGIN SIMULATION ************\n\n");


	MasterController();
	Generate(lambda, service_rate_master, service_rate_assistant);

	// Hold simulation for SIM_TIME
	hold(SIM_TIME);

	// Report Results

	ReportResults(lambda, service_rate_master, service_rate_assistant);

	
	printf("\n************ END SIMULATION ************\n");
	getch();
}

//Controls the aster server
void MasterController()
{
	create("MasterController");
	
	while(1)
	{
		hold(master_not_serve);

		masterIsOn = 1;
		set(on_event);
		hold(master_serve);
		masterIsOn = 0;

		clear(on_event);
	}
}


//Function to generate requests based on exponentially distributed interarrival times
//The generated requests are directed to the assistant server
// The function was generated using the files provided as class activities
void Generate(double lambda, double sr_master, double sr_assistant)
{
	double IAT; // interarrival time

	create("generate");

	while(1)
	{
		hold(exponential(1.0/lambda));
		total_reqs++;		// counts the total number of reqs generated
		AssistantServerHandler(1.0/sr_assistant, sr_master);
	}
}

//Assistant server checks if the master server is on, if so directly directs the reqs to it
//if not based on the algorithm calculations delays the request for a later time
void AssistantServerHandler(double service_time, double sr_master)
{
	create("server_Assistant");

	reserve(server_Assistant);
	hold(service_time);
	release(server_Assistant);

	if(masterIsOn == 1)
	{
		MasterServerHandler(1.0/ sr_master, 1.0/1000.0);
	}
	else
	{
		double delay_time;
		double percentile = 0;
		double current_time = simtime();
		percentile = (current_time/SIM_TIME)*100;
		//printf("%f\n", percentile);
		
		//This algorithms determines how much to delay the requests by
		if (percentile > 10 && percentile <20)
			delay_time = 3;
		else if (percentile >= 20 && percentile < 30)
			delay_time = 1.5;
		else if (percentile >=30 && percentile < 70)
			delay_time = 0;
		else if (percentile >=70 && percentile < 80)
			delay_time = 1.5;
		else if (percentile >= 80 && percentile < 90)
			delay_time = 3;
		else
			delay_time = 0;

		//double current_time;
		MasterServerHandler(1.0/sr_master, delay_time);
	}
}

//If the length of que is more than 50 the req is dropped
//otherwise the request is processed
void MasterServerHandler(double service_time, double delay)
{
	create("server_Master");
	hold(delay);

	if(qlength(server_Master) > 50)
	{
		req_dropped++;			//keeps track of total reqs dropped
	}
	else
	{	
		reserve(server_Master);
		if( masterIsOn == 0)
		{
			wait(on_event);
		}

		hold(service_time);
		release(server_Master);
		req_completed++;		//keeps track of total reqs completed
	}
}


//Outputs the results of the simulation
void ReportResults(double lambda, double service_rate_master, double service_rate_assistant)
{
	double arrival_rate;				/* Mean arrival rate (requests/sec) */
	double utilization_assistant;		/* Assistant Utilization */
	int total_dropped = total_reqs - req_completed;
	double percent_dropped = (double)(total_dropped*100)/total_reqs;

	arrival_rate = lambda * service_rate_master;
	utilization_assistant = arrival_rate / service_rate_assistant;


	printf("============================================================= \n");
	printf("==     *** M/D/1 Energy Save Project Simulation ***        == \n");
	printf("============================================================= \n");
	printf("= Arrival rate				= %6.3f reqs/sec   \n", arrival_rate);
	printf("= Assistant Service rate	= %6.3f reqs/sec   \n", service_rate_assistant);
	printf("= Master Service rate		= %6.3f reqs/sec   \n", service_rate_master);
	printf("= Total CPU time			= %6.3f sec      \n", cputime());
	printf("= Simulation time			= %6.3f sec      \n", clock);
	printf("= Epoch Period				= %6.3f			 \n", master_serve+master_not_serve);
	printf("= Where Master is on %6.3f sec and off %6.3f sec \n", master_serve, master_not_serve);
	printf("============================================================= \n");
	printf("= >>> Efficiency Results                                    - \n");
	printf("= Total master completions  = %ld reqs       \n", req_completed);
	printf("= Total master drops		= %ld reqs		 \n", total_dropped);
	printf("= Total master reqs		= %ld reqs		 \n", total_reqs);
	printf("= %% of Dropped requests = %6.3f %% drops \n" , percent_dropped );
	printf("============================================================= \n");
	printf("==     *** Simulation Result for Master ***       == \n");
	printf("============================================================= \n");
	printf("= >>> Simulation results for master server       - \n");
	printf("=------------------------------------------------------------ \n");
	printf("= Utilization        = %6.3f %%       \n", 100.0 * util(server_Master));
	printf("= Mean throughput    = %6.3f reqs/sec \n", tput(server_Master));
	printf("= Mean customers     = %6.3f reqs     \n", qlen(server_Master));
	printf("= Mean delay	= %6.3f sec      \n", resp(server_Master));
	printf("= Total Completed = %ld reqs	\n", completions(server_Master)) ;
	printf("============================================================= \n");
}

