#include <iostream>
#include <emu.h>
#include <cvt.h>

using namespace std;

int main() {
    float32 a;
    uint32  b;

    cout << "Enter FLOAT32 : ";
    cin >> a;

    b = float32tofloat16(a);
    cout << endl << "FLOAT16 equivalent : " << hex << b << dec << endl; 
    cout <<         "Back to FLOAT32    : " << float16tofloat32(b) << endl;
}
