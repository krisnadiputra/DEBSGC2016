//omiting calculation if there is no enough relations

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <ctime>
#include <thread>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <chrono>
#include <iomanip>

using namespace std;
typedef std::chrono::duration<int,std::milli> millisecs_t ;

int K_Q2,d;

// String tokenizers
vector<string> &split(const string &s, char delim, vector<string> &elems) {
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

vector<string> split(const std::string &s, char delim) {
    vector<string> elems;
    split(s, delim, elems);
    return elems;
}

// Create struct tm from a given input timestamp
std::chrono::system_clock::time_point parseStructTM(string line)
{
    int year, month, day, hour, min, second, millisecond;
    struct tm tm;
    sscanf(line.c_str(), "%d-%d-%dT%d:%d:%d.%d", &year, &month, &day, &hour, &min, &second,&millisecond);
    tm.tm_year = year-1900;
    tm.tm_mon = month-1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = second;
    tm.tm_isdst = -1;


    std::time_t tt = mktime(&tm);
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(tt);
    // Add millisecond
    tp += std::chrono::milliseconds(millisecond);

    return tp;
}

struct event
{
    std::chrono::system_clock::time_point ts;
    string comment_id;
};

struct comment
{
    std::chrono::system_clock::time_point ts;
    std::chrono::system_clock::time_point exp;
    string comment_id;
    string content;
    int n_relation;
    int n_maxclique;
    set<string> s_like;
    set<string> s_maxclique;
};

struct friendship
{
    std::chrono::system_clock::time_point ts;
    string user_id1;
    string user_id2;
};

struct like
{
    std::chrono::system_clock::time_point ts;
    string user_id;
    string comment_id;
};

// Convert timepoint to the desired output
string formatDateTime(std::chrono::system_clock::time_point tp)
{
    char buff[20];
    time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::chrono::duration<double> mlsec = tp - std::chrono::system_clock::from_time_t(tt);
    strftime(buff, 20, "%Y-%m-%dT%H:%M:%S", localtime(&tt));
    ostringstream ss;
    sprintf(buff, "%s.%03d",buff, (int)(mlsec.count()*1000));
    return string(buff)+"+0000";
}

void extractComment(struct comment *comment, string line)
{
    vector<string> token = split(line, '|');
    comment->ts = parseStructTM(token.at(0));
    comment->comment_id = token.at(1);
    comment -> content = token.at(3);
}

void extractFriendship(struct friendship *friendship, string line)
{
    vector<string> token = split(line, '|');
    friendship->ts = parseStructTM(token.at(0));;
    friendship->user_id1 = token.at(1);
    friendship->user_id2 = token.at(2);
}

void extractLike(struct like *like, string line)
{
    vector<string> token = split(line, '|');
    like->ts = parseStructTM(token.at(0));
    like->user_id = token.at(1);
    like->comment_id = token.at(2);
}

void expand(set<string> R,unordered_map<string, set<string> >& friendships_table, set<string>& Q, set<string>& Qmax)
{
    string p;
    set<string> Rp,temp;
    while(!R.empty())
    {
        p=*R.begin();
        if((Q.size()+R.size())>Qmax.size())
        {
            Q.insert(p);
            if(friendships_table.find(p)!=friendships_table.end())
                temp=friendships_table.at(p);
            for(set<string>::iterator it=R.begin(); it!=R.end(); ++it)
            {
                if(temp.find(*it)!=temp.end())
                {
                    Rp.insert(*it);
                }
            }
            if(!Rp.empty())
            {
                expand(Rp, friendships_table,Q,Qmax);
            }
            else if(Q.size()>Qmax.size())
            {
                Qmax.clear();
                for(set<string>::iterator it=Q.begin();it!=Q.end();++it)
                {
                    Qmax.insert(*it);
                }
            }
            if(Q.find(p)!=Q.end())
                Q.erase(Q.find(p));
        }else
        {
            //cout<<"selesai pros"<<endl;
            return;
        }
        //cout<<"delete R"<<endl;
        R.erase(R.find(p));
    }
}

bool do_insertion_sort(struct comment **topm, int rear, int *va)
{
    int len = rear + 1;
    bool isRankChanged = false;
    for (int i = 1; i < len; i++)
    {
        struct comment *x = topm[i];
        int j;
        for (j = i - 1; j >= 0 && ((topm[j]->n_maxclique < x->n_maxclique)||(topm[j]->n_maxclique == x->n_maxclique && topm[j]->content.compare(x->content)>0 )); j--)
        {
            topm[j + 1] = topm[j];
        }
        topm[j + 1] = x;

        if (j + 1 < K_Q2 && i != j + 1)
            isRankChanged = true;
    }
    *va = topm[rear]->n_maxclique;
    return isRankChanged;
}

void printQ2output(std::chrono::system_clock::time_point ts, struct comment **topm, int rear)
{
    cout << formatDateTime(ts);
    for(int i=0;i<K_Q2;i++)
    {
        if(i<=rear)
            cout<<","<<topm[i]->content;
        else cout<<",-";
    }
    cout<<endl;

}

void fileQ2output(std::chrono::system_clock::time_point ts, struct comment **topm, int rear, ofstream *of)
{
    *of << formatDateTime(ts);
    for(int i=0;i<K_Q2;i++)
    {
        if(i<=rear)
            *of<<","<<topm[i]->content;
        else *of<<",-";
    }
    *of<<endl;

}


string ExecQ2(string comment_file, string friendship_file, string like_file, string output_file, int K_Q2_1, int d_1){
    string totalnlatency;
    int output_ctr=0;
    double tot_latency=0;

    std::chrono::steady_clock::time_point readtime;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    std::stringstream ss1;

    // File arguments
    /*
    string comment_file = argv[1];
    string friendship_file = argv[2];
    string like_file = argv[3];
    string output_file = argv[4];
    K_Q2 = atoi(argv[5]);
    d=atoi(argv[6]);
	*/

    // Write file Streams
    ofstream of (output_file);

    // set
    K_Q2=K_Q2_1;
    d=d_1;

    // Open File Streams
    /*ifstream comments_stm ("comments.dat");
    ifstream likes_stm ("likes.dat");
    ifstream friendships_stm ("friendships.dat");*/

    ifstream comments_stm (comment_file);
    ifstream likes_stm (like_file);
    ifstream friendships_stm (friendship_file);

    queue<struct event> eventq;

    set<string> Q,Qmax;

    // Table
    unordered_map<string, struct comment *> comment_table;
    unordered_map<string, set<string> > friendships_table;
    unordered_map<string, set<string> > likes_table;

    // Initiate Cursors
    struct friendship *cur_friendship = NULL;
    struct comment *cur_comment = NULL;
    struct like *cur_like = NULL;
    struct event *cur_event = NULL;

    std::chrono::system_clock::time_point min_time;
    int MAX_C=K_Q2+10;

    //Sorted List
    int rear = -1;
    struct comment *topm[MAX_C + 1];
    int va = -1;
    bool start_filling=true;

    auto comp = [](struct comment *a, struct comment *b) -> bool
    {
        if (a->n_maxclique < b->n_maxclique)
            return true;
        else if (a->n_maxclique == b->n_maxclique)
        {
            if (a->content.compare(b->content)<0)
                return true;
            else
                return false;
        }
        else
            return false;
    };

    set<string> mSet;

    if (cur_comment == NULL && !comments_stm.eof())
    {
        cur_comment = new struct comment;
        string line; // buffer string
        getline(comments_stm, line);
        if (line.length() != 0) {
            extractComment(cur_comment, line);
        }
        else
        {
            free(cur_comment);
            cur_comment=NULL;
        }
    }
    if (cur_friendship == NULL && !friendships_stm.eof())
    {
        cur_friendship = new struct friendship;
        string line; // buffer string
        getline(friendships_stm, line);
        if (line.length() != 0){
            extractFriendship(cur_friendship, line);
        }
        else
        {
            free(cur_friendship);
            cur_friendship=NULL;
        }
    }
    if (cur_like == NULL && !likes_stm.eof())
    {
        cur_like = new struct like;
        string line; // buffer string
        getline(likes_stm, line);
        if (line.length() != 0){
            extractLike(cur_like, line);
        }

        else
        {
            free(cur_like);
            cur_like=NULL;
        }
    }
    if (cur_event == NULL && !eventq.empty())
    {
        cur_event = new struct event(eventq.front());
        eventq.pop();
    }

    readtime = std::chrono::steady_clock::now() ;

    while(cur_comment != NULL || cur_friendship != NULL || cur_like != NULL || cur_event != NULL)
    {

        bool isRankChanged = false;
        bool isOnlyEvent = true;

        // Find the lowest timestamp
        std::chrono::system_clock::time_point min_time = std::chrono::system_clock::time_point::max();
        if(cur_comment != NULL && cur_comment->ts < min_time)
            min_time = cur_comment->ts;
        if(cur_friendship != NULL && cur_friendship->ts < min_time)
            min_time = cur_friendship->ts;
        if(cur_like != NULL && cur_like->ts < min_time)
            min_time = cur_like->ts;
        if(cur_event != NULL && cur_event->ts < min_time)
            min_time = cur_event->ts;


        /*
        cout<< "c " << formatDateTime(cur_comment->ts) << endl;
        cout<< "l " << formatDateTime(cur_like->ts) << endl;
        cout<< "f " << formatDateTime(cur_friendship->ts) << endl;
        if(cur_event != NULL)
            cout<< "e " << formatDateTime(cur_event->ts) << endl;
        cout<< "m " << formatDateTime(min_time) << endl;
        cout << "----------------" <<endl;
        */

        // Step 1: new comment
        while(cur_comment != NULL && cur_comment->ts == min_time)
        {
            isOnlyEvent=false;
            cur_comment->n_relation=0;
            cur_comment->n_maxclique=0;
            comment_table.insert({cur_comment->comment_id, cur_comment});
            //cout<< formatDateTime(cur_comment->ts ) << endl;
            // Insert an event trigger
            struct event t1 {cur_comment->ts + std::chrono::seconds(d), cur_comment->comment_id};
            eventq.push(t1);

            //cout<< formatDateTime(t1.ts) << endl;


            //free(cur_comment);
            cur_comment = NULL;

            if (!comments_stm.eof())
            {
                cur_comment = new struct comment;
                string line; // buffer string
                getline(comments_stm, line);
                if (line.length() != 0)
                {
                    extractComment(cur_comment, line);
                }
                else {
                    free(cur_comment);
                    cur_comment = NULL;
                }
            }

            if (cur_event == NULL && !eventq.empty())
            {
                cur_event = new struct event(eventq.front());
                eventq.pop();
            }
        }

        //step 2: new event
        while(cur_event!=NULL && cur_event->ts==min_time)
        {
            //cout << "F1";
            //cout << "F2";
            //checking change of the rank
            if(mSet.find(cur_event->comment_id)!=mSet.end())
            {
                comment_table.at(cur_event->comment_id)->n_maxclique=-1;
                isRankChanged = do_insertion_sort(topm, rear, &va)||isRankChanged;		// Sort and remove the last
                mSet.erase(topm[rear]->comment_id);
                rear = rear - 1;
                va = topm[rear]->n_maxclique;
            }


            //erasing data from comment_table
            free(comment_table.at(cur_event->comment_id));
            comment_table.erase(cur_event->comment_id);


            //cout << "F3";
            free(cur_event);
            cur_event = NULL;

            if (!eventq.empty())
            {
                cur_event = new struct event(eventq.front());
                eventq.pop();
            }

        }

        // Check whether there is at least one spare
        if (!start_filling && (rear + 1 <= K_Q2))
        {
            int num_c=0;
            // Re-sorting topm from post_table

            priority_queue<struct comment *, vector<struct comment *>, decltype(comp)> max_heap(comp);

            for (unordered_map<string, struct comment *>::iterator it =comment_table.begin(); it != comment_table.end(); ++it)
            {
                if(it->second->n_maxclique>0)
                {
                    max_heap.push(it->second);
                    num_c++;
                }
            }

            mSet.clear();
            if(num_c>MAX_C)
                num_c=MAX_C;
            else start_filling=true;

            for (int i = 0; i < num_c; i++)
            {
                topm[i] = max_heap.top();
                mSet.insert(topm[i]->comment_id);
                max_heap.pop();
            }
            rear = num_c - 1;
        }

        //step 3: new friendship
        while(cur_friendship!=NULL && cur_friendship->ts==min_time)
        {
            isOnlyEvent=false;

            //updating friendships table
            if(friendships_table.find(cur_friendship->user_id1)!=friendships_table.end())
            {
                friendships_table.at(cur_friendship->user_id1).insert(cur_friendship->user_id2);
            }
            else
            {
                friendships_table.insert({cur_friendship->user_id1, {cur_friendship->user_id2}});
            }
            if(friendships_table.find(cur_friendship->user_id2)!=friendships_table.end())
            {
                friendships_table.at(cur_friendship->user_id2).insert(cur_friendship->user_id1);
            }
            else
            {
                friendships_table.insert({cur_friendship->user_id2, {cur_friendship->user_id1}});
            }

            //calculation of clique
            if(likes_table.find(cur_friendship->user_id1)!=likes_table.end())
            {
                for(set<string>::iterator it=likes_table.at(cur_friendship->user_id1).begin();it!=likes_table.at(cur_friendship->user_id1).end();++it)
                {
                    if((likes_table.find(cur_friendship->user_id2)!=likes_table.end()))
                    {
                        if(likes_table.at(cur_friendship->user_id2).find(*it)!=likes_table.at(cur_friendship->user_id2).end())
                        {
                            if(comment_table.find(*it)!=comment_table.end())
                            {
                                //updating #relations
                                comment_table.at(*it)->n_relation+=1;

                                //checking if there are enough relations to create bigger clique
                                if(comment_table.at(*it)->n_relation>=((comment_table.at(*it)->n_maxclique+1)*(comment_table.at(*it)->n_maxclique)/2))
                                {
                                    //calculate maximum clique
                                    Q.clear();
                                    Qmax.clear();
                                    expand(comment_table.at(*it)->s_like,friendships_table, Q,Qmax);

                                    //the maximum clique changes
                                    if((comment_table.at(*it)->n_maxclique)<Qmax.size())
                                    {
                                        //updating the number of maximum clique
                                        comment_table.at(*it)->n_maxclique=Qmax.size();

                                        //check if it is inside topm
                                        if(mSet.find(*it)!=mSet.end())
                                        {
                                            isRankChanged = do_insertion_sort(topm, rear, &va)||isRankChanged;
                                        }
                                        else if(va<=comment_table.at(*it)->n_maxclique)
                                        {
                                            if (rear + 1 < MAX_C) // has a slot in topm
                                            {
                                                mSet.insert(*it);
                                                topm[++rear] = comment_table.at(*it);
                                                isRankChanged = do_insertion_sort(topm, rear, &va)||isRankChanged;
                                            }
                                            else // not available slot in topm
                                            {
                                                mSet.insert(*it);
                                                topm[++rear] = comment_table.at(*it);
                                                isRankChanged = do_insertion_sort(topm, rear, &va)||isRankChanged;
                                                // Delete and adjust the last element va
                                                mSet.erase(topm[rear]->comment_id);
                                                rear = rear - 1;
                                                va = topm[rear]->n_maxclique;
                                            }
                                        }
                                    }

                                }

                            }
                            else //cleaning like table
                            {
                                //cout<<10.1<<endl;
                                //likes_table.at(cur_friendship->user_id1).erase(*it);
                                //cout<<10.2<<endl;
                                //likes_table.at(cur_friendship->user_id2).erase(*it);
                            }
                        }
                    }
                }
            }

            free(cur_friendship);
            cur_friendship = NULL;

            if (!friendships_stm.eof())
            {
                cur_friendship = new struct friendship;
                string line; // buffer string
                getline(friendships_stm, line);
                if (line.length() != 0){
                    extractFriendship(cur_friendship, line);
                }
                else {
                    free(cur_friendship);
                    cur_friendship = NULL;
                }
            }
        }

        //step 4: new like
        while(cur_like!=NULL && cur_like->ts==min_time)
        {
            isOnlyEvent=false;

            //updating comment table

            //checking if it is still a valid like (in d seconds)
            if(comment_table.find(cur_like->comment_id)!=comment_table.end())
            {
                //finding additional relations related to the current comment
                int f_counter=0;
                if(friendships_table.find(cur_like->user_id)!=friendships_table.end())
                {
                    if(comment_table.at(cur_like->comment_id)->s_like.size()<friendships_table.at(cur_like->user_id).size())
                    {
                        for(set<string>::iterator it=comment_table.at(cur_like->comment_id)->s_like.begin();it!=comment_table.at(cur_like->comment_id)->s_like.end();++it)
                        {
                            if(friendships_table.at(cur_like->user_id).find(*it)!=friendships_table.at(cur_like->user_id).end())
                                f_counter++;
                        }
                    }
                    else{
                        for(set<string>::iterator it=friendships_table.at(cur_like->user_id).begin(); it !=friendships_table.at(cur_like->user_id).end(); ++it)
                        {
                            if(comment_table.at(cur_like->comment_id)->s_like.find(*it)!=comment_table.at(cur_like->comment_id)->s_like.end())
                                f_counter++;
                        }
                    }
                }
                comment_table.at(cur_like->comment_id)->n_relation+=f_counter;


                //updating comment table
                comment_table.at(cur_like->comment_id)->s_like.insert(cur_like->user_id);

                //updating likes table
                //there has been already in the table
                if(likes_table.find(cur_like->user_id)!=likes_table.end())
                {
                    likes_table.at(cur_like->user_id).insert(cur_like->comment_id);
                }
                else //not in the table
                {
                    likes_table.insert({cur_like->user_id,{cur_like->comment_id}} );
                }

                //checking if there are enough relations to create bigger clique
                if(comment_table.at(cur_like->comment_id)->n_relation>=((comment_table.at(cur_like->comment_id)->n_maxclique+1)*(comment_table.at(cur_like->comment_id)->n_maxclique)/2))
                {
                    //calculate maximum clique
                    Q.clear();
                    Qmax.clear();
                    expand(comment_table.at(cur_like->comment_id)->s_like,friendships_table, Q,Qmax);

                    //check if the maximum clique changes
                    if((comment_table.at(cur_like->comment_id)->n_maxclique)<Qmax.size())
                    {
                        comment_table.at(cur_like->comment_id)->n_maxclique=Qmax.size();

                        //checking if it is inside topm: no
                        if(mSet.find(cur_like->comment_id)==mSet.end())
                        {
                            //checking if it has surpassed the MAX_C
                            if(start_filling && (rear + 1 == MAX_C))
                                start_filling=false;

                            // For very first posts, if #post is less than k -> output updated
                            if (start_filling)
                            {
                                if(rear+1 < K_Q2)
                                {
                                    mSet.insert(cur_like->comment_id);
                                    topm[++rear] = comment_table.at(cur_like->comment_id);
                                    // Sort and Print
                                    do_insertion_sort(topm, rear, &va);	// va is changed to the last rank
                                    isRankChanged = true;
                                }
                                else
                                {
                                    mSet.insert(cur_like->comment_id);
                                    topm[++rear] = comment_table.at(cur_like->comment_id);
                                    // Sort and Print
                                    isRankChanged = do_insertion_sort(topm, rear, &va)||isRankChanged;
                                }
                            }
                            else if (va <= comment_table.at(cur_like->comment_id)->n_maxclique)
                            {
                                if (rear + 1 < MAX_C) // has a slot in topm
                                {
                                    mSet.insert(cur_like->comment_id);
                                    topm[++rear] = comment_table.at(cur_like->comment_id);
                                    isRankChanged = do_insertion_sort(topm, rear, &va)||isRankChanged;
                                }
                                else // not available slot in topm
                                {
                                    mSet.insert(cur_like->comment_id);
                                    topm[++rear] = comment_table.at(cur_like->comment_id);
                                    isRankChanged = do_insertion_sort(topm, rear, &va)||isRankChanged;
                                    // Delete and adjust the last element va
                                    mSet.erase(topm[rear]->comment_id);
                                    rear = rear - 1;
                                    va = topm[rear]->n_maxclique;
                                }
                            }
                        }else //if it is inside topm
                        {
                            isRankChanged = do_insertion_sort(topm, rear, &va)||isRankChanged;
                        }
                    }
                }
            }
            free(cur_like);
            cur_like = NULL;

            if (!likes_stm.eof())
            {
                cur_like = new struct like;
                string line; // buffer string
                getline(likes_stm, line);
                if (line.length() != 0){
                    extractLike(cur_like, line);
                }
                else {
                    free(cur_like);
                    cur_like = NULL;
                }
            }
        }

        if (isRankChanged)
        {
            printQ2output(min_time, topm, rear);
            fileQ2output(min_time,topm,rear,&of);
            std::chrono::steady_clock::time_point writetime = std::chrono::steady_clock::now() ;
            millisecs_t duration( std::chrono::duration_cast<millisecs_t>(writetime-readtime) ) ;
            output_ctr++;
            tot_latency+=(duration.count());
            //cout << duration.count() << endl;
        }

        if(!isOnlyEvent)
        {
            readtime = std::chrono::steady_clock::now() ;
        }
    }

    friendships_stm.close();
    likes_stm.close();
    comments_stm.close();

    of.close();

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now() ;
    millisecs_t duration( std::chrono::duration_cast<millisecs_t>(end-start) ) ;
    ss1 << std::fixed << std::setprecision(6) << duration.count()/1000.0 << " ";


    ss1 << std::fixed << std::setprecision(6) << tot_latency/output_ctr/1000.0;
    totalnlatency+=ss1.str();

    return totalnlatency;
}


int main()
{
    cout<<ExecQ2("comments1.dat", "friendships1.dat", "likes1.dat", "output.dat",3,43200)<<endl;
    return 0;
}