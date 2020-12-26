#include "cache.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

using namespace std;

enum state_type {IF,ID,IS,EX,WB};
unsigned int tag_num;
unsigned int cycles;
unsigned int new_name[128];
bool	     rdy[128];
unsigned int n,s,blocksize,l1size,l1assoc,l2size,l2assoc;


struct instruction {

	unsigned int 	operation_type;
	bool		src1_ready;
	bool		src2_ready; 
	int		dest_regno;
	int		src1_regno;
	int		src2_regno;	
	int		src1_regno_new;
	int		src2_regno_new;
	unsigned int	if_begin_cycle,if_duration;
	unsigned int	id_begin_cycle,id_duration;
	unsigned int    is_begin_cycle,is_duration;
        unsigned int    ex_begin_cycle,ex_duration;
	unsigned int    wb_begin_cycle,wb_duration;
	unsigned int 	ex_latency;
	unsigned int 	mem_latency;
	state_type	state;
	unsigned int	tag;
	unsigned int	mem_addr;
};

	vector<instruction>	fake_rob;
	vector<instruction> 	dispatch_list;
	vector<instruction>	issue_list;
	vector<instruction>	execute_list;


//--------------------------------FETCH---------------------------------------------------------------------------------
// Read new instructions from the trace as long as 
//1) you have not reached the end-of-file,
//2) the fetch bandwidth is not exceeded,and
//3) the dispatch queue is not full.

//Then, for each incoming instruction:

//1) Push the new instruction onto the fake-ROB. 
//Initialize the instruction’s data structure,including setting its state to IF.
//2) Add the instruction to the dispatch_list and reserve a dispatch queue entry (e.g.,increment a count of the number
// of instructions in the dispatch queue).
//----------------------------------------------------------------------------------------------------------------------	

void Fetch(FILE *file, unsigned int n)
{
	unsigned int 	pc;
        unsigned int    op_type;
        int             dest_reg_num;
        int             src1_reg_num;
        int             src2_reg_num;
	unsigned int	fetch_count = 0;
	unsigned int 	mem_address;

	struct instruction instr;
	
	while( !feof(file) && fetch_count<n && dispatch_list.size()<2*n )
        {
	        
        	fscanf (file,"%x %d %d %d %d %x\n",&pc,&op_type,&dest_reg_num,&src1_reg_num,&src2_reg_num,&mem_address);
		instr.operation_type 	= 	op_type;
		instr.dest_regno	=	dest_reg_num;
		instr.src1_regno	=	src1_reg_num;
		instr.src2_regno	=	src2_reg_num;
		instr.src1_regno_new	=	src1_reg_num;
		instr.src2_regno_new	=	src2_reg_num;
		instr.state		=	IF;
		instr.tag		=	tag_num;
		instr.if_begin_cycle	=	cycles;
		instr.ex_latency	=	1;
		instr.mem_latency	=	0;
		if(op_type ==2 )
		{
		instr.mem_addr		=	mem_address;
		}

		dispatch_list.push_back(instr);
		fake_rob.push_back(instr);
		tag_num++;
		fetch_count++;
	
	}
	
}
//--------------------------------DISPATCH---------------------------------------------------------------------------------
// From the dispatch_list, construct a temp list of instructions in the ID
// state (don’t include those in the IF state – you must model the
// 1 cycle fetch latency). Scan the temp list in ascending order of
// tags and, if the scheduling queue is not full, then:

// 1) Remove the instruction from the dispatch_list and add it to the
// issue_list. Reserve a schedule queue entry (e.g. increment a
// count of the number of instructions in the scheduling
// queue) and free a dispatch queue entry (e.g. decrement a count of
// the number of instructions in the dispatch queue).

// 2) Transition from the ID state to the IS state.

// 3) Rename source operands by looking up state in the register
// file; rename destination operands by updating state in
// the register file. For instructions in the  dispatch_list that are in the IF
// state, unconditionally transition to the ID state (models the 1 cycle
// latency for instruction fetch).
//----------------------------------------------------------------------------------------------------------------------

