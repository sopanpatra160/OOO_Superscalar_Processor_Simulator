class cache{
	public:
	
		cache(unsigned int bs, unsigned int cs, unsigned int a, double rp, unsigned int wp, unsigned int lev); //CONSTRUCTOR
		
		cache *next;					//GENERIC POINTER TO NEXT LEVEL CACHE
		cache *vptr;					//POINTER TO VICTIM CACHE
		
		unsigned int bs;				//BLOCKSIZE
		unsigned int cs;				//CACHE SIZE
		unsigned int a;					//ASSOCIATIVITY
		unsigned int rp;				//READ POLICY - SAME AS LAMBDA
		unsigned int wp;				//WRITE POLICY
		unsigned int lev;				//CACHE LEVEL
		
		unsigned int **tag_store;			//TAG STORE 2-D ARRAY
		unsigned int **valid_store;			//VALID STORE 2-D ARRAY
		unsigned int **dirty_store;			//DIRTY STORE 2-D ARRAY
		unsigned int **lru_store;			//LRU STORE 2-D ARRAY
		unsigned int **lfu_block_store;			//LFU BLOCK_COUNT STORE 2-D ARRAY
		unsigned int *lfu_set_store;			//LFU SET STORE 1-D ARRAY
		
		unsigned int num_bo;				//NUMBER OF BLOCK OFFSET BITS
		unsigned int num_ind;				//NUMBER OF BITS FOR INDEX		
		unsigned int num_tag;				//NUMBER OF BITS FOR TAG
		unsigned int tag;				//TAG VALUE
		unsigned int ind;				//INDEX VALUE
		unsigned int ev_blk;				//LOCATION oF EVICTED BLOCK
		unsigned int num_set;				//NUMBER OF SETS
				
		unsigned int num_wb;				//NUMBER OF WRITEBACKS FROM A LEVEL OF CACHE
		unsigned int num_wb_v;				//NUMBER OF WRITEBACKS FROM VICTIM CACHE
		unsigned int num_swap;				//NUMBER OF SWAPS WITH VICTIM CACHE
		unsigned int num_r;				//NUMBER OF READS
		unsigned int num_miss_r;			//NUMBER OF READ MISSES
		unsigned int num_w;				//NUMBER OF WRITES
		unsigned int num_miss_w;			//NUMBER OF WRITE MISSES
		float mr;					//MISS RATE	
		float hit_time;					//HIT TIME				
		float mp;					//MISS PENALTY
		unsigned int hit;
		unsigned int miss;

		unsigned int read(unsigned int addr);						//CACHE READ OPERATION
		unsigned int write(unsigned int addr);						//CACHE WRITE OPERATION
		void lru_update(unsigned int ind, unsigned int blk);				//LRU UPDATE FUNCTION	
		void lfu_update_block(unsigned int ind, unsigned int blk, unsigned int hit);	//LFU BLOCK_COUNT UPDATE FUNCTION
		void lfu_update_set(unsigned int ind, unsigned int blk);			//LFU SET_COUNT UPDATE FUNCTION
		unsigned int lru_block2evict(unsigned int ind);
		unsigned int lfu_block2evict(unsigned int ind);
		void calc_tag_ind(unsigned int addr);
		unsigned int ev_blk_addr(unsigned int ind, unsigned int tag);
		unsigned int calc_tag(unsigned int addr);
		unsigned int calculate_index(unsigned int addr);
		unsigned int swap(unsigned int found_loc, unsigned int writeorread);
		unsigned int insert_vc(unsigned int addr, unsigned int dirtytovictim);
		
		void print_tag_store();
		void print_statistics();
		unsigned int tot_mem_traffic();
};


