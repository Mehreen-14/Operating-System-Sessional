#include<bits/stdc++.h>
#include<pthread.h>
#include<semaphore.h>
#include<unistd.h>
#include<cstdio>

using namespace std;
using namespace std::chrono;

const int NUM_PRINTING_STATIONS = 4;
const int NUM_BINDING_STATIONS = 2;
const int NUM_READERS = 2;

struct student{
    int groupid;
    int studentid;
    int leader;
};

time_t start,end;

vector<int> printers[NUM_PRINTING_STATIONS];
int notfree_printer[NUM_PRINTING_STATIONS];
pthread_mutex_t printer_lock[NUM_PRINTING_STATIONS];
vector<int> GroupSize;

int N,M,w,x,y;

//semaphores
sem_t binding_locks ;
vector<sem_t>leaderlock ;
vector<sem_t> studentlock;
pthread_mutex_t notfree_printer_lock[NUM_PRINTING_STATIONS];



///reader-writer 
sem_t readerMutex, writerMutex;
int readerCount = 0;
int submissionCount = 0;

void* groupLeader(void* args);
void* staffMember(void* args);



void* print(void* args,time_t end){
    student* stdnt = (student*) args;
    sem_wait(&studentlock[(stdnt->studentid)]);
    pthread_mutex_lock(&printer_lock[(stdnt->studentid % NUM_PRINTING_STATIONS)+1]) ;
    time(&end) ;
    //cout<<"Student "<<(stdnt->studentid)<<" has arrived at the print station "<<(stdnt->studentid%NUM_PRINTING_STATIONS)+1<<" at time "<<end-start<<endl;
    printf("Student %d has arrived at the print station %d at time %d\n",(stdnt->studentid),(stdnt->studentid%NUM_PRINTING_STATIONS)+1,(end-start));
    pthread_mutex_unlock(&printer_lock[(stdnt->studentid % NUM_PRINTING_STATIONS)+1]) ;
    

    sleep(w) ;
    pthread_mutex_lock(&printer_lock[(stdnt->studentid % NUM_PRINTING_STATIONS)+1]) ;
    time(&end) ;
    notfree_printer[(stdnt->studentid % NUM_PRINTING_STATIONS)+1] = 0;
    //cout<<"Student "<<(stdnt->studentid)<<" has finished printing at time "<<end-start<<endl;
    printf("Student %d has finished printing at time %d\n",(stdnt->studentid),(end - start));
    GroupSize[stdnt->groupid]-- ;
    pthread_mutex_unlock(&printer_lock[(stdnt->studentid % NUM_PRINTING_STATIONS)+1]) ;
    


    if(GroupSize[stdnt->groupid]==0){
        sem_post(&leaderlock[stdnt->groupid]);
    }


    ///groupmates
    for(int j = 0 ; j < printers[stdnt->studentid % NUM_PRINTING_STATIONS].size() ; j++) {
        //cout<<"size "<< printers[stdnt->studentid % NUM_PRINTING_STATIONS].size()<<endl<<endl;
        if((printers[stdnt->studentid % NUM_PRINTING_STATIONS][j])/M == stdnt->groupid ) {
            sem_post(&studentlock[printers[(stdnt->studentid % NUM_PRINTING_STATIONS)][j]]) ;
            sleep(2) ;
        }
    }
    

    ///Others
    for(int j = 0 ; j < printers[(stdnt->studentid % NUM_PRINTING_STATIONS)].size() ; j++) {
        if((printers[(stdnt->studentid % NUM_PRINTING_STATIONS)][j])/M != stdnt->groupid ) {
            sem_post(&studentlock[printers[(stdnt->studentid % NUM_PRINTING_STATIONS)][j]]) ;
            sleep(2) ;
        }
    }

    if(stdnt->studentid != stdnt->leader) {
         delete stdnt ;
         return (void*) 1 ;
     }
    
    sem_wait(&leaderlock[stdnt->groupid]) ;
    time(&end) ;
    //cout<<"Group "<<(stdnt->groupid)+1<<" has finished printing at time "<<end - start<<endl;
    printf("Group %d has finished printing at time %d\n",(stdnt->groupid)+1,(end - start));
    sleep(2) ;

     
    ///Binding
    sem_wait(&binding_locks);
    time(&end);
    //cout<<"Group "<<stdnt->groupid+1<<" has started binding at time "<<end - start<<endl;
    printf("Group %d has started binding at time %d\n",stdnt->groupid+1,(end - start));
    sem_post(&binding_locks);
     
    sleep(x);
    
    time(&end);
    //cout<<"Group "<<stdnt->groupid+1<<" has finished binding at time "<<end - start<<endl;
    printf("Group %d has finished binding at time %d\n",stdnt->groupid+1,(end - start));
    sem_post(&binding_locks);



    //Submissions

    //-----------writer--------------
    sem_wait(&writerMutex);
    sleep(y);       
     // Writing to the entry book
    submissionCount++;
    printf("Group Leader : %d , No of submissions : %d\n",stdnt->groupid+1,submissionCount);
    
    // Finished writing
    sem_post(&writerMutex);

    


}

