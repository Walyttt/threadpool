#include <iostream>
#include <vector>
#include <functional>
#include "threadpool.h"

int Fbi(int i){
   if(i == 1 || i == 2){
        return 1;
   }else{
        return Fbi(i-1)+Fbi(i-2);
   }
}

int main(){
    threadpool tp(3);
    std::vector<std::future<int>> fu(100);

    tp.init();
    for(int i = 0; i < 100; i++){
        fu[i] = tp.submit(Fbi, i + 1);
    }
    tp.shutdown();

    for(int i = 0; i < 100; i++){
        std::cout << fu[i].get() << std::endl;
    }

    return 0;
}