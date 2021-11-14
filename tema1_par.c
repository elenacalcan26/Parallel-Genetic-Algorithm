#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<pthread.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

typedef struct _sack_object
{
	int weight;
	int profit;
} sack_object;

typedef struct _individual {
	int fitness;
	int *chromosomes;
    int chromosome_length;
	int cnt_chromosomes; // retine numarul de obiecte din rucsac
	int index;
} individual;

// structura folosita in incapsularea datelor pentru a le folosi in functia executate de thread-uri
typedef struct _my_arg {
	int P;	// numarul total de thread-uri
	int thread_id;	// id-ul thread-ului care se executa
	pthread_barrier_t *barrier;	// bariera folosita
	sack_object *objects;	// obiectele cu progitul si cantitatea lor
	int object_count;	// numarul total de obiecte
	int sack_capacity;	// capacitatea sacului
	int generations_count;	// numarul de generatii din input
	individual *curent_generation;	// generatia curenta
	individual *next_generation;	//urmatoarea generatie
} my_arg;


int read_input(sack_object **objects, int *object_count, int *sack_capacity, int *generations_count, int *num_threads, int argc, char *argv[])
{
	FILE *fp;

	if (argc < 4) {
		fprintf(stderr, "Usage:\n\t./tema1 in_file generations_count P\n");
		return 0;
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		return 0;
	}

	if (fscanf(fp, "%d %d", object_count, sack_capacity) < 2) {
		fclose(fp);
		return 0;
	}

	if (*object_count % 10) {
		fclose(fp);
		return 0;
	}

	sack_object *tmp_objects = (sack_object *) calloc(*object_count, sizeof(sack_object));

	for (int i = 0; i < *object_count; ++i) {
		if (fscanf(fp, "%d %d", &tmp_objects[i].profit, &tmp_objects[i].weight) < 2) {
			free(objects);
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);

	*generations_count = (int) strtol(argv[2], NULL, 10);
    *num_threads = atoi(argv[3]);
	
	if (*generations_count == 0) {
		free(tmp_objects);

		return 0;
	}

	*objects = tmp_objects;

	return 1;
}

void print_objects(const sack_object *objects, int object_count)
{
	for (int i = 0; i < object_count; ++i) {
		printf("%d %d\n", objects[i].weight, objects[i].profit);
	}
}

void print_generation(const individual *generation, int limit)
{
	for (int i = 0; i < limit; ++i) {
		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			printf("%d ", generation[i].chromosomes[j]);
		}

		printf("\n%d - %d\n", i, generation[i].fitness);
	}
}

void print_best_fitness(const individual *generation)
{
	printf("%d\n", generation[0].fitness);
}

void compute_fitness_function(const sack_object *objects, individual *generation, int start, int end, int sack_capacity)
{
	int weight;
	int profit;

	for (int i = start; i < end; ++i) {
		weight = 0;
		profit = 0;

		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			if (generation[i].chromosomes[j]) {
				weight += objects[j].weight;
				profit += objects[j].profit;
			}
		}

		generation[i].fitness = (weight <= sack_capacity) ? profit : 0;
	}
}

int cmpfunc(const void *a, const void *b)
{
	individual *first = (individual *) a;
	individual *second = (individual *) b;

	int res = second->fitness - first->fitness; // descrescator dupa fitness
	if (res == 0) {

		// crescator dupa numarul de obiecte din rucsac
		res = first->cnt_chromosomes - second->cnt_chromosomes; 
		if (res == 0) {
			return second->index - first->index; 
		}
	}

	return res;
}