void Dispatch(unsigned int s,unsigned int n)
{

	vector<instruction>     temp_dispatch_list;
  	unsigned int		dispatch_count=0;

	for (vector<instruction>::iterator it = dispatch_list.begin() ; it != dispatch_list.end(); ++it)
	{
		if(it->state == ID)
		{
			temp_dispatch_list.push_back(*it);			
		}
	}

	for (vector<instruction>::iterator it = temp_dispatch_list.begin() ; it != temp_dispatch_list.end(); ++it)
        {
                if(issue_list.size() < s && dispatch_count < n)
                {
			
			dispatch_count ++;
			it->state=IS;
			if(it->src1_regno_new != -1)
			{
				if(rdy[it->src1_regno_new])
				{
					it->src1_ready=1;
				}
				else
				{
					it->src1_ready=0;
					it->src1_regno_new=new_name[it->src1_regno];
				}
			}
			else
			{
                                it->src1_ready=1;
	
			}
			

			if(it->src2_regno_new != -1)
			{
				if(rdy[it->src2_regno_new])

                        	{
                                	it->src2_ready=1;
                        	}
                        	else
                        	{
                                	it->src2_ready=0;
					it->src2_regno_new=new_name[it->src2_regno];
                        	}
		
			}
			else
			{
                                it->src2_ready=1;
	
			}

			if (it->dest_regno != -1) 
			{
				rdy[it->dest_regno]=0;
				new_name[it->dest_regno] = it->tag;
			}
			it->id_duration = cycles - it->id_begin_cycle;
			it->is_begin_cycle = cycles;
			
			issue_list.push_back(*it);
			//temp_dispatch_list.remove(*it);
			dispatch_list.erase(dispatch_list.begin());
		}
	}
	for (vector<instruction>::iterator it = dispatch_list.begin() ; it != dispatch_list.end(); ++it)
	{
		if(it->state == IF)
		{
			it->state = ID;
			it->if_duration = cycles - it->if_begin_cycle;
			it->id_begin_cycle = cycles;
		}
	}

}

//--------------------------------ISSUE---------------------------------------------------------------------------------
// From the issue_list, construct a temp list of instructions whose
// operands are ready – these are the READY instructions. Scan the READY
// instructions in ascending order of tags and issue up to N of them.
// To issue an instruction:

// 1) Remove the instruction from the issue_list and add it to the execute_list.
// 2) Transition from the IS state to the EX state.
// 3) Free up the scheduling queue entry (e.g., decrement a count of the number of instructions in
// the scheduling queue)
// 4) Set a timer in the instruction’s data structure that will allow
// you to model the execution latency.
//----------------------------------------------------------------------------------------------------------------------

void Issue(unsigned int n)
{
	vector<instruction>     temp_issue_list;

	for (vector<instruction>::iterator it = issue_list.begin() ; it != issue_list.end(); ++it)
        {
		if(it->src1_ready && it->src2_ready)
		{
			temp_issue_list.push_back(*it);			
		}
	}

	unsigned int issue_count = 0;
	for (vector<instruction>::iterator it = temp_issue_list.begin() ; it != temp_issue_list.end(); ++it)
        {
	
		if(issue_count < n)
                {

                        it->state=EX;
                        it->is_duration = cycles - it->is_begin_cycle;
                        it->ex_begin_cycle = cycles;
			execute_list.push_back(*it);			

			for(int i = 0; i < issue_list.size(); i++)
                        {
                                if(issue_list[i].tag == it->tag)
                                {
					issue_list.erase(issue_list.begin()+i);
                                }
                        }

			//mp_issue_list.remove(*it);
			//issue_list.erase(issue_list.begin());
			
			issue_count++ ;
		}		
	}
}

//--------------------------------EXECUTE-------------------------------------------------------------------------------
// From the execute_list, check for instructions that are finishing execution this cycle, and:
// 1) Remove the instruction from the execute_list.
// 2) Transition from EX state to WB state.
// 3) Update the register file state(e.g., ready flag) and wakeup
// dependent instructions (set their operand ready flags).
//----------------------------------------------------------------------------------------------------------------------