void* bind(void* args,time_t end){
    student* stdnt = (student*) args;
    //sem_post(&leaderlock[stdnt->groupid]) ;
    
}

void* phases(void* args){
    time_t end;

    
    print(args,end);
    bind(args,end);
    //int staff_id[2];
    pthread_t staffthread1,staffthread2;
    int *num1 = (int *) malloc(sizeof(int));

    *num1 = 1;
    pthread_create(&staffthread1, NULL, staffMember, (void*) num1);
    sleep(1+rand()%3);
    int *num2 = (int *) malloc(sizeof(int));

    *num2 = 2;
    pthread_create(&staffthread2, NULL, staffMember, (void*) num2);
   
    
}


int main(){
    freopen("in.txt", "r", stdin);
    freopen("output.txt", "w", stdout);
    cin >> N >> M >> w >> x >> y;

    pthread_t thread_s[N] ;
    pthread_t submission[N/M];
    GroupSize = vector<int> (N/M);

    //locks
    studentlock = vector<sem_t>(N);
    leaderlock = vector<sem_t>(N/M);

    
   

    for(int i = 0 ; i < N/M ; i++) {
        sem_init(&leaderlock[i],0,1);
        sem_wait(&leaderlock[i]);
        GroupSize[i] = M;
    }

     //binding lock
    sem_init(&binding_locks,0,2) ;

    // Initialize semaphores
    sem_init(&readerMutex,0,3);
    sem_init(&writerMutex,0,4);

    time(&start) ;
    

    double lambda = 5.0; // Poisson distribution parameter for inter-arrival rate (lambda)

    // Seed the random number generator
    std::random_device rd;
    std::mt19937 gen(rd());

    // Poisson distribution to generate random inter-arrival times
    std::poisson_distribution<> interArrivalTimeDist(lambda);
    int students[N] ;

    
    
    for (int i = 0; i < N; ++i) {
        int interArrivalTime = interArrivalTimeDist(gen); // Generate random inter-arrival time

        std::this_thread::sleep_for(milliseconds(interArrivalTime));
        students[i] = i;
    }

    for(int i = 0 ; i < N ; i++) {
        sleep(1);
        student info ;
        
        info.studentid = students[i] ;
        info.leader = (students[i]/M)*M + M - 1 ;
        info.groupid = students[i]/M ;
        sem_init(&studentlock[students[i]], 0 , 1) ;
        pthread_mutex_lock(&notfree_printer_lock[students[i] % NUM_PRINTING_STATIONS]);
        if(notfree_printer[students[i]%NUM_PRINTING_STATIONS] == 0) {
            notfree_printer[students[i]%NUM_PRINTING_STATIONS] = 1 ;
            pthread_mutex_unlock(&notfree_printer_lock[students[i] % NUM_PRINTING_STATIONS]);
        }
        else {
            pthread_mutex_unlock(&notfree_printer_lock[students[i] % NUM_PRINTING_STATIONS]);
            sem_wait(&studentlock[students[i]]) ;
        }

        printers[students[i]%NUM_PRINTING_STATIONS].push_back(students[i]) ;
        student* studentinfo  = new student ;
        *studentinfo = info ;

        pthread_create(&thread_s[students[i]],NULL,phases,studentinfo) ;
        
    }



    for(int i = 0 ; i < N ;i++) {
        pthread_join(thread_s[students[i]],NULL) ;
    }

    
   
    // Clean up
    sem_destroy(&readerMutex);
    sem_destroy(&writerMutex);

    return 0;
}



void* staffMember(void* args) {
    int staffId = *(int*)args;
    //cout<<"staff from staffMember "<< staffId<<endl;

    while (true) {
        // Sleep with randomness
        int sleepTime = (staffId + 1) * 2; // Different sleep intervals for the two staff members
        sleep(sleepTime);

        // Staff member wants to read
        sem_wait(&readerMutex);
        readerCount++;
        if (readerCount == 1) {
            sem_wait(&writerMutex); // First reader locks the writer
        }
        sem_post(&readerMutex);
        // Reading process
       
        //cout << "Staff " << staffId << " has started reading the entry book at time " << time(NULL) - start << ". No. of submissions = " << submissionCount << endl;
        printf("Staff %d has started reading the entry book at time %d. No. of submissions = %d\n",staffId,time(NULL) - start,submissionCount); 
        sleep(y); // Reading takes y seconds
        // Reading finished
        sem_wait(&readerMutex);
        readerCount--;
        if (readerCount == 0) {
            sem_post(&writerMutex); // Last reader unlocks the writer
        }
        sem_post(&readerMutex);
    }
}