void mutate_bit_string_1(const individual *ind, int generation_index)
{
	int i, mutation_size;
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	if (ind->index % 2 == 0) {
		mutation_size = ind->chromosome_length * 4 / 10;
		for (i = 0; i < mutation_size; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	} else {
		mutation_size = ind->chromosome_length * 8 / 10;
		for (i = ind->chromosome_length - mutation_size; i < ind->chromosome_length; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	}
}


void mutate_bit_string_2(const individual *ind, int generation_index)
{
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	for (int i = 0; i < ind->chromosome_length; i += step) {
		ind->chromosomes[i] = 1 - ind->chromosomes[i];
	}
}

void crossover(individual *parent1, individual *child1, int generation_index)
{
	individual *parent2 = parent1 + 1;
	individual *child2 = child1 + 1;
	int count = 1 + generation_index % parent1->chromosome_length;

	memcpy(child1->chromosomes, parent1->chromosomes, count * sizeof(int));
	memcpy(child1->chromosomes + count, parent2->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));

	memcpy(child2->chromosomes, parent2->chromosomes, count * sizeof(int));
	memcpy(child2->chromosomes + count, parent1->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));
}

void copy_individual(const individual *from, const individual *to)
{
	memcpy(to->chromosomes, from->chromosomes, from->chromosome_length * sizeof(int));
}

void free_generation(individual *generation)
{
	int i;

	for (i = 0; i < generation->chromosome_length; ++i) {
		free(generation[i].chromosomes);
		generation[i].chromosomes = NULL;
		generation[i].fitness = 0;
	}
}


/*
	functie executata de thread-uri pentru rezolvarea problemei rucsacului
	folosind algoritmul genetic
*/
void *run_genetic_algorithm(void *arg)
{
	// se iau datele din structura 
	my_arg *data = (my_arg*) arg;
	int object_count = data->object_count;
	int generations_count = data->generations_count;
	int sack_capacity = data->sack_capacity;
	sack_object *objects = data->objects;
	int id = data->thread_id;
	int P = data->P;
	individual *current_generation = data->curent_generation;
	individual *next_generation = data->next_generation;
	
	// se calculeaza inidici de start si de end pentru a imparti array-urile pthread-uri 
	int start_obj_cnt, end_obj_cnt; 
	start_obj_cnt = id * ((double)object_count / P);
	end_obj_cnt = MIN(object_count, (id + 1) * ((double)object_count / P));

	int count, cursor;
	individual *tmp = NULL;

	// se itereaza generatiile
	for (int k = 0; k < generations_count; ++k) {

		cursor = 0;

		// fiecare thread numara pe bucata lui din array obiectele din rucsac
		current_generation[k].cnt_chromosomes = 0;
		for (int i = start_obj_cnt; i < end_obj_cnt; i++) {
			current_generation[k].cnt_chromosomes += current_generation[k].chromosomes[i]; 
		}
		
		// fiecare thread calculeaza fitness-ul din bucata lui din array-ul de obiecte
		compute_fitness_function(objects, current_generation, start_obj_cnt, end_obj_cnt, sack_capacity);

		pthread_barrier_wait(data->barrier);
		
		// sortarea cromozomilor se face pe un singur thread, thread-ul cu id 0
		if (id == 0) 
			qsort(current_generation, object_count, sizeof(individual), cmpfunc);

		// operatiile de mutatie si de crossover se realizeaza pe un singur thread, thread-ul cu id 0		
		if (id == 0) {

			// se pastreaza primii 30% indivizi pentru generatie urmatoare
			count = object_count * 3 / 10;
			for (int i = 0; i < count; ++i) {
				copy_individual(current_generation + i, next_generation + i);
			}
			cursor = count;

			// se aplica prima varianta de mutatie pe primii 20% din indivizii din generatie
			count = object_count * 2 / 10;
			for (int i = 0; i < count; ++i) {
				copy_individual(current_generation + i, next_generation + cursor + i);
				mutate_bit_string_1(next_generation + cursor + i, k);
			}
			cursor += count;

			// se aplica a doua varianta de mutatie pe primii 20% din indivizii din generatie
			count = object_count * 2 / 10;
		
			for (int i = 0; i < count; ++i) {
				copy_individual(current_generation + i + count, next_generation + cursor + i);
				mutate_bit_string_2(next_generation + cursor + i, k);
			}

			cursor += count;

			// se aplica crossover intr-un punct pe primii 30% din generatie
			count = object_count * 3 / 10;

			if (count % 2 == 1) {
				copy_individual(current_generation + object_count - 1, next_generation + cursor + count - 1);
				count--;
			}
		
			for (int i = 0; i < count; i += 2) {
				crossover(current_generation + i, next_generation + cursor + i, k);
			}
		}

		// pe un singur thread se suprascrie generatie curenta cu noua generatie
		if (id == 0) {
			tmp = current_generation;
			current_generation = next_generation;
			next_generation = tmp;
		}
		
		pthread_barrier_wait(data->barrier);

		// asignare facuta de fiecare thread in bucata lui din array
		for (int i = start_obj_cnt; i < end_obj_cnt; ++i) {
			current_generation[i].index = i;
		}

		if (k % 5 == 0 && id == 0) {
			print_best_fitness(current_generation);
		}
		
		pthread_barrier_wait(data->barrier);
	}

	// fiecare thread calculeaza fitness-ul din bucata lui din array-ul de obiecte
	compute_fitness_function(objects, current_generation, start_obj_cnt, end_obj_cnt, sack_capacity);
	
	pthread_barrier_wait(data->barrier);
	
	// sortarea cromozomilor se face pe un singur thread, thread-ul cu id 0
	if (id == 0) {
		qsort(current_generation, object_count, sizeof(individual), cmpfunc);
		print_best_fitness(current_generation);
	} 

	pthread_exit(NULL);
}