void Execute(cache *l1,cache *l2,cache *v)
{

	vector <unsigned int> delete_inst_tag_list;
	for (vector<instruction>::iterator it = execute_list.begin() ; it != execute_list.end(); ++it)
        {

		if(it->operation_type == 2 && l1size !=0)
		{
			if(it->mem_latency == 0)
			{
				l1->read(it->mem_addr);
				if ((l1->hit  ==	1) && (l2size != 0) )			it->mem_latency = 5;
				if ((l1->miss ==	1) && (l2->hit == 1) && (l2size != 0))	it->mem_latency = 10;
				if ((l1->miss ==	1) && (l2->miss== 1) && (l2size != 0)) 	it->mem_latency = 20;
				if ((l1->hit  == 1) && (l2size == 0) )			it->mem_latency = 5;
				if ((l1->miss == 1) && (l2size == 0) )                   it->mem_latency = 20;
				
				it->ex_latency ++;
			}
			else
			{
				if(it->ex_latency == it->mem_latency)
				{				
					it->state=WB;
					it->ex_duration = cycles - it->ex_begin_cycle;
					it->wb_begin_cycle = cycles;
					it->wb_duration = 1;
					
					for(vector<instruction>::iterator it1 = fake_rob.begin() ; it1 != fake_rob.end(); ++it1)
					{
						//printf("Here2 ! \n");
						if(it1->tag == it->tag)
						{
								
							*it1 = *it;
						}
					}		
					
					if(it->dest_regno != -1)
					{
						for(vector<instruction>::iterator it2 = issue_list.begin() ; it2 != issue_list.end(); ++it2)
                        			{
							//printf("Here3 ! \n");
                        				if(it2->src1_regno_new ==  it->tag)
							{
								it2->src1_ready = 1;
							}
							if(it2->src2_regno_new ==  it->tag)
                        			        {
								it2->src2_ready = 1;
                        			        }

						}
						if(it->tag == new_name[it->dest_regno])
						{
							rdy[it->dest_regno]=1;
							new_name[it->dest_regno] = it->dest_regno;
						}
					}
					
					//execute_list.erase(it);
					delete_inst_tag_list.push_back(it->tag);
				}
				
				else
				{
						//printf("Here:1\t%d\n", it->tag);
						it->ex_latency++;
				}
			}
		}

		else
		{
			if(	(it->operation_type == 0 && it->ex_latency == 1) || 
				(it->operation_type == 1 && it->ex_latency == 2) || 
				(it->operation_type == 2 && it->ex_latency ==5)
			)

			{
				//printf("Here1 ! \n");
				it->state=WB;
				it->ex_duration = cycles - it->ex_begin_cycle;
				it->wb_begin_cycle = cycles;
				it->wb_duration = 1;
				
				for(vector<instruction>::iterator it1 = fake_rob.begin() ; it1 != fake_rob.end(); ++it1)
				{
				//printf("Here2 ! \n");
					if(it1->tag == it->tag)
					{
							
						*it1 = *it;
					}
				}		
				
				if(it->dest_regno != -1)
				{
					for(vector<instruction>::iterator it2 = issue_list.begin() ; it2 != issue_list.end(); ++it2)
                	        	{
				//printf("Here3 ! \n");
                	        		if(it2->src1_regno_new ==  it->tag)
						{
							it2->src1_ready = 1;
						}
						if(it2->src2_regno_new ==  it->tag)
                	        	        {
							it2->src2_ready = 1;
                	        	        }

					}
					if(it->tag == new_name[it->dest_regno])
					{
						rdy[it->dest_regno]=1;
						new_name[it->dest_regno] = it->dest_regno;
					}
				}
				
				//execute_list.erase(it);
				delete_inst_tag_list.push_back(it->tag);
			}
			else
			{
				//printf("Here:1\t%d\n", it->tag);
				it->ex_latency++;
			}
        	}

	}

	for (unsigned int i = 0; i < delete_inst_tag_list.size(); i++) {
		for (unsigned int j = 0; j < execute_list.size(); j++) {
			if (execute_list[j].tag == delete_inst_tag_list[i]) {
				execute_list.erase(execute_list.begin() + j);

			}
		}
	}


}

//--------------------------------FAKE RETIRE---------------------------------------------------------------------------
// Remove instructions from the head of the fake-ROB until an instruction is
// reached that is not in the WB state.
//----------------------------------------------------------------------------------------------------------------------

void FakeRetire()
{
	  //printf ("Checkpoint:1\n");
			//printf("Here4 ! \n");
	  if (fake_rob.empty() != 1) {
		  while(fake_rob.begin()->state==WB && fake_rob.empty() !=1)
		  {
			   //printf ("Checkpoint:2\n");
			   printf ("%d", fake_rob.begin()->tag);
			   printf (" fu{%d}", fake_rob.begin()->operation_type);
			   printf (" src{%d,%d}", fake_rob.begin()->src1_regno, fake_rob.begin()->src2_regno);
			   printf (" dst{%d}", fake_rob.begin()->dest_regno);
			   printf (" IF{%d,%d}", fake_rob.begin()->if_begin_cycle, fake_rob.begin()->if_duration);
			   printf (" ID{%d,%d}", fake_rob.begin()->id_begin_cycle, fake_rob.begin()->id_duration);
			   printf (" IS{%d,%d}", fake_rob.begin()->is_begin_cycle, fake_rob.begin()->is_duration);
			   printf (" EX{%d,%d}", fake_rob.begin()->ex_begin_cycle, fake_rob.begin()->ex_duration);
			   printf (" WB{%d,%d}\n", fake_rob.begin()->wb_begin_cycle, fake_rob.begin()->wb_duration);

			   fake_rob.erase(fake_rob.begin());
		  }
	  }


	return;
}


