/************************************************************
FileName: cluster_sci14.cpp
Author: Yunfei WANG
E-mail: wangyunfeiysm@163.com
Date: Nov.23,2014
***********************************************************/

#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <string>
#include <cstring>
#include <iterator>
#include <fstream>
#include <sstream>

//using namespace std;
using std::cin;
using std::cout;
using std::endl;
using std::cerr;
using std::string;
using std::stringstream;
using std::ifstream;
using std::ofstream;
using std::vector;
using std::map;

//calculate the distance between two samples with Euclidean distance
double EuclideanDistance(const vector<double>& vec1,const vector<double>& vec2);
//calculate the distance between two samples with cosine distance
double cosineDistance(const vector<double>& vec1,const vector<double>& vec2);
//get the data from symmetric matrix
double getMatrixData(double** matrix,int i,int j);
//set the value of specific position in the matrix
void setMatrixData(double** matrix,int i,int j,double val);
//calculate the two-dimensional distance matrix
void distanceMatrix(const vector<vector<double> >& vec,double** matrix,
                    double (*metricfun)(const vector<double>&,const vector<double>&));
//searh for appropriate search radius
double searchRadius(double** matrix,int Num,double tau=0.02);
//calculate the density for each sample
void density(double** matrix,int Num,double threshold,int mode,int nn,double* res);
//get the minimum distance delta_i=min(d_ij) where delta_j>delta_i
void getDelta(double** matrix,int Num,const double* rho,double* delta,
              int* neighbor,int* order);
//sort by density and store the index of corresponding samples
void sortByDensity(const double* rho,int Num,int* index);
//find number of clusters automaticlly with Anomaly Detection
int numberOfClusters(const double* pgamma,int Num,double threshold);
//compute the cumulative distribution function of normal distribution
double CDFofNormalDistribution(double x);
//find initial nClus cluster centers
int findInitialCenters(const double* rho,const double* delta,int Num,
                       int nClus,vector<int>& vec);
//assigen cluster centers to samples
void assignClusters(const int* order,const int* neighbor,int Num,
                    const vector<int>& vec,int* res);
//separate halos from cores of each cluster
void filterHalos(double** matrix,int Num,int nClus,const int* clus,
                 const double* rho,double radius,int* halo);
//algorithm of clustering
void clustering(const vector<vector<double> >& vec,int nClus,int mode,int nn,
                double tau,int metric,const string& outputfile,int* clus);
//read data from file
void readData(const char* filename,int withlabel,vector<vector<double> >& data_vec,
              vector<int>& label_vec);
//check the correctness of function computing CDF of normal distribution
void checkCDF();
//process parameters
void processParams(const string& line,string& inputfile,int& nClus,int& nn,\
                   int&mode,double& tau,int& metric,int& withlabel,string& outputfile);
//print help information
void help();

int main(int argc,char* argv[])
{
    //collect options and corresponding parameters
    string para_line;
    for(int i=1;i<argc;++i)
        para_line+=string(argv[i])+' ';

    //parse the following parameters stored in the para_line
    string inputfile;
    int nClus;
    int nn;
    double tau;
    int metric;
    int mode;
    int withlabel=0;
    string outputfile;
    processParams(para_line,inputfile,nClus,nn,mode,
                  tau,metric,withlabel,outputfile);

    //read data from inputfile
    vector<vector<double> > data_vec;
    vector<int> label_vec;

	cout<<"reading data...\n";
    readData(inputfile.c_str(),withlabel,data_vec,label_vec);

    //clustering procedure
    int Num=data_vec.size();
    int* res=new int[Num];//used to store clustering results
    clustering(data_vec,nClus,mode,nn,tau,metric,outputfile,res);

    delete[] res;//free memory
	return 0;
}

//calculate the distance between two vectors with Euclidean metric
double EuclideanDistance(const vector<double>& vec1,const vector<double>& vec2)
{
	double res=0.0;
	vector<double>::const_iterator iter1=vec1.begin();
	vector<double>::const_iterator iter2=vec2.begin();
	while(iter1!=vec1.end()&&iter2!=vec2.end())
	{
		res+=(*iter1-*iter2)*(*iter1-*iter2);
		++iter1;
		++iter2;
	}
	return sqrt(res);
}

