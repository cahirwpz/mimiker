const char* string = "This is an example string.";

#define BYTES(x) (sizeof(x)/sizeof(char))

int main(){
    char array[sizeof(string) + 1];
    int i = 0, j = 0;
    while(1) array[(i++) % BYTES(array)] = string[(j++) % BYTES(string)];
}