//--------------------------------ADVANCE CYCLE---------------------------------------------------------------------------
// Advance_Cycle performs several functions.
// 
// First, if you want to use the scope tool (below), then it checks for
// instructions that changed states and maintains a history of
// these transitions. When an instruction is removed from the fake-ROB –- FakeRetire()
// function -- you can dump out this timing history in the format read by the scope tool.
// 
// Second, it advances the simulator cycle.
//
//  Third, when it becomes known that the fake-ROB is empty AND the trace is depleted,
//  the function returns “false” to terminate the loop.
//----------------------------------------------------------------------------------------------------------------------
bool Advance_Cycle(FILE *file)
{
	if(fake_rob.empty() && feof(file))
	{
		return false;
	}
	return true;
}



int main(int argc, char *argv[]) {

        char *tracefile;
	FILE *file;
	tag_num=0;
	cycles=0;
	//-------------RMT-----------------

	for(unsigned int i=0; i<127; i++)
	{
		rdy[i] = 1;
		new_name[i] = i;
	}
	//---------------------------------


 	s         = atoi (argv[1]);
        n         = atoi (argv[2]);
        blocksize = atoi (argv[3]);
	l1size	  = atoi (argv[4]);
	l1assoc	  = atoi (argv[5]);
	l2size	  = atoi (argv[6]);
	l2assoc	  = atoi (argv[7]);
	tracefile = 	  argv[8] ;


	cache l1(blocksize,l1size,l1assoc,2,0,1);      
	cache l2(blocksize,l2size,l2assoc,2,0,2);              
	cache v(4,0,0,2,0,0);               

	if(l2size!=0){
                l1.next = &l2;
                l2.next = NULL;

        }
        else{
                l1.next = NULL;
	}
	l1.vptr=NULL;
	v.next=NULL;
        

         file = fopen (tracefile,"r");

	//printf("level now= %d %d %d\n",l1.lev,l2.lev,v.lev);
	 while(Advance_Cycle(file))
	 {
         	FakeRetire();
         	Execute(&l1,&l2,&v);
         	Issue(n);
         	Dispatch(s,n);
		Fetch(file,n);
		cycles = cycles + 1;	
	 }
	fclose(file);


	if(l1size!=0){
		printf("L1 CACHE CONTENTS\n");
		printf("a. number of accesses :%d\n",l1.num_r);
		printf("b. number of misses :%d\n",l1.num_miss_r);
		l1.print_tag_store();
	}
	

	if(l2size!=0){
		printf("L2 CACHE CONTENTS\n");
		printf("a. number of accesses :%d\n",l2.num_r);
		printf("b. number of misses :%d\n",l2.num_miss_r);
		l2.print_tag_store();
	}
	if(l1size!=0) {printf("\n");}

	printf ("CONFIGURATION\n");
        printf (" superscalar bandwidth (N) = %d\n",n );
        printf (" dispatch queue size (2*N) = %d\n", 2*n);
        printf (" schedule queue size (S)   = %d\n", s);
        printf ("RESULTS\n");
        printf (" number of instructions = %d\n", tag_num);
        printf (" number of cycles       = %d\n", cycles-1);
        printf (" IPC                    = %0.2f\n", float(tag_num)/float(cycles-1));

}


  	
   	
   	
      
      
      
      
   	
			//printf("Reached this place\n");
			//flag = l1.read(it->mem_addr);
			//printf("flag = %d\n",flag);
			//printf("But not  here\n");
			//if ((l1.hit  ==	1) && (l2size != 0) )			latency = 5;
			//if ((l1.miss ==	1) && (l2.hit == 1) && (l2size != 0))	latency = 10;
			//if ((l1.miss ==	1) && (l2.miss== 1) && (l2size != 0)) 	latency = 20;
			//if ((l1.hit  == 1) && (l2size == 0) )			latency = 5;
			//if ((l1.miss == 1) && (l2size == 0) )                   latency = 20;
			//printf("latency = %d \n",latency);

			//}
			//else
			//{
			//	latency = 5;
			//}