//calculate the distance between two samples with cosine distance
double cosineDistance(const vector<double>& vec1,const vector<double>& vec2)
{
	vector<double>::const_iterator iter1=vec1.begin();
	vector<double>::const_iterator iter2=vec2.begin();
	double vec_product=0.0;
	double norm1=0.0,norm2=0.0;
	while(iter1!=vec1.end()&&iter2!=vec2.end())
	{
		vec_product+=((*iter1)*(*iter2));
		norm1+=(*iter1)*(*iter1);
		norm2+=(*iter2)*(*iter2);
        ++iter1;
        ++iter2;
	}
	double eps=1e-6;
	double res;
	if(fabs(norm1)<=eps||fabs(norm2)<=eps)
		res=0.0;//vector with norm 0 are parallel to any vector
	else
        res=vec_product/sqrt(norm1*norm2);
	return res;
}

//get the data from a symmetric matrix
//only the elements in the up triangle region is stored
double getMatrixData(double** matrix,int i,int j)
{
	int row=i<j?i:j;
	int col=i>j?i:j;
	col-=row;
	return *(*(matrix+row)+col);
}

//set the value of specific position in the matrix
//only the elements in the up triangle region is stored
void setMatrixData(double** matrix,int i,int j,double val)
{
	int row=i<j?i:j;
	int col=i>j?i:j;
	col-=row;
	*(*(matrix+row)+col)=val;
}

//calculate the two-dimensional distance matrix
void distanceMatrix(const vector<vector<double> >& vec,double** matrix,
                    double (*metricfun)(const vector<double>&,const vector<double>&))
{
	size_t sz=vec.size();
	double dist=0.0;
	for(size_t i=0;i<sz;++i)
	{
		setMatrixData(matrix,i,i,0);
		for(size_t j=i+1;j<sz;++j)
		{
			dist=metricfun(vec[i],vec[j]);
			setMatrixData(matrix,i,j,dist);
		}
	}
}

//searh for appropriate search radius
double searchRadius(double** matrix,int Num,double tau)
{
	int nElem=Num*(Num-1)/2;
	vector<double> dist;
	dist.reserve(nElem);
	int cnt=0;
	for(int i=0;i<Num-1;++i)
		for(int j=i+1;j<Num;++j)
			dist[cnt++]=getMatrixData(matrix,i,j);

	int pos=int(round(tau*Num));//position of d_c
	nth_element(dist.begin(),dist.begin()+pos-1,dist.end());
	return dist[pos-1];
}

//calculate the density for each sample
void density(double** matrix,int Num,double radius,int mode,int nn,double* rho)
{
    double dist,val;
    memset(rho,0,sizeof(double)*Num);//reset values in res
    vector<double> vec(Num,0);

    switch(mode)
    {
    case 0://Gaussian kernel
        cout<<"Gaussian kernel"<<endl;
        for(int i=0;i<Num;++i)
            for(int j=i+1;j<Num;++j)
            {
                dist=getMatrixData(matrix,i,j);
                val=exp(-(dist/radius)*(dist/radius));
                *(rho+i)+=val;
                *(rho+j)+=val;
            }
        break;
    case 1://cutoff kernel
        for(int i=0;i<Num;++i)
            for(int j=i+1;j<Num;++j)
            {
                dist=getMatrixData(matrix,i,j);
                if(dist<radius)
                {
                    *(rho+i)+=1;
                    *(rho+j)+=1;
                }
            }
        break;
    case 2://KNN
        for(int i=0;i<Num;++i)
        {
            for(int j=0;j<Num;++j)
                vec[j]=-getMatrixData(matrix,i,j);
            make_heap(vec.begin(),vec.end());

            vector<double>::iterator first=vec.begin();
            vector<double>::iterator last=vec.end();

            for(int jj=0;jj<nn;++jj)
                pop_heap(first,last--);

            last=vec.end();
            double sum=.0;
            for(int t=0;t<nn;++t)
                sum+=*(--last);
            *(rho+i)=sum/nn;
        }
        break;
    default:
        cerr<<"Invalid option for computing density"<<endl;
        help();
        exit(0);
    }
}

