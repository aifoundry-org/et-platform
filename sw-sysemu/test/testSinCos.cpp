#include <iostream>
#include <emu.h>
#include <cvt.h>
#include <math.h>

using namespace std;

int main() {
    iufval val;
    iufval sinval;
    iufval cosval;

    cout << "Enter FLOAT32 : ";
    while( cin >> val.f ) {
        cout << endl << "Number x = " << val.f << endl;
        sinval.f = sin(val.f);
        cosval.f = cos(val.f);
        cout << endl << "sin(x) = " << sinval.f << endl; 
        cout << endl << "cos(x) = " << cosval.f << endl; 
        cout << "Enter FLOAT32 : ";
    }
}
