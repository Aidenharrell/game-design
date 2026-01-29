#include <iostream>

int fun(){
    for (int x=0;x <= 5; ++x){
        std::cout << "hey there" <<std::endl;
    }
    return 0;
}
int main(){
    std::cout << "hello world" << std::endl;
    std::cout << "this is to make sure it works." << std::endl;
    fun();
    return 0;
}