//sort by density and store the index of corresponding samples
void sortByDensity(const double* rho,int Num,int* order)	
{	
	for(int t=0;t<Num;++t)
		order[t]=t;
	for(int i=0;i<Num-1;++i)
	{
		int max=i;
		for(int j=i+1;j<Num;++j)
			if(*(rho+order[max])<*(rho+order[j]))
				max=j;
        std::swap(order[i],order[max]);
	}
}

//compute the cumulative distribution function of normal distribution
double CDFofNormalDistribution(double x)
{
	const double PI=3.1415926;
	double p0=220.2068679123761;
	double p1=221.2135961699311;
	double p2=112.0792914978709;
	double p3=33.91286607838300;
	double p4=6.373962203531650;
	double p5=.7003830644436881;
	double p6=.03326249659989109;

	double q0=440.4137358247552;
	double q1=793.8265125199484;
	double q2=637.3336333788311;
	double q3=296.5642487796737;
	double q4=86.78073220294608;
	double q5=16.06417757920695;
	double q6=1.755667163182642;
	double q7=0.08838834764831844;

	double cutoff=7.071;//10/sqrt(2)
	double root2pi=2.506628274631001;//sqrt(2*PI)

	double xabs=abs(x);

	double res=0;
	if(x>37.0) 
		res=1.0;
	else if(x<-37.0)
		res=0.0;
	else
	{
		double expntl=exp(-.5*xabs*xabs);
		double pdf=expntl/root2pi;
		if(xabs<cutoff)
			res=expntl*((((((p6*xabs + p5)*xabs + p4)*xabs + p3)*xabs+ \
				p2)*xabs + p1)*xabs + p0)/(((((((q7*xabs + q6)*xabs + \
				q5)*xabs + q4)*xabs + q3)*xabs + q2)*xabs + q1)*xabs+q0);
		else
			res=pdf/(xabs+1.0/(xabs+2.0/(xabs+3.0/(xabs+4.0/(xabs+0.65)))));
	}
	if(x>=0.0)
		res=1.0-res;
	return res;
}

//get the minimum distance delta_i=min(d_ij)
//where the density of j-th sample is greater than that of the i-th one
void getDelta(double** matrix,int Num,const double* rho,double* delta,
              int* neighbor,int* order)
{
    sortByDensity(rho,Num,order);
    double globalMax=getMatrixData(matrix,0,0);
    for(int i=0;i<Num;++i)
    {
        double min=getMatrixData(matrix,order[i],order[0]);
        double buf=0;
        *(neighbor+order[i])=order[0];
        for(int j=0;j<i;++j)
        {
            buf=getMatrixData(matrix,order[i],order[j]);
            if(buf>globalMax) globalMax=buf;
            if(buf<min)
            {
                min=buf;
                *(neighbor+order[i])=order[j];
            }
        }
        *(delta+order[i])=min;
    }
    *(delta+order[0])=globalMax;
}

//find the number of clusters automaticlly in the view of
//Anomaly Detection with Gaussian distribution
int numberOfClusters(const double* pgamma,int Num,double threshold)
{
	double* buf=new double[Num]();
	memcpy(buf,pgamma,Num*sizeof(double));
    //sort gamma for each point in ascending order with algorithm in STL
    std::sort(buf,buf+Num);

	double sum=0.0;
	for(int i=0;i<Num;++i)
		sum+=buf[i];
	double mu=sum/Num;//average of Gaussian distribution
	sum=0.0;
	for(int i=0;i<Num;++i)
		sum+=(buf[i]-mu)*(buf[i]-mu);
	double variance=sum/Num;//variance of Gaussian distribution
	double std=sqrt(variance);

    double prob=0;
	int nClusters=1;
	double var;
	for(int i=Num-1;i>=0;--i)
	{
		var=(buf[i]-mu)/std;
        prob=CDFofNormalDistribution(var);
        if(i>Num-100) cout<<" "<<prob;
        if(prob<threshold||(1-prob)<threshold)//abnormal datapoint
			continue;
		nClusters=Num-1-i;
		break;
	}
	delete[] buf;
	return nClusters;
}

