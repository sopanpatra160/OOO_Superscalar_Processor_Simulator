#include <stdio.h>
#include "cache.h"
#include <iostream>
#include <vector>
#include <string.h>
#include <math.h>

using namespace std;


//############################################################################################################
//CACHE INITIALIZATION
//############################################################################################################

cache::cache(unsigned int bs, unsigned int cs, unsigned int a, double rp, unsigned int wp, unsigned int lev){


	//printf("Initialization done\n");
	this->cs = cs;							//IF CACHE SIZE = 0 , SIMPLY RETURN TO NEXT INSTANTIATION AS THAT LEVEL IS UNDEFINED
	num_r=0;
	num_miss_r=0;
	num_w=0;
	num_miss_w=0;
	num_wb=0;
	num_wb_v=0;
	num_swap=0;
	mr=0;
	hit_time=0;
	mp=0;
	next = NULL;
	vptr = NULL;
	hit=0;
	miss=0;
	if(cs==0)return ;


	num_set = cs/(bs*a);
	num_bo= log2 (bs);
	num_ind= log2 (num_set);
	num_tag= 32-num_bo-num_ind; 
	
	this->bs = bs;
	this->a = a;
	this->rp = rp;
	this->wp = wp;
	this->lev= lev;

	//printf("num_set:%d",num_set);	
	//MEMORY ALLOCATION
	//(1)TAG
	tag_store = new unsigned int *[num_set];
	for (unsigned int i=0; i<num_set; i++){
		tag_store[i] = new unsigned int [a];			
	}
	for (unsigned int i=0; i<num_set; i++){
		for (unsigned int j=0; j<a; j++){
			tag_store[i][j]= 0;
		}
	}
	//(2)VALID
	valid_store = new unsigned int *[num_set];
	for (unsigned int i=0; i<num_set; i++){
		valid_store[i] = new unsigned int [a];			
	}
	for (unsigned int i=0; i<num_set; i++){
		for (unsigned int j=0; j<a; j++){
			valid_store[i][j]= 0;
		}
	}
	//LRU_STORE
	if(rp==2){
		lru_store = new unsigned int *[num_set];
		for (unsigned int i=0; i<num_set; i++){
			lru_store[i] = new unsigned int [a];			
		}
		for (unsigned int i=0; i<num_set; i++){
			for (unsigned int j=0; j<a; j++){
				lru_store[i][j]= a-j-1;
			}
		}
	}

	//LFU_BLOCK_STORE & LFU_SET_STORE
	if (rp==3){
		lfu_block_store = new unsigned int *[num_set];
		for (unsigned int i=0; i<num_set; i++){
			lfu_block_store[i] = new unsigned int [a];			
		}
		for (unsigned int i=0; i<num_set; i++){
			for (unsigned int j=0; j<a; j++){
				lfu_block_store[i][j]= 0;
			}
		}
		lfu_set_store = new unsigned int [num_set];
		for (unsigned int i=0; i<num_set; i++){
				lfu_set_store[i]= 0;
		}
	}
	//DIRTY_STORE
	if(wp==0){
		num_wb=0;
		dirty_store = new unsigned int *[num_set];
		for (unsigned int i=0; i<num_set; i++){
			dirty_store[i] = new unsigned int [a];			
		}
		for (unsigned int i=0; i<num_set; i++){
			for (unsigned int j=0; j<a; j++){
				dirty_store[i][j]= 0;
			}
		}
	}

}



//############################################################################################################
//CACHE READ OPERATION
//############################################################################################################


