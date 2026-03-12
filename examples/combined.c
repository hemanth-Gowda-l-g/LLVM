int main() {
    int secret_math = 100 + 200; // This should be folded
    int ignored_var = 999;       // This should be deleted (DCE)
    return secret_math;
}