//find initial nClus cluster centers
int findInitialCenters(const double* rho,const double* delta,
                       int Num,int nClus,vector<int>& vec)
{
	if(NULL==rho||NULL==delta) 
		return -1;
	vec.clear();
	//scale delta and rho into the range of [0,1]
	double rho_min,rho_max;
	rho_min=rho_max=*rho;
	for(int i=1;i<Num;++i)
	{
		if(*(rho+i)>rho_max) rho_max=*(rho+i);
		else if(*(rho+i)<rho_min) rho_min=*(rho+i);
	}
	double rho_range=rho_max-rho_min;
	
	double delta_min,delta_max;
	delta_min=delta_max=*delta;
	for(int ii=1;ii<Num;++ii)
	{
		if(*(delta+ii)>delta_max) delta_max=*(delta+ii);
		else if(*(delta+ii)<delta_min) delta_min=*(delta+ii);
	}
	double delta_range=delta_max-delta_min;
	
	double *pgamma=new double[Num];
	for(int t=0;t<Num;++t)
		*(pgamma+t)=(*(rho+t)-rho_min)*(*(delta+t)-delta_min)/(rho_range*delta_range);
	
	if(nClus<=0)//found clusters automatically
	{
        double thres=5e-2;
        nClus=numberOfClusters(pgamma,Num,thres);
		cout<<"Number of clusters found "<<nClus<<endl;
	}
	for(int s=0;s<nClus;++s)
	{
		int max=s;
		for(int j=s+1;j<Num;++j)
			if(*(pgamma+max)<*(pgamma+j)) max=j;
        std::swap(*(pgamma+s),*(pgamma+max));
		vec.push_back(max);
	}
	delete[] pgamma;
	return nClus;
}

//separate halos from cores of each cluster
void filterHalos(double** matrix,int Num,int nClus,const int* clus,
                 const double* rho,double radius,int* halo)
{
    memset(halo,0,sizeof(int)*Num);//initialize halo
    if(nClus<=1)
        return;//no need to find halos for a single cluster

    double* boundary_rho=new double[nClus]();//density on the boundary for each cluster
    double dist;
    double avg_rho;
    //calculate the density for the boundary of each cluster
    for(int i=0;i<Num-1;++i)
        for(int j=i+1;j<Num;++j)
        {
            dist=getMatrixData(matrix,i,j);//distance between i and j
            if(dist<=radius&&*(clus+i)!=*(clus+j))
            {
                avg_rho=(*(rho+i)+*(rho+j))/2.0;
                if(boundary_rho[*(clus+i)]<avg_rho)
                    boundary_rho[*(clus+i)]=avg_rho;
                if(boundary_rho[*(clus+j)]<avg_rho)
                    boundary_rho[*(clus+j)]=avg_rho;
            }
        }

    //find the halos for each cluster
    for(int i=0;i<Num;++i)
    {
        //cout<<*(rho+i)<<' '<<*(clus+i)<<' '<<boundary_rho[*(clus+i)]<<endl;
        if(*(rho+i)<boundary_rho[*(clus+i)])
            *(halo+i)=1;
    }
    delete[] boundary_rho;
}

//assigen cluster centers to samples
void assignClusters(const int* order,const int* neighbor,
                    int Num,const vector<int>& vec,int* res)
{
	for(int i=0;i<Num;++i)
		*(res+i)=-1;
	for(size_t sz=0;sz<vec.size();++sz)
		*(res+vec[sz])=sz;
	for(int t=0;t<Num;++t)
		if(*(res+*(order+t))==-1)//waiting for assignment
			*(res+*(order+t))=*(res+*(neighbor+*(order+t)));
}