unsigned int cache::read(unsigned int addr){

	//printf("Entered read \n");
	unsigned int addr_ev;
	num_r++;
	//printf("Access no. %d\n",num_r);
	calc_tag_ind(addr);

	hit=0;
	miss=0;
		//printf("level = %d\n",lev);
	if(lev == 1){

		//#1 check for the element in all the blocks of that set
		for (unsigned int j=0; j<a; j++){
			if((tag_store[ind][j]==tag) && (valid_store[ind][j]==1)){
				if(rp==2)
					lru_update(ind,j);
				else if(rp==3)
					lfu_update_block(ind,j,1);
				hit=1;
				//printf("L1 Hit \n");				
				return 1;
			}
		}
		if(vptr != NULL){
			unsigned int tag_vc = vptr->calc_tag(addr); // tag reconstruction
			unsigned int found_vc=0;
			unsigned int found_loc = 0;
			for (unsigned int j=0; j<vptr->a; j++){
				if((vptr->tag_store[0][j]==tag_vc) && (vptr->valid_store[0][j]==1)){
					found_vc = 1;
					found_loc = j;
				}
			}
			if(found_vc==1){
				//unsigned int tag_vc = vptr->calc_tag(addr);
				num_swap+=1;
				swap(found_loc,0);
				return 1;
			}
		}
		num_miss_r++; // MISS in L1
		miss=1;
		//printf("L1 Miss \n");
		
		//#2 MISS in L1 but invalid case
		for (unsigned int j=0; j<a; j++){
			if(valid_store[ind][j]==0){
				if(next!=NULL)
					next->read(addr);
				tag_store[ind][j]=tag;
				valid_store[ind][j]=1;
				if(rp==2)
					lru_update(ind,j);
				else if(rp ==3) 
					lfu_update_block(ind,j,0);

				return 0;
			}
		}
		
		//#3 MISS IN L1 FOR ALL BITS VALID : EVICTION

		if(rp==2){
			ev_blk= lru_block2evict(ind);
		}
		
		else if(rp==3){
			ev_blk= lfu_block2evict(ind);
		}

		//See if victim cache is present
		if(vptr != NULL){
			addr_ev = ev_blk_addr(ind,tag_store[ind][ev_blk]);
			
			vptr->insert_vc(addr_ev,dirty_store[ind][ev_blk]);
			if(next!=NULL)
				next->read(addr);
			tag_store[ind][ev_blk]=tag;
			dirty_store[ind][ev_blk]=0;
		}
		
		else if(vptr == NULL){
			if(dirty_store[ind][ev_blk]==0){
				if(next!=NULL)
					next->read(addr);
				tag_store[ind][ev_blk]=tag;
			}
			if(dirty_store[ind][ev_blk]==1){
				num_wb+=1;
				addr_ev = ev_blk_addr(ind,tag_store[ind][ev_blk]);
				if(next!=NULL)
					next->write(addr_ev);
				if(next!=NULL)
					next->read(addr);
				tag_store[ind][ev_blk]=tag;
				dirty_store[ind][ev_blk]=0;
			}
		}

		//update LRU/LFU counters
		if(rp==2){
			lru_update(ind,ev_blk);
		}
		else if(rp==3){
			lfu_update_set(ind,ev_blk);
			lfu_update_block(ind,ev_blk,0);
		}
		return 0;
	}	

	else if(lev == 2){
		//#1 check for the element in all the blocks of that set
		for (unsigned int j=0; j<a; j++){
			if((tag_store[ind][j]==tag) && (valid_store[ind][j]==1)){
				if(rp==2)
					lru_update(ind,j);
				else if(rp==3)
					lfu_update_block(ind,j,1);
				hit=1;
				return 1;
			}
		}
		miss=1;	
		num_miss_r++;
		
		//#2 MISS in L1 but invalid case
		for (unsigned int j=0; j<a; j++){
			if(valid_store[ind][j]==0){
				tag_store[ind][j]=tag;
				valid_store[ind][j]=1;
				if(rp==2)
					lru_update(ind,j);
				else 
					lfu_update_block(ind,j,0);
			}
		}
		
		//#3 MISS IN L1 FOR ALL BITS VALID : EVICTION
		if(rp==2){
			ev_blk= lru_block2evict(ind);
		}
		//LFU policy
		else if(rp==3){
			ev_blk= lfu_block2evict(ind);
		}

		if(wp==0){
			if(dirty_store[ind][ev_blk]==0){
				tag_store[ind][ev_blk]=tag;
			}
			if(dirty_store[ind][ev_blk]==1){
				num_wb+=1;
				tag_store[ind][ev_blk]=tag;
				dirty_store[ind][ev_blk]=0;
			}
		}

		if(rp==2){
			lru_update(ind,ev_blk);
		}
		else if(rp==3){
			lfu_update_set(ind,ev_blk);
			lfu_update_block(ind,ev_blk,0);
		}
		return 0;
	}
	return 0;
}


//############################################################################################################
//CACHE WRITE OPERATION
//############################################################################################################

