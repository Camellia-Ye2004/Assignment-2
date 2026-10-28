int leaf(int value) { return value + 1; }
int middle(int value) { return leaf(value); }
int main(void) {
    int first = middle(1);
    int second = middle(2);
    return first + second;
}