//algorithm of clustering
//number of nearest neighbors
void clustering(const vector<vector<double> >& vec,int nClus,int mode,\
                int nn,double tau,int metric,const string& outputfile,int* clus)
{
	int Num=vec.size();
	double** matrix=new double*[Num];
	for(int i=0;i<Num;++i)
		*(matrix+i)=new double[Num-i]();
	cout<<"generating distance matrix...\n";
	//calculate the two-dimensional distance matrix
    double(*metricfun)(const vector<double>&,const vector<double>&);
    if(metric==0)//Euclidean
        metricfun=EuclideanDistance;
    else//Cosine
        metricfun=cosineDistance;
    distanceMatrix(vec,matrix,metricfun);
	
	cout<<"computing density for each sample...\n";
	double* rho=new double[Num];

    double radius=searchRadius(matrix,Num,tau);
    cout<<"Radius searched automatically:"<<radius<<endl;
    density(matrix,Num,radius,mode,nn,rho);
	
	cout<<"computing delta for each sample...\n";
	//get delta
	double* delta=new double[Num];
	int* neighbor=new int[Num];
	int* order=new int[Num];
	getDelta(matrix,Num,rho,delta,neighbor,order);

	//save rho and delty into file
    string decisiongraph_file=outputfile+".decisiongraph";
    ofstream graph_out(decisiongraph_file.c_str());
	for(int t=0;t<Num;++t)
        graph_out<<rho[t]<<' '<<delta[t]<<endl;
    graph_out.close();
	
	cout<<"finding initial cluster centers...\n";
	vector<int> clustersVec;
	//find initial nClus cluster centers
	findInitialCenters(rho,delta,Num,nClus,clustersVec);
    //save the index of samples treated as cluster centers into file
    string centers_file=outputfile+".centers";
    ofstream centers_out(centers_file.c_str());
    for(size_t sz=0;sz<clustersVec.size();++sz)
        centers_out<<clustersVec[sz]<<' ';
    centers_out.close();

	cout<<"assigning cluster centers...\n";
	//assign cluster centers to samples
    assignClusters(order,neighbor,Num,clustersVec,clus);

    cout<<"filtering halos from cores of each cluster...\n";
    int* halo=new int[Num];
    filterHalos(matrix,Num,nClus,clus,rho,radius,halo);
	//free memory
	for(int ii=0;ii<Num;++ii)
		delete[] *(matrix+ii);
	delete[] matrix;
	delete[] rho;
	delete[] delta;
	delete[] neighbor;
	delete[] order;

    //save the results of clustering into file
    string clust_file=outputfile+".result";
    ofstream clus_out(clust_file.c_str());
    for(int i=0;i<Num;++i)
        clus_out<<i<<' '<<*(clus+i)<<' '<<*(halo+i)<<endl;
    clus_out.close();

    delete[] halo;
}

//read data from file
void readData(const char* filename,int withlabel,
              vector<vector<double> >& data_vec,vector<int>& label_vec)
{
	stringstream ss;
	ifstream ifs(filename);
	double val;
	if(ifs)
	{
		int Dim=0;
        data_vec.clear();
		string line;
		vector<double> subvec;
		while(getline(ifs,line))
		{
			ss.clear();
			ss<<line;
			while(ss>>val) subvec.push_back(val);
            if(withlabel>0)//the last column is the corresponding label
            {
                vector<double>::iterator last=subvec.end()-1;
                label_vec.push_back(*last);
                subvec.pop_back();
            }
            data_vec.push_back(subvec);
			subvec.clear();
		}
	}
	else 
		cerr<<filename<<" doesn't exist!\n";
}

//check the correctness of function computing CDF of normal distribution
void checkCDF()
{
	ifstream ifs("cdftable.txt");
	char ch;
	double x,res,pred;
	stringstream ss;
	string line;
	while(getline(ifs,line))
	{
		ss.clear();
		ss<<line;
		ss>>ch>>ch>>x>>ch>>ch>>res;
		pred=CDFofNormalDistribution(x);
		cout<<'('<<pred<<','<<1-res<<')'<<endl;
	}
}

