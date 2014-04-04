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

lart_c aa:andersen <<EOF
int main() {
    int x, y;
    int *b;

    if (__divine_choice(2))
        b = &x;
    else
        b = &y;
}
EOF

lart_c aa:andersen <<EOF
int a;

int main() {
    int x, y;
    int *b = &a;

    if (__divine_choice(2))
        b = &x;
    else
        b = &y;
}
EOF
