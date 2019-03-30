#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int test_gimple_cst() {
    const int t = time(NULL);
    printf("%d", t);
    printf("%f", 5.25);
    printf("%f", 5.25f);
    int arr[] = { 1, 2, 3, 4, 5 };
    printf("[");
    for (int i = 0; i < sizeof(arr) / sizeof(arr[0]); i++) {
        printf("%d ", i);
    }
    printf("]");
    return 0;
}

int test_gimple_arithm() {
    int a, b, c, d, e, f;
    scanf("%d%d%d%d%d%d", &a, &b, &c, &d, &e, &f);
    int g;
    g = (a * b / c - f) ^ ((d << e) | f);
    return 0;
}

int test_gimple_phi() {
    int w, x, y, z;
    scanf("%d", &x);
    x = x - 3;
    if (x < 3) {
        y = x * 2;
        w = y;
    } else {
        y = x - 3;
    }
    w = x - y;
    z = x + y;
    printf("w: %d, x: %d, y: %d, z: %d",
           w, x, y, z);
    return 0;
}

struct TEST_MEM {
    int a;
    int b;
    int c;
};

struct TEST_MEM*test_gimple_memory() {
    int n;
    scanf("%d", &n);
    struct TEST_MEM*arr = (struct TEST_MEM*) malloc(n * sizeof(struct TEST_MEM));
    for (int i = 0; i < n; i++) {
        arr[i].a = i;
        arr[i].b = i * i;
        (arr + i)->c = i * i * i;
    }
    
    return arr;
}

int main() {
    test_gimple_cst();
    test_gimple_arithm();
    test_gimple_phi();
    test_gimple_memory();

    return 0;
}
