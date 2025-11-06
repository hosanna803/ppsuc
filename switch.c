#include <stdio.h>

void main() {
    int num1, num2, choice;

    printf("Enter two numbers: ");
    scanf("%d%d", &num1, &num2);

    printf("Choose operation:\n");
    printf("1 for addition\n");
    printf("2 for subtraction\n");
    printf("3 for multiplication\n");
    printf("4 for division\n");
    scanf("%d", &choice);

    switch (choice) {
        case 1:
            printf("%d + %d = %d\n", num1, num2, num1 + num2);
            break;
        case 2:
            printf("%d - %d = %d\n", num1, num2, num1 - num2);
            break;
            case 3:
            	printf("%d * %d = %d\n", num1, num2, num1 * num2);
            case 4:
            	printf("%d / %d = %d\n", num1, num2, num1 / num2);
       break;
    }
}
