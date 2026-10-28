extern int source(void);
int leaf(int value) { return value + 1; }
int main(void) {
    int value;
    if (source())
        value = leaf(1);
    else
        value = leaf(2);
    return leaf(value);
}
