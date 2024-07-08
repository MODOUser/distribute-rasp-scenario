#include"./../../src/ftp/file-transfer.h"
using namespace std;
int main() {
    if (LocalCopy("./../../4LayerModelhead.h5", "./copyhh")) {
        cout << "复制文件成功" << endl;
    }
    else {
        cout << "复制文件失败" << endl;
     }
    return 0;
}