int main(int argc, char *argv[]) {

	sack_object *objects = NULL;
	int object_count = 0;
	int sack_capacity = 0;
	int generations_count = 0;

    int num_threads, r;
	my_arg *arguments;
	pthread_t *threads;
	void *status;

	pthread_barrier_t barrier;

	if (!read_input(&objects, &object_count, &sack_capacity, &generations_count, &num_threads, argc, argv)) {
		return 0;
    }

	threads = (pthread_t*) malloc(num_threads * sizeof(pthread_t));
	arguments = (my_arg*) malloc(num_threads * sizeof(my_arg));

	individual *current_generation = (individual*) calloc(object_count, sizeof(individual));
	individual *next_generation = (individual*) calloc(object_count, sizeof(individual));

	// se creeaza generatia initiala
	for (int i = 0; i < object_count; ++i) {
		current_generation[i].fitness = 0;
		current_generation[i].chromosomes = (int*) calloc(object_count, sizeof(int));
		current_generation[i].chromosomes[i] = 1;
		current_generation[i].index = i;
		current_generation[i].chromosome_length = object_count;

		next_generation[i].fitness = 0;
		next_generation[i].chromosomes = (int*) calloc(object_count, sizeof(int));
		next_generation[i].index = i;
		next_generation[i].chromosome_length = object_count;
	}
	
	// se initializeaza bariera
	r = pthread_barrier_init(&barrier, NULL, num_threads);

	if (r) {
		printf("Error: init barrier\n");
		exit(-1);
	}

	// se creeaza thread-urile	
	for (int i = 0; i < num_threads; i++) {
		
		arguments[i].barrier = &barrier;
		arguments[i].thread_id = i;
		arguments[i].objects = objects;
		arguments[i].object_count = object_count;
		arguments[i].sack_capacity = sack_capacity;
		arguments[i].generations_count = generations_count;
		arguments[i].P = num_threads;
		arguments[i].curent_generation = current_generation;
		arguments[i].next_generation = next_generation;

		r = pthread_create(&threads[i], NULL, run_genetic_algorithm, &arguments[i]);

		if (r) {
			printf("Error: create thread!\n");
			exit(-1);
		}
	}

	// se face join la thread-uri
	for (int i = 0; i < num_threads; i++) {
		r = pthread_join(threads[i], &status);

		if (r) {
			printf("Eroare la asteptarea thread-ului %d\n", i);
			exit(-1);
		}
	}

	// dealocare bariera
	r = pthread_barrier_destroy(&barrier);

	if (r) {
		printf("Error: destroy barrier\n");
		exit(-1);
	}

	free_generation(current_generation);
	free_generation(next_generation);
	free(current_generation);
	free(next_generation);

	free(objects);
	free(threads);
	free(arguments);

    return 0;
}