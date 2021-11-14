# Parallel-Genetic-Algorithm

<h5>Calcan Elena-Claudia <br/>
331CA</h5><br/>


   • tema consta in paralelizarea problemei rucsacului folosind un algoritm genetic
  
   • implementarea a pornit de la implementarea secventiala a acestei probleme
    
   • din implementarea secventiala s-au modificat urmatoarele lucruri:
        
       * setarea generatiei initiale se realizeaza in functia main()
       * obiectele din rucsac sunt numarate in functia run_genetic_algorithm()
          si salvate intr-o noua variabila din structura individual
       * in functia de compare folosita la sortare doar verifica valoarea de fitness
          si dupa numarul de obiecte din rucsac, daca este nevoie
        
   • thread-urile sunt create pe rand si sunt executate in functia run_genetic_algorithm()
	 
   • pentru paralizare s-au impartit array-urile pe thread-uri, fiecare thread avand
      o bucata din array pe care o executa in paralel
			
   • impartirea array-ului s-a realizat prin calcularea unor indici de start si de end
      dupa urmatoarele formule:

        int  start = id * (double) N / P
        int  end = MIN(n, (id + 1) * (double) N / P)), 
        
        unde id = id-ul thread-ului care se executa, N = numarul de obiecte, 
              P = numarul total de thread-uri

   • in functia de run_genetic_algoritm() s-au paralizat parcurgerile din interiorul
    primei itartii
   • de asemenea, s-a paralelizat cu aceeasi metoda prima iteratie din functia de 
    cmpute_fitness()

   • sortarea cromozomilor si operatiile de mutare, de crosover si de suprascriere  a 
      generatiilor sunt realizate de un singur thread, s-a considerat thread-ul cu id-ul 0

   • pentru sincronizarea thread-urilor s-a folosit bariera
   • bariera a fost pusa astfel:
        
       ‣ intre calcularea valorii de fitness si sortarea cromozomilor
       ‣ dupa ce generatiile au fost interschimbate
       ‣ la finalul instructiunilor care se executa in parcurgerea generatiilor
        
