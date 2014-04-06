. lib

lart_c aa:andersen <<EOF
#include <pthread.h>

int main() {
    pthread_self();
    return 0;
}
EOF
