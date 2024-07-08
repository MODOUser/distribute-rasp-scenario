//LINUX/UNIX c获取某个目录下的所有文件的文件名
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <dirent.h>
#include <string>
using namespace std;
int main(int argc, char * argv[])
{
    struct dirent *ptr;    
    DIR *dir;
    string task = "1345847a";
    dir=opendir("./../../model/");
    printf("文件列表:\n");
    while((ptr=readdir(dir))!=NULL)
    {
 
        //跳过'.'和'..'两个目录
        if(ptr->d_name[0] == '.')
            continue;
        printf("%s\n",ptr->d_name);
    }
    closedir(dir);
    return 0;
//    string path = "./hello";
//    int isCreate = mkdir(path.c_str(),S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
//    if( !isCreate )
//        printf("create path:%s\n",path);
//    else
//        printf("create path failed! error code : %s \n",isCreate,path);
//    return 0;
}
