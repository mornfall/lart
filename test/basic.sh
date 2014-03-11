. lib

lart_c aa:andersen <<EOF
void fun(int **x, int *y) {
    *x = y;
}

int main() {
    int a = 0;
    int *b = &a;
    int *c = &a;
    fun(&b, c);
    fun(&c, b);
}
EOF