unsigned int cache::write(unsigned int addr){

	unsigned int addr_ev;
	num_w++;
	calc_tag_ind(addr);
		
	if(lev == 1){
		//#1 check for the element in all the blocks of that set	
		for (unsigned int j=0; j<a; j++){
			if((tag_store[ind][j]==tag) && (valid_store[ind][j]==1)){
				if(rp==2)
					lru_update(ind,j);
				else if(rp==3) 
					lfu_update_block(ind,j,1);
				if(wp==0){
					dirty_store[ind][j]=1;
				}
				return 1;
			}
		}
		if(vptr != NULL){
			unsigned int tag_vc = vptr->calc_tag(addr);
			unsigned int found_loc;
			unsigned int found_vc = 0;
			for (unsigned int j=0; j<vptr->a; j++){
				
				if((vptr->tag_store[0][j]==tag_vc) && (vptr->valid_store[0][j]==1)){
					found_vc = 1;
					found_loc = j;
				}
			}
			if(found_vc==1){
				num_swap++;
				swap(found_loc,1);
				return 1;
			}
		}

		num_miss_w++;
		
		//#2 MISS in L1 but invalid case
		for (unsigned int j=0; j<a; j++){
			if(valid_store[ind][j]==0){
				if(next!=NULL)
					next->read(addr);
				tag_store[ind][j]=tag;
				valid_store[ind][j]=1;
				dirty_store[ind][j]=1;
				if(rp==2)
					lru_update(ind,j);
				else 
					lfu_update_block(ind,j,0);
				return 0;
			}
		}
		
		//#3 MISS IN L1 FOR ALL BITS VALID : EVICTION
		//LRU
		if(rp==2){
			ev_blk= lru_block2evict(ind);
		}
		//LFU policy
		else if(rp==3){
			ev_blk= lfu_block2evict(ind);
		}


		if(vptr != NULL){
			addr_ev = ev_blk_addr(ind,tag_store[ind][ev_blk]);
			vptr->insert_vc(addr_ev,dirty_store[ind][ev_blk]);
			if(next!=NULL)
				next->read(addr);
			tag_store[ind][ev_blk]=tag;
			dirty_store[ind][ev_blk]=1;
			

		}
		else if(vptr == NULL){
			if(dirty_store[ind][ev_blk]==0){
				if(next!=NULL)
					next->read(addr);
				tag_store[ind][ev_blk]=tag;
				dirty_store[ind][ev_blk]=1;
			}
			else if(dirty_store[ind][ev_blk]==1){
				num_wb+=1;
				addr_ev = ev_blk_addr(ind,tag_store[ind][ev_blk]);
				if(next!=NULL)
					next->write(addr_ev);
				if(next!=NULL)
					next->read(addr);
				tag_store[ind][ev_blk]=tag;
				dirty_store[ind][ev_blk]=1;
			}
		}

		if(rp==2){
			lru_update(ind,ev_blk);
		}
		else if(rp==3){
			lfu_update_set(ind,ev_blk);
			lfu_update_block(ind,ev_blk,0);
		}
		return 0;

	}	
		
	if(lev == 2){
		//#1 check for the element in all the blocks of that set
		for (unsigned int j=0; j<a; j++){
			if((tag_store[ind][j]==tag) && (valid_store[ind][j]==1)){

				if(rp==2)
					lru_update(ind,j);
				else if(rp==3) 
					lfu_update_block(ind,j,1);
				if(wp==0){
					dirty_store[ind][j]=1;
				}

				return 1;
			}
		}

		num_miss_w+=1;

		//#2 MISS in L1 but invalid case	
		for (unsigned int j=0; j<a; j++){
			if(valid_store[ind][j]==0){
				if(wp == 0){
					tag_store[ind][j]=tag;
					valid_store[ind][j]=1;
					dirty_store[ind][j]=1;
					if(rp==2)
						lru_update(ind,j);
					else 
						lfu_update_block(ind,j,0);
				}

				return 0;
			}
		}
		
		//#3 MISS IN L1 FOR ALL BITS VALID : EVICTION	
		//LRU policy
		if(rp==2){
			ev_blk= lru_block2evict(ind);
		}
		//LFU policy
		else if(rp==3){
			ev_blk= lfu_block2evict(ind);
		}

		//update dirty according to write policy
		//WB policy
		if(wp == 0){
			if(dirty_store[ind][ev_blk]==0){
				tag_store[ind][ev_blk]=tag;
				dirty_store[ind][ev_blk]=1;
				if(rp==2){
					lru_update(ind,ev_blk);
				}
				else if(rp==3){
					lfu_update_set(ind,ev_blk);
					lfu_update_block(ind,ev_blk,0);
				}
			}
			else if(dirty_store[ind][ev_blk]==1){
				num_wb+=1;
				tag_store[ind][ev_blk]=tag;
				if(rp==2){
					lru_update(ind,ev_blk);
				}
				else if(rp==3){
					lfu_update_set(ind,ev_blk);
					lfu_update_block(ind,ev_blk,0);
				}
			}
		}
		return 0;
	}
	return 0;
}