//process parameters
void processParams(const string& line,string& inputfile,int& nClus,int& nn,\
                   int& mode,double& tau,int& metric,int& withlabel,string& outputfile)
{
    if(!line.size())
    {
        cerr<<"Options must be provied here."<<endl;
        help();
    }
    //set default values for parameters
    inputfile="";
    nClus=-1;
    nn=5;
    mode=0;//Gauss kernel.
    tau=0.05;//average percent for the number of neighbors of each point
    metric=0;//Euclidean
    outputfile="";//output
    withlabel=0;//without label in the input file

    map<string,int> cmd_map;
    cmd_map["--help"]=-1;
    cmd_map["--input"]=1;
    cmd_map["--clusters"]=2;
    cmd_map["--neighbors"]=3;
    cmd_map["--metric"]=4;
    cmd_map["--mode"]=5;
    cmd_map["--output"]=6;
    cmd_map["--withlabel"]=7;
    cmd_map["--tau"]=8;

    stringstream ss;
    ss<<line;

    string cmd,val;
    vector<string> cmd_vec;
    vector<string> val_vec;
    while(ss>>cmd)
    {
        if(!cmd_map[cmd])//invalid option
        {
            cerr<<"Invalid option:"<<cmd<<endl;
            help();
            exit(0);
        }
        cmd_vec.push_back(cmd);
        if(cmd=="--help")
        {
            val_vec.push_back("help");//just to fill in the position here
            continue;
        }
        ss>>val;
        if(cmd_map[val])//value for last option is missing
        {
            cerr<<"Value missing for option:"<<cmd<<endl;
            help();
            exit(0);
        }
        val_vec.push_back(val);
    }

    for(size_t sz=0;sz<cmd_vec.size();++sz)
    {
        cmd=cmd_vec[sz];
        cout<<"cmd:"<<cmd<<" val:"<<val_vec[sz]<<endl;

        switch(cmd_map[cmd])
        {
        case -1://help
            help();
            break;
        case 1://input file
            inputfile=val_vec[sz];
            cout<<"input:"<<inputfile<<endl;
            if(outputfile=="") outputfile=inputfile;
            break;
        case 2://number of clusters
            nClus=atoi(val_vec[sz].c_str());
            cout<<"clusters:"<<nClus<<endl;
            break;
        case 3://number of nearest neighbors
            ss>>nn;
            cout<<"neighbors:"<<nn<<endl;
            break;
        case 4://metric for distance
            metric=atoi(val_vec[sz].c_str());
            cout<<"metric:"<<metric<<endl;
            break;
        case 5://mode of density computing
            mode=atoi(val_vec[sz].c_str());
            cout<<"mode:"<<mode<<endl;
            break;
        case 6://output of filename
            outputfile=val_vec[sz];
            cout<<"outputfile:"<<outputfile<<endl;
            break;
        case 7://indicates whether the last column are the labels
            withlabel=atoi(val_vec[sz].c_str());
            cout<<"withlabel:"<<withlabel<<endl;
            break;
        case 8://average percent for the number of neighbors of each point
            tau=atof(val_vec[sz].c_str());
            cout<<"tau:"<<tau<<endl;
            if(tau<0.0||tau>1)
            {
                cerr<<"Invalid tau(lies in (0,1)"<<endl;
                exit(0);
            }
            break;
        default:
            cerr<<"Invalid option:"<<cmd<<endl;
            help();
            exit(0);
        }
    }
}

//print help information
void help()
{
    cout<<"==================================================================\n\
OPTIONS\n\
    --input     Requests that all the samples are stored in the given file\n\
                in which each row is a sample.\n\
    --clusters  Specify the number of clusters.\n\
                If clusters<=0, the program will estimate the appropriate\n\
                number of clusters automatically.\n\
    --withlabel Specify whether the last column in the input files are labels.\n\
                0-the last column is also one of the features.\n\
                1-the last column indicates the label of each sample.\n\
    --mode      Specify the approach to compute density.\n\
                0-Gauss Kernel(default)\n\
                1-Cutoff Kernel\n\
                2-K Nearest Neighbors\n\
    --neighbors Specify the number of nearest neighbors, which is\n\
                used to estiamte the density of each sample. It works\n\
                only when the option '--mode 2'  is used.(default 5)\n\
    --tau       Sepcify the average ratio(0<tau<1) between the number of neighbors and the whole\n\
                number of points(default 0.05).\n\
    --metric    Specify the metric used to compute distance two samples.\n\
                0-Euclidean(default)\n\
                1-Cosine\n\
    --output    Specify the name of output files(default:the same as inputfile).\n\
                The file named output.decisiongraph stores the decision graph,\n\
                the two columns correspond to rho and delta respectively for each sample.\n\
                The file named output.result stores the clustering result,\n\
                in which the first column indicates the index of each sample\n\
                and the second column indicate the index of its cluster.\n\
    --help"<<endl;
}
