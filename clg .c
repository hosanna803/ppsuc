#include <stdio.h>
#include <math.h>

int main() {
    double a, b, c, D, r1, r2, img, real;
    printf("Enter values for a, b, and c: ");
    scanf("%lf%lf%lf", &a, &b, &c);

    D = b*b - 4*a*c;

    if (D > 0) {
        r1 = (-b + sqrt(D)) / (2*a);
        r2 = (-b - sqrt(D)) / (2*a);
        printf("Root1 = %.2lf\nRoot2 = %.2lf\n", r1, r2);
    } else if (D == 0) {
        r1 = r2 = -b / (2*a);
        printf("Root1 = %.2lf\nRoot2 = %.2lf\n", r1, r2);
    } else {
        real = -b / (2*a);
        img = sqrt(-D) / (2*a);
        printf("Root1 = %.2lf + %.2lf i\n", real, img);
        printf("Root2 = %.2lf - %.2lf i\n", real, img);
    }

    return 0;
}