//############################################################################################################
//CACHE TAG AND INDEX CACULATION
//############################################################################################################


void cache::calc_tag_ind(unsigned int addr){
	ind= ((( addr >> num_bo) << (num_bo+num_tag)) >> (num_bo+num_tag));
	tag=  addr >> (num_bo+num_ind);
}

//############################################################################################################
//LRU UPDATE FUNCTION
//############################################################################################################
void cache::lru_update(unsigned int ind, unsigned int blk){
	unsigned int tmp;
	tmp=lru_store[ind][blk];
	for (unsigned int j=0; j<a; j++){
		if(lru_store[ind][j]<tmp)
			lru_store[ind][j]++;
	}
	lru_store[ind][blk]=0;

}
//############################################################################################################
//LFU BLOCK UPDATE FUNCTION
//############################################################################################################
void cache::lfu_update_block(unsigned int ind, unsigned int blk, unsigned int hit){
	//For HIT  block count = block count +1
	if(hit == 1){
		lfu_block_store[ind][blk]+=1;
	}
	//For MISS block count = set count + 1
	else if(hit == 0){
		lfu_block_store[ind][blk]=lfu_set_store[ind]+1;
	}
}
//############################################################################################################
//CALCULATE TAG and RETURN 
//############################################################################################################
unsigned int cache::calc_tag(unsigned int addr){
        tag=  addr >> (num_bo+num_ind);
        return tag;
}

//############################################################################################################
//SWAP TAGS AND UPDATE ALL COUNTERS
//############################################################################################################
unsigned int cache::swap(unsigned int found_loc,unsigned int writeorread){
	//LRU policy
	if(rp==2){
		ev_blk= lru_block2evict(ind);
	}
	//LFU policy
	else if(rp==3){
		ev_blk= lfu_block2evict(ind);
	}
	
	unsigned int temp_tag = tag_store[ind][ev_blk];
	unsigned int temp_dirty = dirty_store[ind][ev_blk];
	unsigned int reconstr_addr_to_vc = ev_blk_addr(ind,temp_tag);	
	unsigned int reconstr_tag_to_vc = vptr->calc_tag(reconstr_addr_to_vc);

	tag_store[ind][ev_blk] = tag;
	if(writeorread)
		dirty_store[ind][ev_blk] = 1; 
	else
		dirty_store[ind][ev_blk] = vptr->dirty_store[0][found_loc];
	vptr->tag_store[0][found_loc] = reconstr_tag_to_vc;
	vptr->dirty_store[0][found_loc] = temp_dirty;
	//Update Victim Cache counters
	if(vptr->rp==2)
		vptr->lru_update(0,found_loc);
	//Update L1 Cache counters
	if(rp==2)
		lru_update(ind,ev_blk);
	else if(rp==3){
		lfu_update_set(ind,ev_blk);
		lfu_update_block(ind,ev_blk,0);
	}
	return 0;
}

//############################################################################################################
//CALCULATE LOCATION OF BLOCK TO BE EVICTED FOR LRU POLICY AND RETURN
//############################################################################################################
unsigned int cache::lru_block2evict(unsigned int ind){

	unsigned int max=0;
	for(unsigned int j=0;j<a; j++){	
		if(lru_store[ind][max]<lru_store[ind][j])
			max=j;
	}
	return max;
}

//############################################################################################################
//CALCULATE LOCATION OF BLOCK TO BE EVICTED FOR LFU POLICY AND RETURN
//############################################################################################################
unsigned int cache::lfu_block2evict(unsigned int ind){
	unsigned int min=0;
	for(unsigned int j=0; j<a; j++){	
		if(lfu_block_store[ind][min]>lfu_block_store[ind][j])
			min=j;
	}
	return min;
}



