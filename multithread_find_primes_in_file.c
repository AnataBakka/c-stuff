#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct{
	int *file_numbers;
	int start;
	int end;
} limits;

typedef struct{
	const int MAX_SIZE_INCREMENT;
	const int type_size;
	int length;
	int max_size;
	void *starting_malloc;
} malloc_vals;

void malloc_default(malloc_vals *args){
	args->max_size = args->MAX_SIZE_INCREMENT;
	args->starting_malloc = malloc(args->type_size*args->max_size);
	args->length = -1;

	if(!args->starting_malloc){
		printf("Error: not enough memory.");
		exit(1);
	}
}

void add_to_malloc(malloc_vals *args, void *value_to_add){
	args->length++;

	if(args->length >= args->max_size){
		args->max_size += args->MAX_SIZE_INCREMENT;
		void *realloc_vals = realloc(args->starting_malloc, args->type_size*args->max_size);
		
		if(!realloc_vals){
			printf("Error: not enough memory.");
			exit(1);
		}
		
		args->starting_malloc = realloc_vals;
	}
	
	//dereferencing to char (assuming to have the size
	//all the other types derive from, e.g 1 byte)
	char* malloc = args->starting_malloc;
	char* value = value_to_add;
	int start_malloc = args->length*args->type_size;

	for(int i = 0; i < args->type_size; i++){
		malloc[start_malloc+i] = value[i];
	}
}

//this feels very inefficient. maybe create a function
//just for one thread which finds all primes in order
//and passes them to the other threads?
int is_prime(int val){
	if(val < 2){
		return 0;
	}

	int stop = val / 2;

	for(int i = 2; i <= stop; i++){
		if(val%i == 0){
			return 0;
		}
	}
	
	return 1;
}

void* get_file_primes(limits* args){
	malloc_vals* primes = malloc(sizeof(*primes));
	*(int *)&primes->MAX_SIZE_INCREMENT = 100;
	*(int *)&primes->type_size = sizeof(int);
	malloc_default(primes);
	
	int start = args->start;
	int end = args->end;
	int *file_numbers = args->file_numbers;
	
	for(int i = start; i <= end; i++){
		if(is_prime(file_numbers[i])){
			add_to_malloc(primes, &(file_numbers[i]));
		}
	}

	pthread_exit(primes);
}

void main(int argc,char**argv){
	malloc_vals filename = {16, sizeof(char)};
	malloc_default(&filename);
	
	if(argc < 2){
		printf("Input the file name: ");
		
		char c;

		while((c = fgetc(stdin)) != '\n'){
			add_to_malloc(&filename, &c);
		}
	}
	else{
		char c;

		for(int i = 0; (c = argv[1][i]) != '\n'; i++){
			add_to_malloc(&filename, &c);
		}
	}
	
	{
		char c = NULL;
		add_to_malloc(&filename, &c);
	}
	
	FILE *f = fopen(filename.starting_malloc, "r");
	
	if(!f){
		printf("Error: file '%s' could not be opened.",filename.starting_malloc);
		exit(1);
	}

	int thread_limit;
	
	if(argc < 3){
		printf("Input the value for threads: ");
		scanf("%d",&thread_limit);
	}
	else{
		thread_limit = atoi(argv[2]);
	}
	
	if(thread_limit <= 0){
		printf("Error: threads cannot be less than 1.");
		exit(1);
	}

	pthread_t threadIds[thread_limit];

	malloc_vals file_numbers = {100, sizeof(int)};
	malloc_default(&file_numbers);

	int scanned_value;
	int error_check;

	while((error_check = fscanf(f,"%d",&scanned_value)) >= 1){
		add_to_malloc(&file_numbers, &scanned_value);
	}
	
	fclose(f);

	if(error_check >= 0 && error_check < 1){
		printf("Error(%d): file \"%s\" cannot be parsed.", error_check, filename.starting_malloc);
		exit(1);
	}
	
	if(file_numbers.length == -1){
		free(file_numbers.starting_malloc);
		return;
	}

	int breakpoint = file_numbers.length / thread_limit;
	int leftovers = file_numbers.length % thread_limit;
	
	limits boundary[thread_limit];

	boundary[0].start = 0;
	boundary[0].end = breakpoint;

	for(int i = 1; i <= leftovers; i++){
		boundary[i].start = boundary[i-1].end + 1;
		boundary[i].end = boundary[i-1].end + breakpoint + 1;
	}
	
	for(int i = leftovers + 1; i < thread_limit; i++){
		boundary[i].start = boundary[i-1].end + 1;
		boundary[i].end = boundary[i-1].end + breakpoint;
	}

	for(int i = 0; i < thread_limit; i++){
		boundary[i].file_numbers = file_numbers.starting_malloc;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_create(&threadIds[i], &attr, get_file_primes, &boundary[i]);
		printf("Thread %d created %d - %d. Delta = %d\n", i, boundary[i].start, boundary[i].end,
				boundary[i].end - boundary[i].start + 1); //+1 is to count the 0
	}

	malloc_vals* prime_list_ptrs[thread_limit];

	for(int i = 0; i < thread_limit; i++){
		pthread_join(threadIds[i], &prime_list_ptrs[i]);
	}
	
	printf("\n");

	int total_primes = 0;

	for(int i = 0; i < thread_limit; i++){
		malloc_vals* primes = prime_list_ptrs[i];
		int length = primes->length;
		int* starting_prime = primes->starting_malloc;
		
		printf("Thread %d primes %d:\n", i, length + 1);

		for(int i = 0; i <= length; i++){
			printf("%d\n",starting_prime[i]);
		}
		
		printf("\n");
		total_primes += length + 1;
		free(starting_prime);
		free(primes);
	}
	
	free(file_numbers.starting_malloc);
	free(filename.starting_malloc);

	printf("Total primes = %d", total_primes);
}