//############################################################################################################
//CACLULATE THE RECONSTRUCTED ADDRESS
//############################################################################################################
unsigned int cache::ev_blk_addr(unsigned int ind, unsigned int tag){
	unsigned int reconstr_addr = ((ind << num_bo) | (tag << (num_ind+num_bo)));
	return reconstr_addr;
}

//############################################################################################################
//UPDATE LFU SET COUNT
//############################################################################################################
void cache::lfu_update_set(unsigned int ind, unsigned int blk){
	lfu_set_store[ind]=lfu_block_store[ind][blk];
}

//############################################################################################################
//PUT EVICTED BLOCK IN VICTIM CACHE
//############################################################################################################
unsigned int cache::insert_vc(unsigned int addr_ev, unsigned int dirty2victim){
	unsigned int  reconstr_tag = calc_tag(addr_ev);
	//unsigned int  addr_ev;	
	
	for (unsigned int j=0; j<a; j++){
		if(valid_store[0][j] == 0){
			tag_store[0][j]=reconstr_tag;
			dirty_store[0][j]=dirty2victim;
			valid_store[0][j]=1;
			if(rp==2)
				lru_update(0,j);
			else 
				lfu_update_block(0,j,0);
			return 0;
		}
	}
	
	//#3. IF ALL VALID
	if(rp==2){
		ev_blk= lru_block2evict(0);
	}
	else if(rp==3){
		ev_blk= lru_block2evict(0);
	}

	if(dirty_store[0][ev_blk]==0){
		tag_store[0][ev_blk]=reconstr_tag;
		dirty_store[0][ev_blk]=dirty2victim;
	}
	else if(dirty_store[0][ev_blk]==1){
		num_wb_v+=1;
		addr_ev = ev_blk_addr(0,tag_store[0][ev_blk]);
		if(next!=NULL){
			next->write(addr_ev);
		}
		tag_store[0][ev_blk]=reconstr_tag;
		dirty_store[0][ev_blk]=dirty2victim;
	}
	if(rp==2)
		lru_update(0,ev_blk);
	else if(rp==3){
		lfu_update_set(0,ev_blk);
		lfu_update_block(0,ev_blk,0);
	}
	return 0;
}




void cache::print_statistics(){
	mr = (((float)num_miss_r+(float)num_miss_w)/((float)num_r+(float)num_w));
	unsigned int num_mem_traff = num_miss_r + num_miss_w + num_wb;
	//printf("\n%s","====== Simulation results (raw) ======\n");
	if(lev==1){
		printf("a. number of L%d reads: %d\n",lev,num_r);
		printf("b. number of L%d read misses: %d\n",lev,num_miss_r);
		printf("c. number of L%d writes: %d\n",lev,num_w);
		printf("d. number of L%d write misses: %d\n",lev,num_miss_w);
		printf("e. L%d miss rate: %.4f\n",lev,mr);
		printf("f. number of swaps:     %d\n",num_swap);
	}
	if(lev==0)
	{
			printf("g. number of victim cache writeback:     %d\n",num_wb_v);


	}

	if(lev==2){
		printf("h. number of L%d reads: %d\n",lev,num_r);
		printf("i. number of L%d read misses: %d\n",lev,num_miss_r);
		printf("j. number of L%d writes: %d\n",lev,num_w);
		printf("k. number of L%d write misses: %d\n",lev,num_miss_w);
		if(cs!=0)
			printf("l. L%d miss rate: %.4f\n",lev,mr);
		else
			printf("l. L%d miss rate: 0\n",lev);
		if(cs!=0)
			printf("m. number of L2 writeback: %d\n",num_wb);
		else
			printf("m. number of L2 writeback: 0\n");
			
		printf("%s %d\n","n. total memory traffic:",num_mem_traff);

	}

}

//Print tag matrix
void cache::print_tag_store(){
//printf("num set = %d \n",num_set);
	for(unsigned int i=0; i<num_set; i++){
	printf("set  %4d:  ",i);
		for(unsigned int j=0; j<a; j++){
			printf ("%8x ",tag_store[i][j]);
			if(wp == 0){
				if(dirty_store[i][j]==1)
					printf ("D");
				else
					printf (" ");
			}
		}
	printf ("\n");
	}
}



unsigned int cache :: tot_mem_traffic(){
	return (num_miss_r + num_miss_w + num_wb +num_wb_v );
}

